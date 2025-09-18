package com.example.mysystemservice;

import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import java.lang.reflect.Method;


/**
 * 系统服务辅助工具类
 * 为客户端应用提供便捷的服务访问方法
 */
public class SystemServiceHelper {

    private static final String TAG = "MySystemService";
    private static final String SERVICE_NAME = "my_custom_manager";

    private static IMySystemService sServiceInstance;

    /**
     * 获取系统服务实例
     */
    public static IMySystemService getSystemService() {
        if (sServiceInstance != null) {
            return sServiceInstance;
        }

        try {
            // 使用反射调用 ServiceManager.getService
            Class<?> serviceManagerClass = Class.forName("android.os.ServiceManager");
            Method getServiceMethod = serviceManagerClass.getMethod("getService", String.class);

            IBinder binder = (IBinder) getServiceMethod.invoke(null, SERVICE_NAME);

            if (binder != null) {
                sServiceInstance = IMySystemService.Stub.asInterface(binder);
                Log.i(TAG, "Successfully connected to system service");
            } else {
                Log.e(TAG, "System service not found: " + SERVICE_NAME);
            }

        } catch (Exception e) {
            Log.e(TAG, "Failed to get system service", e);
        }

        return sServiceInstance;
    }

    /**
     * 检查服务是否可用
     */
    public static boolean isServiceAvailable() {
        IMySystemService service = getSystemService();
        if (service == null) {
            return false;
        }

        try {
            // 尝试调用一个简单的方法来检查连接
            service.isServiceReady();
            return true;
        } catch (RemoteException e) {
            Log.e(TAG, "Service connection lost", e);
            sServiceInstance = null; // 重置实例，下次重新获取
            return false;
        }
    }

    /**
     * 执行 awesome 操作
     */
    public static String doAwesome(String parameter) {
        IMySystemService service = getSystemService();
        if (service == null) {
            return "Service not available";
        }

        try {
            return service.doAwesomeThing(parameter);
        } catch (RemoteException e) {
            Log.e(TAG, "Remote call failed", e);
            return "Remote call failed: " + e.getMessage();
        }
    }

    /**
     * 获取魔法数字
     */
    public static int getMagicNumber() {
        IMySystemService service = getSystemService();
        if (service == null) {
            return -1;
        }

        try {
            return service.getMagicNumber();
        } catch (RemoteException e) {
            Log.e(TAG, "Remote call failed", e);
            return -1;
        }
    }

    /**
     * 获取服务信息
     */
    public static String getServiceInfo() {
        IMySystemService service = getSystemService();
        if (service == null) {
            return "Service not available";
        }

        try {
            return service.getServiceInfo();
        } catch (RemoteException e) {
            Log.e(TAG, "Remote call failed", e);
            return "Remote call failed";
        }
    }

    /**
     * 设置调试模式
     */
    public static boolean setDebugMode(boolean enabled) {
        IMySystemService service = getSystemService();
        if (service == null) {
            return false;
        }

        try {
            service.setDebugMode(enabled);
            return true;
        } catch (RemoteException e) {
            Log.e(TAG, "Remote call failed", e);
            return false;
        }
    }

    /**
     * 清理缓存的服务实例
     */
    public static void clearServiceCache() {
        sServiceInstance = null;
    }
}