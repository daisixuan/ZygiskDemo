# Zygisk 演示项目

这是一个用于开发 Android Zygisk 模块的模板项目。

## 特性

*   **Zygisk 模块模板：** 提供创建 Zygisk 模块的基本结构。
*   **自动打包：** 包含 Gradle 脚本，可自动构建模块并将其打包为可刷写的 ZIP 文件 (zygisk_module.zip)。

## 如何使用

1.  克隆此仓库。
2.  在 `module/src/main/cpp/` 中修改模块代码以实现所需功能。
3.  在 `module/module.prop` 中更新模块详细信息。
4.  使用 Android Studio 或在终端中运行 `./gradlew assembleRelease` 来构建项目。
5.  可刷写的 ZIP 文件将生成在 `out/` 目录中。

## 构建

要构建 Zygisk 模块，您可以使用提供的 Gradle 任务：

```bash
./gradlew :module:assembleRelease
```

这将编译 C++ 代码，打包必要的文件，并在 `out/magisk_module_release/` 目录中创建可刷写的 ZIP 文件。
