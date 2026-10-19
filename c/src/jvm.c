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


static int cache_static_method(JNIEnv *env, const char *class_name, const char *method_name, const char *signature, jclass *out_cls, jmethodID *out_mid) {
    jclass local_cls = (*env)->FindClass(env, class_name);
    if (local_cls == NULL) {
        fprintf(stderr, "Failed to find Java class '%s'\n", class_name);
        (*env)->ExceptionDescribe(env); // Prints exact JVM error to stderr
        (*env)->ExceptionClear(env);    // Clear the exception
        return 1;
    }
    *out_cls = (*env)->NewGlobalRef(env, local_cls);
    (*env)->DeleteLocalRef(env, local_cls);
    if (*out_cls == NULL) {
        fprintf(stderr, "Failed to cache Java class '%s'\n", class_name);
        return 1;
    }

    *out_mid = (*env)->GetStaticMethodID(env, *out_cls, method_name, signature);
    if (*out_mid == NULL) {
        fprintf(stderr, "Failed to find method '%s' in '%s'\n", method_name, class_name);
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 1;
    }
    return 0;
}

int cache_all_methods(JNIEnv *env) {
    int rc = 0;
    rc |= cache_static_method(env, "Main", "multiple", "(DD)D", &g_java_cls, &g_java_mid);
    rc |= cache_static_method(env, "KotlinMain", "sin", "(D)D", &g_kotlin_cls, &g_kotlin_mid);
    rc |= cache_static_method(env, "GroovyMain", "cos", "(D)D", &g_groovy_cls, &g_groovy_mid);
    rc |= cache_static_method(env, "ScalaMain", "tan", "(D)D", &g_scala_cls, &g_scala_mid);
    return rc;
}

int init_jvm() {
  printf("Initializing JVM\n");

  JNIEnv *env;
  JavaVMInitArgs vm_args;
  JavaVMOption options[1];
  static char classpath[PATH_MAX * 4 + 64];

  snprintf(classpath, sizeof(classpath), "-Djava.class.path=%s:%s/groovy.jar:%s/groovy-callsite.jar", jvm_exe_dir(), jvm_exe_dir(), jvm_exe_dir());
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
