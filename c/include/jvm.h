#include <jni.h>

extern JavaVM *g_jvm;

extern jclass g_java_cls;
extern jmethodID g_java_mid;

extern jclass g_kotlin_cls;
extern jmethodID g_kotlin_mid;

extern jclass g_groovy_cls;
extern jmethodID g_groovy_mid;

extern jclass g_scala_cls;
extern jmethodID g_scala_mid;

int cache_all_methods(JNIEnv *env);
int init_jvm();
void destroy_jvm();
JNIEnv *get_current_env(void);
