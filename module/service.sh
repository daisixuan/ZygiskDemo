#!/system/bin/sh
MODDIR=${0%/*}

# 确保路径定义
export PATH=/system/bin:/system/xbin:$PATH

# 定义日志函数
log() {
    echo "[JYLModule] $(date '+%Y-%m-%d %H:%M:%S') $1" >> /data/local/tmp/jylmodule.log
}

# 等待系统完全启动
log "等待系统启动完成"
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 1
done
sleep 5 # 额外等待，确保服务启动完成

# 获取系统版本
SDK_VERSION=$(getprop ro.build.version.sdk)
log "检测到系统版本: SDK $SDK_VERSION"


# 确保模块目录权限正确
chmod -R 755 /data/adb/modules/zygisk-sample
chown -R root:root /data/adb/modules/zygisk-sample
chmod 644 /system/framework/mysystemservice.jar

log "安装脚本执行完成"

# 脚本完成
exit 0