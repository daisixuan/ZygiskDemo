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

#include "SystemServerInject.h"
#include "zygisk.hpp"
#include "nlohmann/json.hpp"
#include "xdl.h"
#include "log.h"
#include "inject.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

using json = nlohmann::json;

#define BUFFER_SIZE 1024


std::string getPathFromFd(int fd) {
    char buf[PATH_MAX];
    std::string fdPath = "/proc/self/fd/" + std::to_string(fd);
    ssize_t len = readlink(fdPath.c_str(), buf, sizeof(buf) - 1);
    close(fd);
    if (len != -1) {
        buf[len] = '\0';
        return {buf};
    } else {
        // Handle error
        return "";
    }
}

void writeString(int fd, const std::string& str) {
    size_t length = str.size() + 1;
    write(fd, &length, sizeof(length));
    write(fd, str.c_str(), length);
}

std::string readString(int fd) {
    size_t length;
    read(fd, &length, sizeof(length));
    std::vector<char> buffer(length);
    read(fd, buffer.data(), length);
    return {buffer.data()};
}

void copy_file(const std::string& source_path, const std::string& dest_path) {
    FILE *source_file, *dest_file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    source_file = fopen(source_path.c_str(), "rb");
    if (source_file == nullptr) {
        LOGD("Error opening source file: %s", source_path.c_str());
        exit(EXIT_FAILURE);
    }

    dest_file = fopen(dest_path.c_str(), "wb");
    if (dest_file == nullptr) {
        LOGD("Error opening destination file: %s", dest_path.c_str());
        fclose(source_file);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            LOGD("Error writing to destination file");
            fclose(source_file);
            fclose(dest_file);
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(source_file)) {
        LOGD("Error reading from source file");
    }

    fclose(source_file);
    fclose(dest_file);
}

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char* package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        const char* app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        std::string module_dir = getPathFromFd(api->getModuleDir());
        json info;
        info["module_dir"] = module_dir;
        info["data_dir"] = std::string(app_data_dir);
        info["package_name"] = std::string(package_name);
        int fd = api->connectCompanion();
        writeString(fd, info.dump());
        std::string result_str = readString(fd);
        json result = json::parse(result_str);
        if (result["code"] != 0){
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        load = true;
        frida_gadget_name = result["frida_gadget_name"];
        frida_config_name = result["frida_config_name"];
        delay = result["delay"];
        lib_dir = result["lib_dir"]; // 获取安装目录
        load_mode = result["load_mode"];
        target_package_name = strdup(package_name);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (load) {
            // 将安装目录传递给注入线程
            JavaVM *vm = nullptr;
            if (env->GetJavaVM(&vm) == JNI_OK) {
                std::thread t(inject_prepare, target_package_name, lib_dir, frida_gadget_name,
                              frida_config_name, load_mode, delay, vm);
                t.detach();
            }else {
                LOGE("Failed to get JavaVM");
            }
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        int uid = args->uid;
        int gid = args->gid;
        LOGI("preServerSpecialize for uid: %d, gid: %d", uid, gid);
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

    void postServerSpecialize(const ServerSpecializeArgs *args) override {
//        if (args->uid != 1000) return; // 确保是system用户
//        LOGI("postSystemServerSpecialize: system_server has specialized.");
//
//        if (env) {
//            loadCustomJar(env);
//        }else{
//            LOGE("postSystemServerSpecialize: JNIEnv没有");
//        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool load = false;
    char* target_package_name = nullptr;
    uint delay;
    std::string frida_gadget_name;
    std::string frida_config_name;
    std::string lib_dir; // 新增成员变量
    std::string load_mode;
    std::string dex_name;
};

// 修改后的 companion_handler 函数
static void companion_handler(int i) {
    std::string info_str = readString(i);
    json info = json::parse(info_str);
    std::string module_dir = info["module_dir"];
    std::string package_name = info["package_name"];
    std::string data_dir = info["data_dir"];
    std::string config_file_path = module_dir + std::string("/config.json");
    std::string frida_gadget_name = "libjyl.so";
    std::string frida_config_name = "libjyl.config.so";
    uint delay = 300000;
    std::string load_mode = "Standard";

    json result;
    std::ifstream file(config_file_path);
    if (!file) {
        result["code"] = 1;
        LOGD("The configuration file does not exist");
        writeString(i, result.dump());
        return;
    }

    json config;
    file >> config;
    if (!config.contains(package_name)) {
        result["code"] = 2;
        LOGD("No configuration for package: %s", package_name.c_str());
        writeString(i, result.dump());
        return;
    }

//    std::string lib_dir = GetLibDir(vms);
//    std::string lib_dir = "/data/app/~~vt-EeVuGL4J6jyPafYFLYg==/com.example.nativeproject-2-I5OvhKLZEUiYRHeCItwg==/lib/arm64";

    char buffer[256]; // Choose a size large enough for the full path
    snprintf(buffer, sizeof(buffer), "/data/data/%s/files/", package_name.c_str());
    std::string lib_dir = buffer;

//    int data_dir_fd = openat(-100, data_dir.c_str(), O_NONBLOCK);
//    int newfd = dup(data_dir_fd);
//    std::string real_data_dir = getPathFromFd(newfd);
//    LOGD("real_data_dir: %s", real_data_dir.c_str());


    if (lib_dir.empty()) {
        result["code"] = 3;
        LOGD("Failed to find lib_dir for package: %s", package_name.c_str());
        writeString(i, result.dump());
        return;
    }
    result["lib_dir"] = lib_dir;
    LOGD("lib_dir: %s", lib_dir.c_str());

    result["code"] = 0;
    json package_config = config[package_name];
    if (package_config.contains("delay")) {
        delay = package_config["delay"];
    } else {
        delay = 300000;  // Default delay if not provided
    }

    if (package_config.contains("load")) {
        load_mode = package_config["load"];
    } else {
        load_mode = "Standard"; // Default delay if not provided
    }

    // 检查并创建目录（如果需要）
    struct stat st = {0};
    if (stat(lib_dir.c_str(), &st) == -1) {
        if (mkdir(lib_dir.c_str(), 0755) != 0) {
            result["code"] = 4;
            LOGD("Failed to create directory: %s", lib_dir.c_str());
            writeString(i, result.dump());
            return;
        }
    }

    LOGD("Copying config file");
    std::string copy_config_dst = lib_dir + frida_config_name;

    if (package_config.contains("config")) {
        copy_file(package_config["config"], copy_config_dst);
    } else {
        std::string copy_config_src = module_dir + "/libgadget.config.so";
        copy_file(copy_config_src, copy_config_dst);
    }
    LOGD("Successfully copied config");

    LOGD("Copying gadget");
    std::string copy_gadget_dst = lib_dir + frida_gadget_name;

    if (package_config.contains("gadget")) {
        copy_file(package_config["gadget"], copy_gadget_dst);
    } else {
        std::string copy_gadget_src = module_dir + "/libgadget.so";
        copy_file(copy_gadget_src, copy_gadget_dst);
    }
    LOGD("Successfully copied gadget");


    result["frida_gadget_name"] = frida_gadget_name;
    result["frida_config_name"] = frida_config_name;
    result["delay"] = delay;
    result["load_mode"] = load_mode;
    writeString(i, result.dump());
}


// 注册我们的模块类和 companion handler 函数
REGISTER_ZYGISK_MODULE(MyModule)

// 伴随进程是具备root权限 它可以做所有的事情，所以我们一般来说移动文件的操作就会交给我们的伴随进程来实现
REGISTER_ZYGISK_COMPANION(companion_handler)
