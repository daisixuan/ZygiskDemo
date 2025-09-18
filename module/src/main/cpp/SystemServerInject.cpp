#include <jni.h>
#include "log.h"

jobject getSystemContext(JNIEnv *env) {
    // 在system_server进程中，可以直接通过ActivityThread.systemMain()获取
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) return nullptr;

    // 获取currentActivityThread
    jmethodID currentMethod = env->GetStaticMethodID(activityThreadClass,
                                                     "currentActivityThread",
                                                     "()Landroid/app/ActivityThread;");
    jobject currentThread = env->CallStaticObjectMethod(activityThreadClass, currentMethod);

    // 获取getSystemContext
    jmethodID getContextMethod = env->GetMethodID(activityThreadClass,
                                                  "getSystemContext",
                                                  "()Landroid/app/ContextImpl;");
    jobject context = env->CallObjectMethod(currentThread, getContextMethod);

    env->DeleteLocalRef(activityThreadClass);
    env->DeleteLocalRef(currentThread);

    return context;
}

void callCustomMethod(JNIEnv *env, jobject classLoader) {
    // 1. 获取ClassLoader的loadClass方法
    jclass classLoaderClass = env->GetObjectClass(classLoader);
    jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass",
                                                 "(Ljava/lang/String;)Ljava/lang/Class;");

    // 2. 加载自定义类
    jstring className = env->NewStringUTF("com/example/mysystemservice/ServiceInjector");
    jclass customClass = (jclass)env->CallObjectMethod(classLoader, loadClassMethod, className);

    if (!customClass) {
        LOGE("加载自定义类失败");
        return;
    }
    LOGI("加载自定义类成功 开始获取injectService方法");
    // 3. 获取静态方法
    jmethodID injectServiceMethod = env->GetStaticMethodID(customClass, "injectService",
                                                           "(Landroid/content/Context;)Z"); // 根据实际方法签名调整

    if (!injectServiceMethod) {
        LOGE("找不到injectService静态方法");
        return;
    }
    LOGI("加载injectService成功 开始getSystemContext");
    // 4. 获取Context对象 (通常在system_server中可以通过ActivityManagerService获取)
//    jobject context = getSystemContext(env); // 需要实现这个方法获取系统Context
//
//    if (!context) {
//        LOGE("获取Context失败");
//        return;
//    }
    LOGI("getSystemContext成功 开始调用injectService");

    // 5. 调用injectService静态方法
    jboolean result = env->CallStaticBooleanMethod(customClass, injectServiceMethod, nullptr);

    if (result) {
        LOGI("injectService调用成功");
    } else {
        LOGE("injectService返回false");
    }

    // 清理引用
    env->DeleteLocalRef(className);
    env->DeleteLocalRef(customClass);
    env->DeleteLocalRef(classLoaderClass);
}

void loadCustomJar(JNIEnv *env) {
    // 1. 获取PathClassLoader类
    jclass pathClassLoaderClass = env->FindClass("dalvik/system/PathClassLoader");
    if (!pathClassLoaderClass) {
        LOGE("找不到PathClassLoader类");
        return;
    }

    // 2. 获取PathClassLoader构造函数
    jmethodID constructorId = env->GetMethodID(pathClassLoaderClass, "<init>",
                                               "(Ljava/lang/String;Ljava/lang/ClassLoader;)V");

    // 3. 创建JAR路径字符串
    jstring jarPath = env->NewStringUTF("/system/framework/mysystemservice.jar");

    // 4. 获取系统ClassLoader
    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID getSystemClassLoaderMethod = env->GetStaticMethodID(classLoaderClass,
                                                                  "getSystemClassLoader",
                                                                  "()Ljava/lang/ClassLoader;");
    jobject systemClassLoader = env->CallStaticObjectMethod(classLoaderClass,
                                                            getSystemClassLoaderMethod);

    // 5. 创建PathClassLoader实例
    jobject pathClassLoader = env->NewObject(pathClassLoaderClass, constructorId,
                                             jarPath, systemClassLoader);

    if (!pathClassLoader) {
        LOGE("创建PathClassLoader失败");
        return;
    }else{
        LOGI("创建PathClassLoader成功");
    }

    // 6. 加载自定义类并调用静态方法
    callCustomMethod(env, pathClassLoader);

    // 清理引用
    env->DeleteLocalRef(jarPath);
    env->DeleteLocalRef(pathClassLoader);
    env->DeleteLocalRef(systemClassLoader);
    env->DeleteLocalRef(pathClassLoaderClass);
    env->DeleteLocalRef(classLoaderClass);
}




