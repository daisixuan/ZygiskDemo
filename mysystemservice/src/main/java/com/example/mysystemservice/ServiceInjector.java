package com.example.mysystemservice;

import android.content.Context;
import android.util.Log;
import java.lang.reflect.Method;

/**
 * 服务注入器 - 被 Zygisk 模块调用
 * 这个类负责在 system_server 进程中注册自定义系统服务
 */
public class ServiceInjector {

    private static final String TAG = "MySystemService";
    private static boolean sInjected = false;

    /**
     * 主入口方法 - Zygisk 模块会调用这个方法
     * @param context System context (通常是 system_server 的 context)
     * @return 注入是否成功
     */
    public static boolean injectService(Context context) {
        if (sInjected) {
            Log.w(TAG, "Service already injected, skipping");
            return true;
        }

        Log.i(TAG, "Starting service injection in PID: " + android.os.Process.myPid() + ";UID: " + android.os.Process.myUid());

        try {
            // 检查当前进程是否为 system_server
            if (!isSystemServerProcess()) {
                Log.e(TAG, "Not in system_server process, injection aborted");
                return false;
            }

            // 注册自定义系统服务
            boolean success = MySystemService.registerService(context);

            if (success) {
                sInjected = true;
                Log.i(TAG, "Service injection completed successfully");

                // 可选：设置清理钩子
                // setupCleanupHook();

            } else {
                Log.e(TAG, "Service injection failed");
            }

            return success;

        } catch (Exception e) {
            Log.e(TAG, "Exception during service injection", e);
            return false;
        }
    }

    /**
     * 检查当前进程是否为 system_server
     */
    private static boolean isSystemServerProcess() {
        try {
            // 方法1: 检查进程名
            String processName = getCurrentProcessName();
            if ("system_server".equals(processName)) {
                return true;
            }

            // 方法2: 检查 UID (system_server 的 UID 通常是 1000)
            int uid = android.os.Process.myUid();
            if (uid == 1000) {
                Log.d(TAG, "Running as system UID: " + uid);
                return true;
            }

            Log.d(TAG, "Process: " + processName + ", UID: " + uid);
            return false;

        } catch (Exception e) {
            Log.e(TAG, "Error checking process", e);
            return false;
        }
    }

    /**
     * 获取当前进程名
     */
    private static String getCurrentProcessName() {
        try {
            // 方法1: 通过 /proc/self/cmdline
            java.io.BufferedReader reader = new java.io.BufferedReader(
                    new java.io.FileReader("/proc/self/cmdline"));
            String processName = reader.readLine();
            reader.close();

            if (processName != null) {
                processName = processName.trim();
                // 移除可能的 null 字符
                int nullPos = processName.indexOf('\0');
                if (nullPos >= 0) {
                    processName = processName.substring(0, nullPos);
                }
                return processName;
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to read process name from cmdline", e);
        }

        // 方法2: 使用反射获取 ActivityThread.currentProcessName()
        try {
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method currentProcessNameMethod = activityThreadClass.getMethod("currentProcessName");
            String processName = (String) currentProcessNameMethod.invoke(null);
            if (processName != null) {
                return processName;
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to get process name via ActivityThread", e);
        }

        return "unknown";
    }

    /**
     * 设置清理钩子（可选）
     */
    private static void setupCleanupHook() {
        try {
            Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                Log.i(TAG, "Cleaning up injected service");
                MySystemService instance = MySystemService.getInstance();
                if (instance != null) {
                    instance.cleanup();
                }
            }));
        } catch (Exception e) {
            Log.w(TAG, "Failed to setup cleanup hook", e);
        }
    }

    /**
     * 获取注入状态
     */
    public static boolean isInjected() {
        return sInjected;
    }
}