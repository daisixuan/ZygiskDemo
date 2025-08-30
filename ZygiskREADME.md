#### zygisk详解

Zygisk 是基于 Android 的 Zygote 进程实现的。Zygote 是 Android 系统中的一个特殊进程，负责预加载并启动 Java 虚拟机 (JVM)，然后在启动应用程序时进行分叉 (fork) 来生成新的应用程序进程。由于 Zygote 进程是所有 Android 应用进程的母进程，因此通过 Zygisk，可以在每个新生成的应用进程中注入自定义代码。

它有相关的生命周期，和回调函数，具体在zygisk.hpp中有，描述。

在正常的zygsik modulebase里面，它的权限是有问题的，因为它本身是在zygote里面执行的，配合app执行实现的生命周期，也就是说，它一般只有app的权限，作为一个app，只有app的权限它很难做事情。

zygisk会启动一个伴随进程，这个伴随进程是唯一的，在你启动app之后，这个伴随进程是可以和你的app进行相关的通讯，具体的通讯是使用ipc通讯，伴随进程是具备root权限的，它可以做所有的事情，所以我们一般来说移动文件的操作就会交给我们的伴随进程来实现，这个是可以并发的，而不是一次只能处理一个app的一个任务，而是可以处理app的多个任务

zygisk加载进入app之后如何和我们的伴随进程进行一个通讯：

zygisk有相关回调函数，ipc通讯，伴随进程回调函数(可以并发的)


#### zygisk生命周期

zygisk::ModuleBase

```c++
virtual void onLoad([[maybe_unused]] Api *api, [[maybe_unused]] JNIEnv *env) {}
它会在每次模块被加载到目标app进程中时调用，可以进行初始化，是在fork之前的。

virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs *args) {}
在应用进程被 fork 出来后，正式变为一个应用进程之前被调用，此时还没有实现专有化，还是zygote权限，不是root权限，导致是无法访问我们magisk目录的，此时是无法访问app的安装目录和app的专有目录的，此时可以访问/data/local/tmp目录，这个位置和frida-server spawn启动注入的位置非常的相似。

virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs *args) {}
在应用进程被专用化之后调用。此时，进程已具备应用程序的沙箱环境和特定权限，并与应用的实际运行环境一致。此时，应用的权限和资源限制已经设置完成，进程不再具有 Zygote 的高权限，无法访问/data/local/tmp目录，而是变成与应用相同的权限级别，可以访问app的安装目录和app的专业目录了，应用的代码也即将开始执行。

virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs *args) {}
preServerSpecialize 方法是在 Zygote 进程 fork 出 system_server 进程之后、system_server 进程正式专用化之前调用的，这里可以操作serverspeciali

virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs *args) {}
在 system_server 进程完成专用化之后 调用，此时server_system已经正常使用了。
    
注意：preServerSpecialize和postServerSpecialize在全局只会调用一次，使用不多，server_system相关。
```


#### 基础

##### 注册

```c++
// 注册我们的模块类和 companion handler 函数
REGISTER_ZYGISK_MODULE(MyModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
```


##### 回调函数详解

icp通讯

通过在zygisk模块触发，这个fd很关键

```c++
int fd = api->connectCompanion();
```

此时回调函数会收到这个fd，这个fd是文件描述符

```c++
static void companion_handler(int i) {
    ...下面是你的代码
}
```

我们使用read和write实现通讯

```c++
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
```

这里read是阻塞的，但是write不是阻塞的，这个两个代码是运行在不同的进程里面的，所以我们可以一方面使用write一方面使用read来实现app内部的zygisk模块和伴随进程进行通讯，因为app本身模块没有高权限，但是伴随进程是有高权限的。


#### 常用方法

读取zygisk目录代码

```c++
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
```

文件复制代码

```c++
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
```