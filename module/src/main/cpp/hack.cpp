//
// Created by jiyulan on 2025/8/28.
//

#include "hack.h"
#include "log.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <jni.h>
#include <thread>
#include <sys/mman.h>
#include <linux/unistd.h>
#include <array>
#include <sys/stat.h>
//#include <asm-generic/fcntl.h>
#include <fcntl.h>


void hack_thread_func(const char *data_dir, const char *package_name, JavaVM *vm) {
    bool load = false;
    const char *soname = "test";
    LOGI("Hack thread started for package: %s", package_name);
    LOGI("Hack thread started for data_dir: %s", data_dir);

    // Note: Delay is now handled in main thread before this thread is created
    LOGI("Starting injection immediately (delay already applied in main thread)");

    // 构建新文件路径，使用传入的 soname 参数
    char new_so_path[256];
    snprintf(new_so_path, sizeof(new_so_path), "%s/files/%s.so", data_dir, soname);

    // 构建源文件路径
    char src_path[256];
    snprintf(src_path, sizeof(src_path), "/data/local/tmp/%s.so", soname);

    // 打开源文件
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        LOGE("Failed to open %s: %s (errno: %d)", src_path, strerror(errno), errno);
        return;
    }

    int dest_fd = open(new_so_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        LOGE("Failed to open %s", new_so_path);
        close(src_fd);
        return;
    }

    // 复制文件内容
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes) != bytes) {
            LOGE("Failed to write to %s", new_so_path);
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    // 关闭文件描述符
    close(src_fd);
    close(dest_fd);

    // 修改文件权限
    if (chmod(new_so_path, 0755) != 0) {
        LOGE("Failed to change permissions on %s: %s (errno: %d)", new_so_path, strerror(errno), errno);
        return;
    } else {
        LOGI("Successfully changed permissions to 755 on %s", new_so_path);
    }

    // 加载 .so 文件
    void *handle;
    for (int i = 0; i < 10; i++) {
        handle = dlopen(new_so_path, RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            LOGI("Successfully loaded %s", new_so_path);
            load = true;
            break;
        } else {
            LOGE("Failed to load %s: %s", new_so_path, dlerror());
            sleep(1);
        }
    }
    // 如果加载失败
    if (!load) {
        LOGI("%s.so not found in thread %d", soname, gettid());
    }

}


void hack_prepare(const char *data_dir, const char *package_name, void *data, size_t length, JavaVM *vm) {
    LOGI("hack_prepare called for package: %s, dir: %s", package_name, data_dir);

    std::thread hack_thread(hack_thread_func, data_dir, package_name, vm);
    hack_thread.join();
}