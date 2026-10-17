#include <jni.h>

extern JavaVM *g_jvm;
extern jclass g_main_cls;
extern jmethodID g_multiple_mid;

int cache_java_methods(JNIEnv *env);
int init_jvm();
void destroy_jvm();
JNIEnv *get_current_env(void);
