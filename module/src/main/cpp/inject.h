//
// Created by jiyulan on 2025/8/31.
//

#ifndef ZYGISKDEMO_INJECT_H
#define ZYGISKDEMO_INJECT_H

void inject_prepare(const char* target_package_name, const std::string& lib_dir, const std::string& frida_gadget_name, const std::string& frida_config_name, const std::string& load_mode, uint time_to_sleep, JavaVM *vm);

#endif //ZYGISKDEMO_INJECT_H
