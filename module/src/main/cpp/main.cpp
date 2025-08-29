/* Copyright 2022-2023 John "topjohnwu" Wu
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "zygisk.hpp"
#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cinttypes>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#include "hack.h"
#include "log.h"
#include "dlfcn.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;



#define AimPackageName "com.example.nativeproject"


class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        // Use JNI to fetch our process name
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        LOGD("preAppSpecialize %s %s", package_name, app_data_dir);
        preSpecialize(package_name, app_data_dir);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

//    void preServerSpecialize(ServerSpecializeArgs *args) override {
//        preSpecialize("system_server");
//    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            JavaVM *vm = nullptr;
            if (env->GetJavaVM(&vm) == JNI_OK) {
                // Get injection delay from config
                int delay = 2;
                LOGI("Main thread blocking for %d seconds before injection", delay);

                // Block main thread for the delay period
                sleep(delay);

                // Then start hack thread with JavaVM
                std::thread hack_thread(hack_prepare, _data_dir, _package_name, data, length, vm);
                hack_thread.detach();
            } else {
                LOGE("Failed to get JavaVM");
            }
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack;
    char *_data_dir;
    char *_package_name;
    void *data;
    size_t length;

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        if (strcmp(package_name, AimPackageName) == 0) {
            LOGI("成功注入目标进程: %s", package_name);
            enable_hack = true;
            _data_dir = new char[strlen(app_data_dir) + 1];
            strcpy(_data_dir, app_data_dir);
            _package_name = new char[strlen(package_name) + 1];
            strcpy(_package_name, package_name);
            auto path = "zygisk/arm64-v8a.so";
            int dirfd = api->getModuleDir();
            int fd = openat(dirfd, path, O_RDONLY);
            if (fd != -1) {
                struct stat sb{};
                fstat(fd, &sb);
                length = sb.st_size;
                data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
            } else {
                LOGW("Unable to open arm file");
            }
        }else{
            // Since we do not hook any functions, we should let Zygisk dlclose ourselves
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

};

//static int urandom = -1;
//
//static void companion_handler(int i) {
//    if (urandom < 0) {
//        urandom = open("/dev/urandom", O_RDONLY);
//    }
//    unsigned r;
//    read(urandom, &r, sizeof(r));
//    LOGD("companion r=[%u]\n", r);
//    write(i, &r, sizeof(r));
//}

// Register our module class and the companion handler function
REGISTER_ZYGISK_MODULE(MyModule)
//REGISTER_ZYGISK_COMPANION(companion_handler)
