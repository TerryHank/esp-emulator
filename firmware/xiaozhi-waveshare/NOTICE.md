# 来源与分发说明

包含小智 v2.4.2（MIT）、CarCYD（GPL-3.0）、MiniBike 设计参考（MIT）、Barlow/Noto Sans SC（SIL OFL）、Material Design Icons 和 Espressif SIMD 适配（Apache-2.0）。组合派生代码按 GPL-3.0 分发，原组件许可及版权声明保留，详见 LICENSES/ 和板级 skin_licenses/。

小智上游 https://github.com/78/xiaozhi-esp32
CarCYD 上游 https://github.com/muki01/Car-Dashboard-ESP32-CYD
MiniBike 上游 https://github.com/Cyberj812/MiniBike-Stats

光球来自用户提供的 ESP32S3_LVGL_Canvas_Orb 工程，原有说明保存在板级 LICENSE-Orb。

公开包省略可重建依赖缓存、构建缓存、整片设备 Flash、个人运行日志、NVS 和网络配置。dependencies.lock 与实际构建配置保留。两个上游 MQTT 文档中的 AES 示例改为零值；字体生成注释中的本机命令路径已省略，功能代码和字形数组不变。
