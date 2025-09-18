package com.example.mysystemservice;

import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import java.lang.reflect.Method;

/**
 * 基于 AIDL 的自定义系统服务实现
 * 继承自动生成的 IMySystemService.Stub 类
 */
public class MySystemService extends IMySystemService.Stub {

    public static final String SERVICE_NAME = "my_custom_manager";
    private static final String TAG = "MySystemService";
    private static MySystemService sInstance;

    private Context mContext;
    private boolean mDebugMode = false;
    private boolean mServiceReady = false;

    public MySystemService(Context context) {
        this.mContext = context;
        sInstance = this;
        mServiceReady = true;
        Log.i(TAG, "MySystemService created with AIDL interface");
    }

    // 实现 AIDL 接口中定义的方法

    @Override
    public String doAwesomeThing(String parameter) throws RemoteException {
        Log.d(TAG, "doAwesomeThing called with parameter: " + parameter);

        if (!mServiceReady) {
            throw new RemoteException("Service not ready");
        }

        // 执行一些有用的操作
        String result = "Awesome result for: " + parameter +
                " (processed by PID: " + android.os.Process.myPid() + ")";

        if (mDebugMode) {
            Log.d(TAG, "doAwesomeThing result: " + result);
        }

        return result;
    }

    @Override
    public int getMagicNumber() throws RemoteException {
        Log.d(TAG, "getMagicNumber called");

        if (!mServiceReady) {
            return -1;
        }

        // 返回一个基于当前时间的"魔法数字"
        int magicNumber = (int) (System.currentTimeMillis() % 1000000);

        if (mDebugMode) {
            Log.d(TAG, "Generated magic number: " + magicNumber);
        }

        return magicNumber;
    }

    @Override
    public boolean isServiceReady() throws RemoteException {
        Log.d(TAG, "isServiceReady called, returning: " + mServiceReady);
        return mServiceReady;
    }

    @Override
    public void setDebugMode(boolean enabled) throws RemoteException {
        Log.i(TAG, "setDebugMode called with enabled: " + enabled);
        mDebugMode = enabled;

        if (mDebugMode) {
            Log.d(TAG, "Debug mode enabled for MySystemService");
        }
    }

    @Override
    public String getServiceInfo() throws RemoteException {
        Log.d(TAG, "getServiceInfo called");

        StringBuilder info = new StringBuilder();
        info.append("MySystemService Information:\n");
        info.append("- Service Name: ").append(SERVICE_NAME).append("\n");
        info.append("- Process ID: ").append(android.os.Process.myPid()).append("\n");
        info.append("- Process UID: ").append(android.os.Process.myUid()).append("\n");
        info.append("- Service Ready: ").append(mServiceReady).append("\n");
        info.append("- Debug Mode: ").append(mDebugMode).append("\n");
        info.append("- Context: ").append(mContext != null ? mContext.getClass().getSimpleName() : "null").append("\n");

        String result = info.toString();

        if (mDebugMode) {
            Log.d(TAG, "Service info:\n" + result);
        }

        return result;
    }

    /**
     * 静态方法，用于被 Zygisk 模块调用以注册服务
     */
    public static boolean registerService(Context context) {
        try {
            // 确保只注册一次
            if (sInstance != null) {
                Log.w(TAG, "Service already registered");
                return true;
            }

            Log.i(TAG, "Starting AIDL service registration...");

            MySystemService service = new MySystemService(context);

            // 使用反射调用 ServiceManager.addService
            Class<?> serviceManagerClass = Class.forName("android.os.ServiceManager");
            Method addServiceMethod = serviceManagerClass.getMethod("addService",
                    String.class, IBinder.class);

            Log.i(TAG, "Registering service with name: " + SERVICE_NAME);
            Log.i(TAG, "Service instance: " + service);
            Log.i(TAG, "Service class: " + service.getClass().getName());
            Log.i(TAG, "Service descriptor: " + service.getInterfaceDescriptor());

            // 注册服务 - service 继承自 IMySystemService.Stub，本身就是 IBinder
            addServiceMethod.invoke(null, SERVICE_NAME, service);

            Log.i(TAG, "addService call completed");

            // 立即验证注册
            Thread.sleep(100); // 短暂等待确保注册完成

            // 方法1：使用 checkService (不等待)
            Method checkServiceMethod = serviceManagerClass.getMethod("checkService", String.class);
            IBinder checkBinder = (IBinder) checkServiceMethod.invoke(null, SERVICE_NAME);
            Log.i(TAG, "checkService result: " + checkBinder);

            // 方法2：使用 getService (可能等待)
            Method getServiceMethod = serviceManagerClass.getMethod("getService", String.class);
            IBinder testBinder = (IBinder) getServiceMethod.invoke(null, SERVICE_NAME);
            Log.i(TAG, "getService result: " + testBinder);

            // 使用 checkService 的结果进行验证，因为它更可靠
            IBinder verifyBinder = checkBinder != null ? checkBinder : testBinder;

            if (verifyBinder != null) {
                Log.i(TAG, "Service registration verified successfully");
                Log.i(TAG, "Verified binder: " + verifyBinder);
                Log.i(TAG, "Verified binder class: " + verifyBinder.getClass().getName());

                // 测试 Binder 状态
                try {
                    boolean alive = verifyBinder.isBinderAlive();
                    Log.i(TAG, "Binder alive status: " + alive);
                } catch (Exception e) {
                    Log.w(TAG, "Cannot check binder alive status", e);
                }

                // 额外验证：尝试将 IBinder 转换为我们的接口
                try {
                    IMySystemService testInterface = IMySystemService.Stub.asInterface(verifyBinder);
                    if (testInterface != null) {
                        Log.i(TAG, "Interface conversion successful");
                        // 测试接口调用
                        boolean ready = testInterface.isServiceReady();
                        Log.i(TAG, "Service interface test successful, ready: " + ready);
                    } else {
                        Log.w(TAG, "Interface conversion returned null");
                    }
                } catch (Exception e) {
                    Log.w(TAG, "Interface test failed", e);
                }

                return true;
            } else {
                Log.e(TAG, "Service registration failed - service not found after registration");

                // 额外调试：列出所有服务
                try {
                    Method listServicesMethod = serviceManagerClass.getMethod("listServices");
                    String[] allServices = (String[]) listServicesMethod.invoke(null);
                    boolean foundInList = false;

                    for (String svcName : allServices) {
                        if (SERVICE_NAME.equals(svcName)) {
                            foundInList = true;
                            break;
                        }
                    }

                    Log.e(TAG, "Service found in service list: " + foundInList);

                } catch (Exception listE) {
                    Log.w(TAG, "Failed to list services for debugging", listE);
                }

                return false;
            }

        } catch (Exception e) {
            Log.e(TAG, "Failed to register MySystemService", e);
            e.printStackTrace();
            return false;
        }
    }

    /**
     * 获取服务实例
     */
    public static MySystemService getInstance() {
        return sInstance;
    }

    /**
     * 清理资源
     */
    public void cleanup() {
        Log.i(TAG, "MySystemService cleanup");
        mServiceReady = false;
        mDebugMode = false;
        // 清理其他资源
    }

    /**
     * 获取接口描述符 - AIDL 会自动生成这个
     */
    @Override
    public String getInterfaceDescriptor() {
        return IMySystemService.DESCRIPTOR;
    }

    /**
     * 处理服务的生命周期
     */
    public void onServiceStart() {
        Log.i(TAG, "Service started");
        mServiceReady = true;
    }

    public void onServiceStop() {
        Log.i(TAG, "Service stopping");
        mServiceReady = false;
    }
}