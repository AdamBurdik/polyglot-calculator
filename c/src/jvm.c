#include "jvm.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *jvm_exe_dir(void) {
  static char dir[PATH_MAX];
  static int ready;
  if (!ready) {
    char path[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n > 0) {
      char *slash;
      path[n] = '\0';
      slash = strrchr(path, '/');
      if (slash) *slash = '\0';
      snprintf(dir, sizeof(dir), "%s", path);
    } else {
      snprintf(dir, sizeof(dir), ".");
    }
    ready = 1;
  }
  return dir;
}


int cache_all_methods(JNIEnv *env) {
    // 1. Load Java class (Main.class)
    jclass local_java = (*env)->FindClass(env, "Main");
    if (local_java == NULL) {
        fprintf(stderr, "Failed to find Java class 'Main'\n");
        (*env)->ExceptionDescribe(env); // Prints exact JVM error to stderr
        (*env)->ExceptionClear(env);    // Clear the exception
        return 1;
    }
    g_java_cls = (*env)->NewGlobalRef(env, local_java);
    (*env)->DeleteLocalRef(env, local_java);
    
    g_java_mid = (*env)->GetStaticMethodID(env, g_java_cls, "multiple", "(DD)D");
    if (g_java_mid == NULL) {
        fprintf(stderr, "Failed to find method 'multiple'\n");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 1;
    }

    // 2. Load Kotlin class (MainKt.class)
    jclass local_kotlin = (*env)->FindClass(env, "KotlinMain");
    if (local_kotlin == NULL) {
        fprintf(stderr, "Failed to find Kotlin class 'KotlinMain'\n");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 1;
    }
    g_kotlin_cls = (*env)->NewGlobalRef(env, local_kotlin);
    (*env)->DeleteLocalRef(env, local_kotlin);
    
    g_kotlin_mid = (*env)->GetStaticMethodID(env, g_kotlin_cls, "sin", "(D)D");
    if (g_kotlin_mid == NULL) {
        fprintf(stderr, "Failed to find method 'sin' in MainKt\n");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 1;
    }

    return 0;
}

int init_jvm() {
  printf("Initializing JVM\n");

  JNIEnv *env;
  JavaVMInitArgs vm_args;
  JavaVMOption options[1];
  static char classpath[PATH_MAX + 32];

  snprintf(classpath, sizeof(classpath), "-Djava.class.path=%s", jvm_exe_dir());
  options[0].optionString = classpath;

  vm_args.version = JNI_VERSION_10; // JNI version for Java 25
  vm_args.nOptions = 1;
  vm_args.options = options;
  vm_args.ignoreUnrecognized = JNI_FALSE;

  // Init JVM
  long status = JNI_CreateJavaVM(&g_jvm, (void**)&env, &vm_args);
  if (status < 0 || !env) {
    fprintf(stderr, "Failed to launch Java 25 JVM\n");
    return 1;
  }

  if (cache_all_methods(env) != 0) {
    fprintf(stderr, "Warning: could not cache JVM methods (classpath: %s)\n", classpath);
  }
  return 0;
}

void destroy_jvm() {
  // Shutdown the JVM
  jint destroy_status = (*g_jvm)->DestroyJavaVM(g_jvm);
  if (destroy_status < 0) {
      fprintf(stderr, "Warning: Failed to destroy Java VM\n");
  }
}

JNIEnv *get_current_env(void) {
  JNIEnv *env = NULL;
  jint res = (*g_jvm)->GetEnv(g_jvm, (void**)&env, JNI_VERSION_10);
  
  if (res == JNI_EDETACHED) {
      (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&env, NULL);
  }
  
  return env;
}
