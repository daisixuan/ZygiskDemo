// IMySystemService.aidl
package com.example.mysystemservice;

/**
 * AIDL interface for custom system service
 * This will be injected into system_server via Zygisk
 */
interface IMySystemService {
    /**
     * Performs an awesome operation with the given parameter
     */
    String doAwesomeThing(String parameter);

    /**
     * Returns a magic number
     */
    int getMagicNumber();

    /**
     * Additional methods for demonstration
     */
    boolean isServiceReady();

    void setDebugMode(boolean enabled);

    String getServiceInfo();
}