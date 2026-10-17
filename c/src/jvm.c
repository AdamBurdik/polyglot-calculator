#include "jvm.h"

int cache_java_methods(JNIEnv *env) {
    jclass local_cls = (*env)->FindClass(env, "Main");
    if (local_cls == NULL) return 1;

    g_main_cls = (*env)->NewGlobalRef(env, local_cls);
    (*env)->DeleteLocalRef(env, local_cls); // Clean up local ref
    if (g_main_cls == NULL) return 1;

    g_multiple_mid = (*env)->GetStaticMethodID(env, g_main_cls, "multiple", "(DD)D");
    if (g_multiple_mid == NULL) return 1;

    return 0;
}


int init_jvm() {
  printf("Initializing JVM\n");

  JNIEnv *env;
  JavaVMInitArgs vm_args;
  JavaVMOption options[1];

  options[0].optionString = "-Djava.class.path=.";

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

  cache_java_methods(env);
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
