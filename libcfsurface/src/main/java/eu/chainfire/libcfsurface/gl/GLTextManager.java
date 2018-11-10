package eu.chainfire.libcfsurface.gl;

import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.opengl.GLES20;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

public class GLTextManager extends GLTextRendererBase {
    private class Line {
        private final String mText;
        private final int mColor;
        private Bitmap mBitmap = null;
        private GLPicture mPicture = null;

        public Line(String text, int color) {
            mText = text;
            mColor = color;
        }

        public void destroy() {
            if (mPicture != null) {
                mPicture.destroy();
                mPicture = null;
            }
            if (mBitmap != null) {
                mBitmaps.add(mBitmap);
                mBitmap = null;
            }
        }

        public GLPicture getPicture() {
            if (mBitmap == null) {
                if (mBitmaps.size() > 0) {
                    mBitmap = mBitmaps.remove(0);
                }
                mBitmap = GLTextManager.this.getBitmap(mText, mColor, mWidth, Justification.LEFT, mBitmap);
            }
            if (mPicture == null) {
                mPicture = GLTextManager.this.getPicture(mBitmap);
            }
            return mPicture;
        }
    }

    protected final int mLeft;
    protected final int mTop;
    protected final int mWidth;
    protected final int mHeight;
    protected final int mMaxHeight;
    protected final int mLineCount;
    protected final List<Line> mLines = new ArrayList<Line>();
    protected final List<Bitmap> mBitmaps = new ArrayList<Bitmap>();
    protected volatile boolean mWordWrap = true;

    protected final ReentrantLock lock = new ReentrantLock(true);

    public GLTextManager(GLTextureManager textureManager, GLHelper helper, int width, int height, int lineHeight) {
        this(textureManager, helper, 0, 0, width, height, 0, lineHeight);
    }

    public GLTextManager(GLTextureManager textureManager, GLHelper helper, int left, int top, int width, int height, int maxHeight, int lineHeight) {
        super(textureManager, helper, Typeface.MONOSPACE, lineHeight);
        mLeft = left;
        mTop = top;
        mWidth = width;
        mHeight = height;
        mMaxHeight = maxHeight;
        mLineCount = (int)Math.round(Math.ceil((float)mHeight / (float)mLineHeight));
    }

    public ReentrantLock getLock() {
        return lock;
    }

    public void setWordWrap(boolean wordWrap) {
        lock.lock();
        try {
            mWordWrap = wordWrap;
        } finally {
            lock.unlock();
        }
    }

    public void draw() {
        draw(1.0f);
    }

    public void draw(float alpha) {
        lock.lock();
        try {
            boolean scissor = true;
            if (mMaxHeight != 0) {
                scissor = mHelper.scissorOn(mLeft, mTop, mWidth, mHeight, mMaxHeight);
            }

            int top = mTop + mHeight - mLineHeight;
            for (int i = mLines.size() - 1; i >= 0; i--) {
                mHelper.draw(mLines.get(i).getPicture(), mLeft, top, mWidth, mLineHeight, GLPicture.AlphaType.IMAGE, alpha);
                top -= mLineHeight;
            }

            mHelper.scissorOff(scissor);
        } finally {
            lock.unlock();
        }
    }

    private void add(Line line) {
        lock.lock();
        try {
            mLines.add(line);
        } finally {
            lock.unlock();
        }
    }

    private void reduce() {
        lock.lock();
        try {
            while (mLines.size() > mLineCount) {
                mLines.remove(0).destroy();
            }
        } finally {
            lock.unlock();
        }
    }

    public void add(String text, int color) {
        add(text, color, mWordWrap);
    }

    public void add(String text, int color, boolean wordWrap) {
        if (text == null) return;
        if (text.equals("")) {
            add(new Line("", color));
        } else {
            while (text.length() > 0) {
                int max = mPaint.breakText(text, true, mWidth - (mHorizontalPadding * 2), null);
                int lf = text.indexOf('\n');
                if ((lf >= 0) && (lf < max)) {
                    add(new Line(text.substring(0, lf), color));
                    if (text.length() > lf) {
                        text = text.substring(lf + 1);
                    } else {
                        break;
                    }
                } else {
                    add(new Line(text.substring(0, max), color));
                    if (text.length() > max) {
                        text = text.substring(max);
                    } else {
                        break;
                    }
                }
                if (!wordWrap) {
                    break;
                }
            }
        }
        reduce();
    }

    public void removeLastLine() {
        lock.lock();
        try {
            if (mLines.size() > 0) {
                mLines.remove(mLines.size() - 1).destroy();
            }
        } finally {
            lock.unlock();
        }
    }

    @Override
    public void destroy() {
        lock.lock();
        try {
            for (Line line : mLines) line.destroy();
            mLines.clear();

            for (Bitmap bitmap : mBitmaps) bitmap.recycle();
            mBitmaps.clear();
        } finally {
            lock.unlock();
        }
    }
}
