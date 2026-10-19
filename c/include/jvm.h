#include <jni.h>

extern JavaVM *g_jvm;

extern jclass g_java_cls;
extern jmethodID g_java_mid;

extern jclass g_kotlin_cls;
extern jmethodID g_kotlin_mid;

int cache_java_methods(JNIEnv *env);
int cache_kotlin_methods(JNIEnv *env);
int init_jvm();
void destroy_jvm();
JNIEnv *get_current_env(void);
