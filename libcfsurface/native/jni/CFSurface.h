#ifndef _CFSURFACE_H
#define _CFSURFACE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Thread.h>

#include <EGL/egl.h>

#include "jni.h"

#include <gui/ISurfaceComposer.h>

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class CFSurface : public Thread, public IBinder::DeathRecipient
{
public:
    CFSurface(JavaVM* vm, jobject javaInstance, jmethodID onSize, jmethodID onDrawFrame, jmethodID onWantVisible, jmethodID onSetSurfaceControl, jmethodID onSetSurfaceLayer, jmethodID onSetSurfaceVisible);
    virtual     ~CFSurface();

    sp<SurfaceComposerClient> session() const;

private:
    virtual bool            threadLoop();
    virtual status_t        readyToRun();
    virtual void            onFirstRef();
    virtual void            binderDied(const wp<IBinder>& who);

    sp<SurfaceComposerClient>       mSession;
    JavaVM      *mVM;
    jobject     mJavaInstance;
    jmethodID   mOnSize;
    jmethodID   mOnDrawFrame;
    jmethodID   mOnWantVisible;
    jmethodID   mOnSetSurfaceControl;
    jmethodID   mOnSetSurfaceLayer;
    jmethodID   mOnSetSurfaceVisible;
    jlong       mJavaSurfaceControl;
    int         mWidth;
    int         mHeight;
    EGLDisplay  mDisplay;
    EGLDisplay  mContext;
    EGLDisplay  mSurface;
    sp<SurfaceControl> mFlingerSurfaceControl;
    sp<Surface> mFlingerSurface;

    // cross-API-level work-arounds
    void                    initImports();
    void                    setVisible(bool visible, JNIEnv *env);
    sp<SurfaceControl>      createSurface(int w, int h);
    status_t                setLayer(sp<SurfaceControl> control, int32_t layer, JNIEnv *env);
    sp<ISurfaceComposer>    getComposer();
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // _CFSURFACE_H
