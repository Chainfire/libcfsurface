package eu.chainfire.libcfsurface.gl;

import android.graphics.Bitmap;
import android.graphics.Typeface;

public class GLTextRenderer extends GLTextRendererBase {
    public GLTextRenderer(GLTextureManager textureManager, GLHelper helper, Typeface typeface, int lineHeight) {
        super(textureManager, helper, typeface, lineHeight);
    }

    @Override
    public Bitmap getBitmap(String text, int color, int width, Justification justification, Bitmap inBitmap) {
        return super.getBitmap(text, color, width, justification, inBitmap);
    }

    @Override
    public GLPicture getPicture(Bitmap bitmap) {
        return super.getPicture(bitmap);
    }

    @Override
    public GLPicture getPicture(String text, int color, int width, Justification justification, Bitmap inBitmap) {
        return super.getPicture(text, color, width, justification, inBitmap);
    }
}
