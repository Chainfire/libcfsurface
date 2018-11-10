#define LOG_NDEBUG 0
#define LOG_TAG "CFSurface"

#ifdef __GNUC__
#define	__GNUC_PREREQ(x, y)	\
    ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||	\
     (__GNUC__ > (x)))
#else
#define	__GNUC_PREREQ(x, y)	0
#endif

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <utils/misc.h>
#include <signal.h>

#include <binder/IPCThreadState.h>
#include <utils/Log.h>

#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <GLES2/gl2.h>
#include <EGL/eglext.h>

#include <dlfcn.h>

#include "CFSurface.h"

namespace android {

// ---------------------------------------------------------------------------

// _ZN7android15ComposerService18getComposerServiceEv
typedef sp<ISurfaceComposer>(*ComposerService_getComposer)();

sp<ISurfaceComposer> CFSurface::getComposer() {
    // needed starting API 26
    ComposerService_getComposer getComposer;
    *(void **)(&getComposer) = dlsym(RTLD_DEFAULT, "_ZN7android15ComposerService18getComposerServiceEv");
    if (getComposer != NULL) {
        fprintf(stderr, "XAPI: ComposerService::getComposerService()\n");
        return (*getComposer)();
    }
    return NULL;
};

class SurfaceComposerClientProxy
{
public:
    // API pre-26 :: _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij
    typedef sp<SurfaceControl>(SurfaceComposerClientProxy::*createSurface)(
            const String8& name,// name of the surface
            uint32_t w,         // width in pixel
            uint32_t h,         // height in pixel
            PixelFormat format, // pixel-format desired
            uint32_t flags      // usage flags
    );

    // API 26 :: _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEjj
    typedef sp<SurfaceControl>(SurfaceComposerClientProxy::*createSurface26)(
            const String8& name,// name of the surface
            uint32_t w,         // width in pixel
            uint32_t h,         // height in pixel
            PixelFormat format, // pixel-format desired
            uint32_t flags    , // usage flags
            SurfaceControl* parent, // parent
            uint32_t windowType, // from WindowManager.java (STATUS_BAR, INPUT_METHOD, etc.)
            uint32_t ownerUid   // UID of the task
    );

    // API 28 :: _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEii
    typedef sp<SurfaceControl>(SurfaceComposerClientProxy::*createSurface28)(
            const String8& name,// name of the surface
            uint32_t w,         // width in pixel
            uint32_t h,         // height in pixel
            PixelFormat format, // pixel-format desired
            uint32_t flags    , // usage flags
            SurfaceControl* parent, // parent
            int32_t windowType, // from WindowManager.java (STATUS_BAR, INPUT_METHOD, etc.)
            int32_t ownerUid   // UID of the task
    );

    // API pre-28 :: static _ZN7android21SurfaceComposerClient21openGlobalTransactionEv
    typedef void (SurfaceComposerClientProxy::*openGlobalTransaction)();

    // API pre-28 :: static _ZN7android21SurfaceComposerClient22closeGlobalTransactionEb
    typedef void (SurfaceComposerClientProxy::*closeGlobalTransaction)(bool synchronous);
};

class SurfaceControlProxy
{
public:
    // API pre-25 :: _ZN7android14SurfaceControl8setLayerEi
    typedef status_t(SurfaceControlProxy::*setLayer)(int32_t layer);

    // API 25 :: _ZN7android14SurfaceControl8setLayerEj
    typedef status_t(SurfaceControlProxy::*setLayer25)(uint32_t layer);

    // API pre-28 :: _ZN7android14SurfaceControl4showEv
    typedef status_t(SurfaceControlProxy::*show)();

    // API pre-28 :: _ZN7android14SurfaceControl4hideEv
    typedef status_t(SurfaceControlProxy::*hide)();

    // API 28 :: _ZN7android14SurfaceControl20writeSurfaceToParcelERKNS_2spIS0_EEPNS_6ParcelE
    typedef status_t(SurfaceControlProxy::*writeSurfaceToParcel)(const sp<SurfaceControl>& control, Parcel* parcel);
};

// different signatures API pre-25 and 25 (26 may be same as pre-25?), different again in 28 (see further down)
static SurfaceControlProxy::setLayer setLayerOrg;
static SurfaceControlProxy::setLayer25 setLayer25;

// different signatures API pre-26, 26, and 28
static SurfaceComposerClientProxy::createSurface createSurfaceOrg;
static SurfaceComposerClientProxy::createSurface26 createSurface26;
static SurfaceComposerClientProxy::createSurface28 createSurface28;

// surface show/hide API pre-28
static SurfaceControlProxy::show show;
static SurfaceControlProxy::hide hide;

// transaction handling API pre-28
static SurfaceComposerClientProxy::openGlobalTransaction openGlobalTransaction;
static SurfaceComposerClientProxy::closeGlobalTransaction closeGlobalTransaction;

CFSurface::CFSurface(JavaVM* vm, jobject javaInstance, jmethodID onSize, jmethodID onDrawFrame, jmethodID onWantVisible, jmethodID onSetSurfaceControl, jmethodID onSetSurfaceLayer, jmethodID onSetSurfaceVisible) : Thread(true)
{
    // this is needed on Oreo or 'new SurfaceComposerClient' causes a crash
    sp<ISurfaceComposer> svc = getComposer();

    mSession = new SurfaceComposerClient();
    mVM = vm;
    mJavaInstance = javaInstance;
    mOnDrawFrame = onDrawFrame;
    mOnSize = onSize;
    mOnWantVisible = onWantVisible;
    mOnSetSurfaceControl = onSetSurfaceControl;
    mOnSetSurfaceLayer = onSetSurfaceLayer;
    mOnSetSurfaceVisible = onSetSurfaceVisible;
}

CFSurface::~CFSurface() {
}

void CFSurface::onFirstRef() {
    status_t err = mSession->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));
    if (err == NO_ERROR) {
        run("CFSurface", PRIORITY_DISPLAY, 1024);
    }
}

sp<SurfaceComposerClient> CFSurface::session() const {
    return mSession;
}

void CFSurface::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
}

void CFSurface::initImports() {
    void* handle = dlopen("libgui.so", RTLD_LAZY);
    if (handle != NULL) {
        *(void **)(&setLayerOrg) = dlsym(handle, "_ZN7android14SurfaceControl8setLayerEi");
        *(void **)(&setLayer25) = dlsym(handle, "_ZN7android14SurfaceControl8setLayerEj");
        *(void **)(&createSurfaceOrg) = dlsym(handle, "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij");
        *(void **)(&createSurface26) = dlsym(handle, "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEjj");
        *(void **)(&createSurface28) = dlsym(handle, "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEii");
        *(void **)(&show) = dlsym(handle, "_ZN7android14SurfaceControl4showEv");
        *(void **)(&hide) = dlsym(handle, "_ZN7android14SurfaceControl4hideEv");
        *(void **)(&openGlobalTransaction) = dlsym(handle, "_ZN7android21SurfaceComposerClient21openGlobalTransactionEv");
        *(void **)(&closeGlobalTransaction) = dlsym(handle, "_ZN7android21SurfaceComposerClient22closeGlobalTransactionEb");
        dlclose(handle);
    }
}

sp<SurfaceControl> CFSurface::createSurface(int w, int h) {
    SurfaceComposerClient *scc = session().get();
    if (createSurfaceOrg != NULL) {
        fprintf(stderr, "XAPI: SurfaceComposerClient->createSurface()\n");
        return (((SurfaceComposerClientProxy *) scc)->*createSurfaceOrg)(String8("CFSurface"), w, h, PIXEL_FORMAT_RGB_565, 0);
    } else if (createSurface26 != NULL) {
        fprintf(stderr, "XAPI: SurfaceComposerClient->createSurface26()\n");
        return (((SurfaceComposerClientProxy *) scc)->*createSurface26)(String8("CFSurface"), w, h, PIXEL_FORMAT_RGB_565, 0, NULL, 0, 0);
    } else if (createSurface28 != NULL) {
        fprintf(stderr, "XAPI: SurfaceComposerClient->createSurface28()\n");
        return (((SurfaceComposerClientProxy *) scc)->*createSurface28)(String8("CFSurface"), w, h, PIXEL_FORMAT_RGB_565, 0, NULL, -1, -1);
    } else {
        fprintf(stderr, "XAPI: SurfaceComposerClient->createSurfaceNULL()\n");
        return NULL;
    }
};

status_t CFSurface::setLayer(sp<SurfaceControl> control, int32_t layer, JNIEnv *env) {
    if (mJavaSurfaceControl > 0) {
        env->CallVoidMethod(mJavaInstance, mOnSetSurfaceLayer, layer);
        return 0;
    } else {
        // API pre-28
        if ((openGlobalTransaction == NULL) || (closeGlobalTransaction == NULL)) {
            fprintf(stderr, "XAPI: SurfaceComposerClient->(open/close)GlobalTransactionNULL()\n");
            return NO_INIT;
        }

        status_t ret;
        SurfaceControl *sc = control.get();
        fprintf(stderr, "XAPI: SurfaceComposerClient->openGlobalTransaction()\n");
        (((SurfaceComposerClientProxy *) NULL)->*openGlobalTransaction)();
        if (setLayerOrg != NULL) {
            fprintf(stderr, "XAPI: SurfaceControl->setLayer()\n");
            ret = (((SurfaceControlProxy *) sc)->*setLayerOrg)(0x7FFFFFFF);
        } else if (setLayer25 != NULL) {
            fprintf(stderr, "XAPI: SurfaceControl->setLayer25()\n");
            ret = (((SurfaceControlProxy *) sc)->*setLayer25)(0x7FFFFFFF);
        } else {
            fprintf(stderr, "XAPI: SurfaceControl->setLayerNULL()\n");
            ret = NO_INIT;
        }
        fprintf(stderr, "XAPI: SurfaceComposerClient->closeGlobalTransaction()\n");
        (((SurfaceComposerClientProxy *) NULL)->*closeGlobalTransaction)(false);
        return ret;
    }
};

void CFSurface::setVisible(bool visible, JNIEnv *env) {
    if (mJavaSurfaceControl > 0) {
        env->CallVoidMethod(mJavaInstance, mOnSetSurfaceVisible, visible ? 1 : 0);
    } else {
        // API pre-28
        if ((openGlobalTransaction == NULL) || (closeGlobalTransaction == NULL)) {
            fprintf(stderr, "XAPI: SurfaceComposerClient->(open/close)GlobalTransactionNULL()\n");
            return;
        }

        SurfaceControl *sc = mFlingerSurfaceControl.get();
        fprintf(stderr, "XAPI: SurfaceComposerClient->openGlobalTransaction()\n");
        (((SurfaceComposerClientProxy *) NULL)->*openGlobalTransaction)();
        if (visible) {
            if (show != NULL) {
                fprintf(stderr, "XAPI: SurfaceControl->show()\n");
                (((SurfaceControlProxy *) sc)->*show)();
            } else {
                fprintf(stderr, "XAPI: SurfaceControl->showNULL()\n");
            }
        } else {
            if (hide != NULL) {
                fprintf(stderr, "XAPI: SurfaceControl->hide()\n");
                (((SurfaceControlProxy *) sc)->*hide)();
            } else {
                fprintf(stderr, "XAPI: SurfaceControl->hideNULL()\n");
            }
        }
        fprintf(stderr, "XAPI: SurfaceComposerClient->closeGlobalTransaction()\n");
        (((SurfaceComposerClientProxy *) NULL)->*closeGlobalTransaction)(false);
    }
}

status_t CFSurface::readyToRun() {
    initImports();

    return NO_ERROR;
}

bool CFSurface::threadLoop()
{
    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status)
        return 0;

    // see if Java can create the surface
    mJavaSurfaceControl = env->CallLongMethod(mJavaInstance, mOnSetSurfaceControl, dinfo.w, dinfo.h);

    sp<SurfaceControl> control = NULL;
    sp<Surface> s = NULL;

    if (mJavaSurfaceControl > 0) {
        control = reinterpret_cast<SurfaceControl *>(mJavaSurfaceControl);
    } else {
        // on older API's, create the native surface ourselves
        // we can handle up to API 27 perfectly, but for API 28 setLayer and setVisible will not work
        // in that case it mostly still works during normal Android operation, but it will not
        // overlay the bootanimation, and the surface will always be in visible state
        control = createSurface(dinfo.w, dinfo.h);
    }

    // retrieve the surface
    s = control->getSurface();

    // initialize opengl and egl
    const EGLint attribs[] = {
            EGL_RED_SIZE,   		8,
            EGL_GREEN_SIZE, 		8,
            EGL_BLUE_SIZE,  		8,
            EGL_ALPHA_SIZE, 		8,
            EGL_DEPTH_SIZE, 		0,
            EGL_RENDERABLE_TYPE, 	EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };
    EGLint w, h, dummy;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    surface = eglCreateWindowSurface(display, config, s.get(), NULL);
    EGLint surfaceAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, NULL, surfaceAttribs);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        return 0;

    mDisplay = display;
    mContext = context;
    mSurface = surface;
    mWidth = w;
    mHeight = h;
    mFlingerSurfaceControl = control;
    mFlingerSurface = s;

    setLayer(mFlingerSurfaceControl, 0x7FFFFFFF, env);

    env->CallVoidMethod(mJavaInstance, mOnSize, mWidth, mHeight);

    int visible = 0; // natively created surface start visible, Java created hidden
    while (!exitPending()) {
        int wantVisible = env->CallIntMethod(mJavaInstance, mOnWantVisible);
        if (visible != wantVisible) {
            setVisible(wantVisible, env);
            visible = wantVisible;
        }
        if (visible) {
            env->CallVoidMethod(mJavaInstance, mOnDrawFrame);
            eglSwapBuffers(mDisplay, mSurface);
        }
    }

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    eglDestroySurface(mDisplay, mSurface);
    mFlingerSurface.clear();
    mFlingerSurfaceControl.clear();
    eglTerminate(mDisplay);
    //IPCThreadState::self()->stopProcess();
    mVM->DetachCurrentThread();
    return 0;
}

// ---------------------------------------------------------------------------

}
; // namespace android
