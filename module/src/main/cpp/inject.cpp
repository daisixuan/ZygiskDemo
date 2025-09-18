//
// Created by jiyulan on 2025/8/31.
//
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string.h>
#include <thread>
#include <dirent.h>
#include <regex>
#include <sys/stat.h>
#include <jni.h>

#include "newriruhide.h"
#include "xdl.h"
#include "log.h"


#include "inject.h"
void load_so_file_standard(const std::string& so_path){
    void* handle = xdl_open(so_path.c_str(), 1);
    if (handle) {
        LOGI("%s loaded by standard", so_path.c_str());
    } else {
        LOGE("%s failed to load by standard", so_path.c_str());
    }
}

void load_so_file_riru(const std::string& so_path){
    void* handle = xdl_open(so_path.c_str(), 1);
    if (handle) {
        LOGI("%s loaded by riru", so_path.c_str());
        riru_hide(so_path.c_str());
        LOGI("Applied riru_hide to: %s", so_path.c_str());
    } else {
        LOGE("%s failed to load by riru", so_path.c_str());
    }
}



void injection_thread(const char* target_package_name, const std::string& lib_dir, const std::string& frida_gadget_name, const std::string& frida_config_name, const std::string& load_mode, uint time_to_sleep, JavaVM *vm) {
    LOGD("injection thread start for %s, gadget name: %s, load_mode: %s, usleep: %d", target_package_name, frida_gadget_name.c_str(), load_mode.c_str(), time_to_sleep);
    usleep(time_to_sleep);

    // 修改路径为安装目录下的 lib/arm64 目录
    std::string gadget_path = lib_dir + frida_gadget_name;
    std::string config_path = lib_dir + frida_config_name;

    std::ifstream gadget_file(gadget_path);
    if (gadget_file) {
        LOGD("Gadget is ready to load from %s", gadget_path.c_str());
    } else {
        LOGD("Cannot find gadget in %s", gadget_path.c_str());
        return;
    }

    if (load_mode == "Riru"){
        // todo riru hide app会崩溃
        load_so_file_riru(gadget_path);
    }else{
        load_so_file_standard(gadget_path);
        // 如果有权限，可以尝试删除文件
        if (unlink(gadget_path.c_str()) == 0) {
            LOGD("Deleted gadget file: %s", gadget_path.c_str());
        } else {
            LOGD("Failed to delete gadget file: %s", gadget_path.c_str());
        }

        if (unlink(config_path.c_str()) == 0) {
            LOGD("Deleted config file: %s", config_path.c_str());
        } else {
            LOGD("Failed to delete config file: %s", config_path.c_str());
        }
    }
    // 获取当前线程的JNIEnv
//    JNIEnv *env;
//    jint result = vm->AttachCurrentThread(&env, NULL);
//    if (result == JNI_OK) {
//        // 现在可以使用env了
//        // 使用完毕后要分离线程
//        vm->DetachCurrentThread();
//    }else{
//        LOGE("Failed to attach current thread: %d get JNIEnv", result);
//    }

}


void inject_prepare(const char* target_package_name, const std::string& lib_dir, const std::string& frida_gadget_name, const std::string& frida_config_name, const std::string& load_mode, uint delay, JavaVM *vm) {
    LOGD("inject_prepare start for %s, gadget name: %s, load_mode: %s, usleep: %d", target_package_name, frida_gadget_name.c_str(), load_mode.c_str(), delay);

    std::thread hack_thread(injection_thread, target_package_name, lib_dir, frida_gadget_name,
                            frida_config_name, load_mode, delay, vm);
    hack_thread.join();
}