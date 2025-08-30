- frida-gadget使用配置
  - /data/adb/modules/zygisk_demo这个目录下存放着gadget的配置文件
config.json
  - 指定包名注入配置文件 格式如下
    - config指的是gadget的配置文件
    - gadget指的是gadget文件
    - delay指的是gadget加载的延迟时间 单位是微妙 1000000=1秒
```
"com.hackcatml.test": {
    "config": "你的配置文件位置",
    "gadget": "gadget文件位置",
    "delay": 300000
},
"com.example.nativeproject": {},
```

libgadget.config.so
    - gadget配置文件
listen模式 `frida -U -F`进行加载，类似于attach

不阻塞

```
{
  "interaction": {
    "type": "listen",
    "on_port_conflict": "fail",
    "on_load": "resume",
    "address": "0.0.0.0",
    "port": 14725
  }
}
```

阻塞

```
{
  "interaction": {
    "type": "listen",
    "on_port_conflict": "fail",
    "on_load": "wait",
    "address": "0.0.0.0",
    "port": 14725
  }
}
```

持久化模式 不需要使用frida的时候

```
{
  "interaction": {
    "type": "script",
    "path": "/home/oleavr/explore.js"
  }
}
```

libgadget.so
    - frida-gadget文件


gadget时机问题

在app启动的时候，会去fork我们的zygote进程，然后在这个基础上加载我们的app

就是你很难和frida-server一样去在fork之后马上进行加载frida-agent.so

导致一些线程可能是在frida-gadget之前加载的，导致你没有办法去处理这些线程

加载比较晚有好有坏，如果是一次性的，就可以直接绕过，如果是开线程检测的，就没有办法去处理

gadget的特征是比我们frida-server更少的

核心就是让app执行dlopen函数就可


gadget加载逻辑

1.读取配置文件

只能在伴随进程实现，需要root权限才能读取/data/adb/modules下面的目录啊

preAppSpecialize

JNIEnv，从这里拿到包名称

发送过去给我们的伴随进程看是否在配置里面

2.伴随进程进行处理

如果在配置文件里面

把gadget和gadget.config.so复制过去我们的data目录下面去

告诉我们的preAppSpecialize部分，这个是有的

3.加载gadget

postAppSpecialize

这个地方执行我们的加载gadget逻辑
