#include <unistd.h>
// #include <stdio.h>
// #include <fcntl.h>
// #include <cstdlib>
// #include <cstring>
// #include <dlfcn.h>
// #include <errno.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <thread>
#include <v8.h>
// #include <dirent.h>
#ifdef ANDROID
#include <android/log.h>
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android_native_app_glue.h>
#endif
#ifdef LUMIN
#include <ml_logging.h>
#include <ml_lifecycle.h>
#include <ml_privileges.h>
#endif

using namespace v8;

namespace node {
  extern std::map<std::string, std::pair<void *, bool>> dlibs;
  int Start(int argc, char* argv[]);
}

#include "build/libexokit/dlibs.h"

namespace node {
  extern std::map<std::string, std::pair<void *, bool>> dlibs;
  int Start(int argc, char* argv[]);
}

#ifdef ANDROID

struct android_app *androidApp;
JNIEnv *androidJniEnv;
std::vector<std::string> androidArgs;

typedef struct AssetStatStruct {
  char name[256];
  uint32_t key;
  uint32_t type;
  uint32_t parentKey;
  size_t size;
} AssetStat;

extern "C" {
void initAssetManager(AAssetManager *am);
}

JNIEnv *jniGetEnv(JavaVM *vm) {
  JNIEnv *env;
  if (vm == 0) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "Invalid global Java VM");
    return 0;
  }

  int status;
  status = vm->GetEnv((void **) &env, JNI_VERSION_1_6);
  if (status < 0) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "Failed to get JNI environment, trying to attach thread");
    // Try to attach native thread to JVM:
    status = vm->AttachCurrentThread(&env, 0);
    if (status < 0) {
      __android_log_print(ANDROID_LOG_ERROR, "exokit", "Failed to attach current thread to JVM");
      return 0;
    }
  }

  return env;
}
jobject jniGetContext(JNIEnv *env) {

  jclass activityThread = env->FindClass("android/app/ActivityThread");
  jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread",
                                                           "()Landroid/app/ActivityThread;");
  jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);

  jmethodID getApplication = env->GetMethodID(activityThread, "getApplication",
                                              "()Landroid/app/Application;");
  jobject context = env->CallObjectMethod(at, getApplication);
  return context;
}


void jniOnload(JavaVM *vm) {
  __android_log_print(ANDROID_LOG_INFO, "exokit", "on load 1");

  // Uses Java VM and application context to obtain Java asset manager
  // and then uses NDK AAssetManager_fromJava()
  // Note: In Android emulator, this seems to only work in main thread!
  JNIEnv *env = jniGetEnv(vm);
  __android_log_print(ANDROID_LOG_ERROR, "exokit", "on load 2 %lx", (unsigned long) env);
  jobject context = jniGetContext(env);
  __android_log_print(ANDROID_LOG_ERROR, "exokit", "on load 3 %lx", (unsigned long) context);
  jmethodID methodGetAssets =
      env->GetMethodID(env->GetObjectClass(context),
                       "getAssets",
                       "()Landroid/content/res/AssetManager;");
  if (methodGetAssets == 0) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "Could not get getAssets method");
    return;
  }
  jobject localAssetManager =
      env->CallObjectMethod(context,
                            methodGetAssets);
  if (localAssetManager == 0) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "Could not get local Java Asset Manager");
    return;
  }
  jobject globalAssetManager = env->NewGlobalRef(localAssetManager);
  if (globalAssetManager == 0) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "Could not get global Java Asset Manager");
    return;
  }

  AAssetManager *am = AAssetManager_fromJava(env, globalAssetManager);
  __android_log_print(ANDROID_LOG_INFO, "exokit", "Got Java Asset Manager 1 %lx %lx", (unsigned long)am, (unsigned long)initAssetManager);
  initAssetManager(am);
  __android_log_print(ANDROID_LOG_INFO, "exokit", "Got Java Asset Manager 2 %lx %lx", (unsigned long)am, (unsigned long)initAssetManager);

  androidJniEnv = env;
  __android_log_print(ANDROID_LOG_INFO, "exokit", "Got JNI Env %lx", (unsigned long)androidJniEnv);

  // parse args
  // adb shell am start -n com.webmr.exokit/android.app.NativeActivity -e args "'lol zol'"
  {
    std::vector<std::string> &args = androidArgs;

    jobject activity = androidApp->activity->clazz;
    jmethodID get_intent_method = env->GetMethodID(env->GetObjectClass(activity), "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(activity, get_intent_method);
    jmethodID get_string_extra_method = env->GetMethodID(env->GetObjectClass(intent), "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
    jvalue get_string_extra_args;
    get_string_extra_args.l = env->NewStringUTF("args");
    jstring extra_str = static_cast<jstring>(env->CallObjectMethodA(intent, get_string_extra_method, &get_string_extra_args));

    std::string args_str;
    if (extra_str) {
      const char* extra_utf = env->GetStringUTFChars(extra_str, nullptr);
      args_str = extra_utf;
      env->ReleaseStringUTFChars(extra_str, extra_utf);
      env->DeleteLocalRef(extra_str);
    }

    __android_log_print(ANDROID_LOG_INFO, "exokit", "got args 1 '%s'", args_str.c_str());

    env->DeleteLocalRef(get_string_extra_args.l);
    env->DeleteLocalRef(intent);

    // split args_str
    std::stringstream ss(args_str);
    std::string arg;
    while (std::getline(ss, arg, ' ')) {
      if (!arg.empty()) {
        __android_log_print(ANDROID_LOG_INFO, "exokit", "got args 2 %lx '%s'", args.size(), arg.c_str());

        args.push_back(arg);
      }
    }
  }
}

#endif

const char *LOG_TAG = "exokit";
constexpr ssize_t STDIO_BUF_SIZE = 64 * 1024;

void pumpStdout(int fd) {
  char buf[STDIO_BUF_SIZE + 1];
  ssize_t i = 0;
  ssize_t lineStart = 0;

  for (;;) {
    ssize_t size = read(fd, buf + i, sizeof(buf) - i);

    if (size > 0) {
      for (ssize_t j = i; j < i + size; j++) {
        if (buf[j] == '\n') {
          buf[j] = 0;
          // ML_LOG_TAG(Info, LOG_TAG, "%s", buf + lineStart);
          __android_log_print(ANDROID_LOG_INFO, "exokit", "%s", buf + lineStart);

          lineStart = j + 1;
        }
      }

      i += size;

      if (i >= sizeof(buf)) {
        ssize_t lineLength = i - lineStart;
        memcpy(buf, buf + lineStart, lineLength);
        i = lineLength;
        lineStart = 0;
      }
    } else {
      if (i > 0) {
        buf[i] = 0;
        // ML_LOG_TAG(Info, LOG_TAG, "%s", buf);
        __android_log_print(ANDROID_LOG_INFO, "exokit", "%s", buf);
      }
      break;
    }
  }
}
void pumpStderr(int fd) {
  char buf[STDIO_BUF_SIZE + 1];
  ssize_t i = 0;
  ssize_t lineStart = 0;

  for (;;) {
    ssize_t size = read(fd, buf + i, sizeof(buf) - i);

    if (size > 0) {
      for (ssize_t j = i; j < i + size; j++) {
        if (buf[j] == '\n') {
          buf[j] = 0;
          // ML_LOG_TAG(Error, LOG_TAG, "%s", buf + lineStart);
          __android_log_print(ANDROID_LOG_ERROR, "exokit", "%s", buf + lineStart);

          lineStart = j + 1;
        }
      }

      i += size;

      if (i >= sizeof(buf)) {
        ssize_t lineLength = i - lineStart;
        memcpy(buf, buf + lineStart, lineLength);
        i = lineLength;
        lineStart = 0;
      }
    } else {
      if (i > 0) {
        buf[i] = 0;
        __android_log_print(ANDROID_LOG_ERROR, "exokit", "%s", buf);
        // ML_LOG_TAG(Error, LOG_TAG, "%s", buf);
      }
      break;
    }
  }
}

void android_main(struct android_app *app) {
  char cwdbuf[4096];
  char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 1 %s", cwd);

  int stdoutfds[2];
  int stderrfds[2];
  pipe(stdoutfds);
  pipe(stderrfds);

  // if (stdoutfds[1] != 1) {
  close(1);
  dup2(stdoutfds[1], 1);
  // }
  // if (stderrfds[1] != 2) {
  close(2);
  dup2(stderrfds[1], 2);
  // }

  // read stdout/stderr in threads
  int stdoutReadFd = stdoutfds[0];
  std::thread stdoutReaderThread([stdoutReadFd]() -> void { pumpStdout(stdoutReadFd); });
  int stderrReadFd = stderrfds[0];
  std::thread stderrReaderThread([stderrReadFd]() -> void { pumpStderr(stderrReadFd); });

  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 1 %lx", (unsigned long)app->activity);
  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 2 '%s'", app->activity->internalDataPath);
  int setenvResult = setenv("HOME", app->activity->internalDataPath, 1);

  // initAssetStats();
  
  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 3 %x", setenvResult);

  androidApp = app;
  jniOnload(app->activity->vm);

  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 4 %lx", (unsigned long)androidApp);

  registerDlibs(node::dlibs);

  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main cwd 5 %lx", (unsigned long)app);

  // std::cout << "test log stdout" << std::endl;

  // exout << "---------------------exokit start" << std::endl;

  /* std::atexit([]() -> void {
    // exout << "---------------------exokit end" << std::endl;
  }); */

  __android_log_print(ANDROID_LOG_ERROR, "exokit", "main 3");

  const char *nodeString = "node";
  const char *experimentalWorkerString = "--experimental-worker";
  const char *dotString = "/package";
  const char *jsString = "/package/examples/tutorial.html";

  char argsString[4096];
  int i = 0;

  char *nodeArg = argsString + i;
  strncpy(nodeArg, nodeString, sizeof(argsString) - i);
  i += strlen(nodeString) + 1;

  char *experimentalWorkerArg = argsString + i;
  strncpy(experimentalWorkerArg, experimentalWorkerString, sizeof(argsString) - i);
  i += strlen(experimentalWorkerString) + 1;

  char *dotArg = argsString + i;
  strncpy(dotArg, dotString, sizeof(argsString) - i);
  i += strlen(dotString) + 1;
  char *jsArg = argsString + i;
  strncpy(jsArg, jsString, sizeof(argsString) - i);
  i += strlen(jsString) + 1;

  char *argv[] = {nodeArg, experimentalWorkerArg, dotArg, jsArg/*, eArg, consoleArg*/};
  size_t argc = sizeof(argv) / sizeof(argv[0]);

  for (int i = 0; i < argc; i++) {
    __android_log_print(ANDROID_LOG_ERROR, "exokit", "arg %d %s", i, argv[i]);
  }

  int result = node::Start(argc, argv);

  __android_log_print(ANDROID_LOG_ERROR, "exokit", "exit %d", result);

  close(1);
  close(2);
  stdoutReaderThread.join();
  stderrReaderThread.join();
}
