package eu.chainfire.libcfsurface;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.SystemClock;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Locale;
import java.util.concurrent.locks.ReentrantLock;

import eu.chainfire.librootjava.Logger;
import eu.chainfire.librootjava.RootJava;

public abstract class SurfaceHost {    
    @SuppressLint("SdCardPath")
    public static String getLibraryPath(Context context) {
        return RootJava.getLibraryPath(context, "cfsurface", String.format(Locale.ENGLISH, "/data/data/%s/lib/libcfsurface.so", context.getPackageName()));
    }
    
    public static String getLaunchString(Context context, Class<?> clazz, String app_process32_init) {
        return RootJava.getLaunchString(context, clazz, app_process32_init, new String[] { context.getPackageCodePath(), getLibraryPath(context) });
    }

    public static String getLaunchString(String packageCodePath, String libraryPath, Class<?> clazz, String app_process32_init) {
        return RootJava.getLaunchString(packageCodePath, clazz, app_process32_init, new String[] { packageCodePath, libraryPath });
    }

    private native void nativeInit();
    private native void nativeDone();
    
    private volatile boolean mInitialized = false;
    private volatile boolean mExiting = false;
    private volatile boolean mShow = true;
    private int mWidth = 0;
    private int mHeight = 0;
    private long mLastFrame = 0;
    private String mAPK = null;
    private String mLib = null;

    private Object mSurfaceSession = null;
    private Object mSurfaceControl = null;
    private Method mSurfaceControlOpenTransaction = null;
    private Method mSurfaceControlCloseTransaction = null;
    private Method mSurfaceControlSetLayer = null;
    private Method mSurfaceControlShow = null;
    private Method mSurfaceControlHide = null;

    protected final ReentrantLock lock = new ReentrantLock(true);

    private final long onSetSurfaceControl(int width, int height) {
        // used on API 28 instead of some native calls, could probably be ported a number of
        // API levels back and cut some of the C dlsym madness, but then it'd have to be retested
        // on many devices - libcfsurface is used in apps that run back to 4.2
        try {
            Class<?> cSurfaceSession = Class.forName("android.view.SurfaceSession");
            Constructor<?> ctorSurfaceSession = cSurfaceSession.getConstructor();
            ctorSurfaceSession.setAccessible(true);
            mSurfaceSession = ctorSurfaceSession.newInstance();

            Class<?> cSurfaceControl = Class.forName("android.view.SurfaceControl");
            Constructor<?> ctorSurfaceControl = cSurfaceControl.getDeclaredConstructor(cSurfaceSession, String.class, int.class, int.class, int.class, int.class, cSurfaceControl, int.class, int.class);
            ctorSurfaceControl.setAccessible(true);
            mSurfaceControl = ctorSurfaceControl.newInstance(mSurfaceSession, "CFSurface", width, height, 4 /*PixelFormat.RGB_565*/, 0x00000004 /*SurfaceControl.HIDDEN*/, null, 0, 0);

            mSurfaceControlOpenTransaction = cSurfaceControl.getDeclaredMethod("openTransaction");
            mSurfaceControlCloseTransaction = cSurfaceControl.getDeclaredMethod("closeTransaction");
            mSurfaceControlSetLayer = cSurfaceControl.getDeclaredMethod("setLayer", int.class);
            mSurfaceControlShow = cSurfaceControl.getDeclaredMethod("show");
            mSurfaceControlHide = cSurfaceControl.getDeclaredMethod("hide");

            Field fSurfaceControlNativeObject = cSurfaceControl.getDeclaredField("mNativeObject");
            fSurfaceControlNativeObject.setAccessible(true);
            return fSurfaceControlNativeObject.getLong(mSurfaceControl);
        } catch (Exception e) {
            // this will happen on many devices, means this method of construction the surface isn't supported using this code
            //Logger.ex(e);
        }
        return 0;
    }

    private final void onSetSurfaceLayer(int z) {
        try {
            mSurfaceControlOpenTransaction.invoke(null);
            mSurfaceControlSetLayer.invoke(mSurfaceControl, z);
            mSurfaceControlCloseTransaction.invoke(null);
        } catch (Exception e) {
            Logger.ex(e);
        }
    }

    private final void onSetSurfaceVisible(int visible) {
        try {
            mSurfaceControlOpenTransaction.invoke(null);
            if (visible == 0) {
                mSurfaceControlHide.invoke(mSurfaceControl);
            } else {
                mSurfaceControlShow.invoke(mSurfaceControl);
            }
            mSurfaceControlCloseTransaction.invoke(null);
        } catch (Exception e) {
            Logger.ex(e);
        }
    }
    
    private void initGL() {
        lock.lock();
        try {
            if (!mInitialized) {
                onInitGL();
                mInitialized = true;
            }
        } finally {
            lock.unlock();
        }
    }
    
    private void doneGL() {
        lock.lock();
        try {
            if (mInitialized) {
                onDoneGL();
            }
        } finally {
            lock.unlock();
        }
    }
    
    public final void informSize(int width, int height) {
        mWidth = width;
        mHeight = height;
        onSize(width, height);
    }
        
    public final void doDrawFrame() {
        if (mExiting) {
            if (mInitialized) {
                doneGL();
            }
            return;
        }        
        initGL();                       
        
        onDrawFrameGL();
        
        long now = SystemClock.uptimeMillis();
        long diff = now - mLastFrame;
        if (now - mLastFrame < 17) {
            try {
                Thread.sleep(17 - diff);
            } catch (Exception e) {                
            }
            mLastFrame = now;
        }
    }

    public final int onWantVisible() {
        return mShow ? 1 : 0;
    }

    public static final int reservedArgs = 2;
    
    public final void run(String[] args) {
        try {
            if ((args == null) || (args.length < 2))
                return;
        
            mAPK = args[0];
            mLib = args[1];        
            Runtime.getRuntime().load(mLib);
                        
            String[] initArgs = new String[args.length - reservedArgs];
            for (int i = 0; i < args.length - reservedArgs; i++) {
                initArgs[i] = args[i + reservedArgs];
            }
        
            onInit(initArgs);
            nativeInit();
        
            onMainLoop();
        
            mExiting = true;        
            nativeDone();
            while (true) {
                try { 
                    Thread.sleep(64); 
                } catch (Exception e) {                 
                }
                lock.lock();
                try {
                    if (!mInitialized)
                        break;
                } finally {
                    lock.unlock();
                }
            }
            onDone();
        } catch (Exception e) {
            Logger.ex(e);
        }
        System.exit(0);
    }

    protected void show() {
        mShow = true;
    }

    protected void hide() {
        mShow = false;
    }
    
    protected abstract void onSize(int width, int height);
    protected abstract void onInit(String[] args);
    protected abstract void onInitGL();
    protected abstract void onMainLoop();
    protected abstract void onDrawFrameGL();
    protected abstract void onDoneGL();
    protected abstract void onDone();

    protected boolean isInitialized() {
        return mInitialized;
    }

    protected boolean isExiting() {
        return mExiting;
    }

    protected int getWidth() {
        return mWidth;
    }

    protected int getHeight() {
        return mHeight;
    }

    protected String getAPK() {
        return mAPK;
    }

    protected String getLib() {
        return mLib;
    }
}
