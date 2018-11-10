#define LOG_TAG "CFSurface"

#ifdef __GNUC__
#define	__GNUC_PREREQ(x, y)	\
    ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||	\
     (__GNUC__ > (x)))
#else
#define	__GNUC_PREREQ(x, y)	0
#endif

#include <stdint.h>
#include <inttypes.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <utils/threads.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sys/resource.h>
#endif

#include "CFSurface.h"
#include "jni.h"

using namespace android;

// ---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

static sp<CFSurface> cfsurface = NULL;

JavaVM *vm = NULL;
jclass classGraphicsHost = NULL;
jmethodID methodOnSize = 0;
jmethodID methodOnDrawFrame = 0;
jmethodID methodOnWantVisible = 0;
jmethodID methodOnSetSurfaceControl = 0;
jmethodID methodOnSetSurfaceLayer = 0;
jmethodID methodOnSetSurfaceVisible = 0;

//int main(int argc, char** argv)
JNIEXPORT void JNICALL Java_eu_chainfire_libcfsurface_SurfaceHost_nativeInit(JNIEnv * env, jobject clazz)
{
#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
#endif

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    sp<IServiceManager> sm = defaultServiceManager();
    const String16 name("SurfaceFlinger");
    const int SERVICE_WAIT_SLEEP_MS = 100;
    const int LOG_PER_RETRIES = 1;
    int retry = 0;
    while (sm->checkService(name) == NULL) {
        retry++;
        if ((retry % LOG_PER_RETRIES) == 0) fprintf(stderr, "(%d) Waiting for SurfaceFlinger...", retry);
        usleep(SERVICE_WAIT_SLEEP_MS * 1000);
    };

    cfsurface = new CFSurface(vm, env->NewGlobalRef(clazz), methodOnSize, methodOnDrawFrame, methodOnWantVisible, methodOnSetSurfaceControl, methodOnSetSurfaceLayer, methodOnSetSurfaceVisible);

    //IPCThreadState::self()->joinThreadPool();

    //return 0;
}

JNIEXPORT void JNICALL Java_eu_chainfire_libcfsurface_SurfaceHost_nativeDone(JNIEnv *, jobject) {
    if (cfsurface != NULL) {
        cfsurface->requestExit();
        cfsurface = NULL;
    }
}

#define MARKER ":/librootjava"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    // fix LD_LIBRARY_PATH
    char* ldLibraryPath = getenv("LD_LIBRARY_PATH");
    if (ldLibraryPath != NULL) {
        char* libRootJava = strstr(ldLibraryPath, MARKER);
        if (libRootJava != NULL) {
            int MARKER_LEN = strlen(MARKER);
            if (strlen(libRootJava) == MARKER_LEN) {
                unsetenv("LD_LIBRARY_PATH");
            } else {
                setenv("LD_LIBRARY_PATH", &libRootJava[MARKER_LEN], 1);
            }
        }
    }

    vm = jvm;

    JNIEnv *env;
    vm->GetEnv((void**)&env, JNI_VERSION_1_4);

    classGraphicsHost = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("eu/chainfire/libcfsurface/SurfaceHost")));
    methodOnSize = env->GetMethodID(classGraphicsHost, "informSize", "(II)V");
    methodOnDrawFrame = env->GetMethodID(classGraphicsHost, "doDrawFrame", "()V");
    methodOnWantVisible = env->GetMethodID(classGraphicsHost, "onWantVisible", "()I");
    methodOnSetSurfaceControl = env->GetMethodID(classGraphicsHost, "onSetSurfaceControl", "(II)J");
    methodOnSetSurfaceLayer = env->GetMethodID(classGraphicsHost, "onSetSurfaceLayer", "(I)V");
    methodOnSetSurfaceVisible = env->GetMethodID(classGraphicsHost, "onSetSurfaceVisible", "(I)V");
    return JNI_VERSION_1_4;
}

#ifdef __cplusplus
}
#endif

