# 小智 AI + VESC 中文 CarCYD 仪表

适用 **Waveshare ESP32-S3-Touch-LCD-3.5（非 B）**。本目录是截至 2026-10-22 的当前开发代码；仓库根目录的原 ESP 模拟器项目保留。

**当前开发快照有已知性能回归：用户报告整机严重卡顿，日志显示编码队列满和音频丢帧；也尚未证明自我打断与本地命令问题已解决。该代码按预发布归档，不能视作稳定版本。设备已回退到第一轮已验收固件。**

保留中文 CarCYD、VESC 只读遥测/MCP、语音对话、拍照、聊天滚动、原生 480×320 / 320×480 分辨率、IMU 四向旋转、摄像头方向联动、触屏配网九宫格键盘、BOOT 返回、可选竖屏进入小智及声音响应光球。屏幕不自动休眠，默认播放音量 100%。

本次开发增加 20 ms Opus、PSRAM 编码栈、软件播放参考 AEC、NSNet2、WN9l“你好小智”、MultiNet7 本地命令和启动前本地资源加载。候选版麦克风增益18dB，本地命令包括打开仪表、返回小智、显示电压、打开设置、停止播报；这些新增功能仍有性能与实机验收问题。

## 源码与固件下载

- [最新开发候选（已知卡顿）](https://github.com/TerryHank/esp-emulator/releases/tag/waveshare-speech-20261022-rc1)
- [光球布局稳定基线](https://github.com/TerryHank/esp-emulator/releases/tag/waveshare-orb-layout-20261022)
- [历史固件合集（含当前回退运行的第一轮）](https://github.com/TerryHank/esp-emulator/releases/tag/waveshare-history-20261022)
- [完整版本索引](docs/releases/firmware-index.md)

固件 ZIP 内含中文说明、应用/资源或合并镜像及 SHA256。更新同分区设备时，应用写入0x20000，配套资源写入0x800000；不要跨版本混用资源。完整镜像写入0x0，会覆盖空白NVS区，需重新配网。不要对设备执行整片擦除来做普通升级。

## 构建

使用 ESP-IDF 6.0.2；ESP-SR 2.4.7、LVGL 9.5.0，依赖由组件管理器按 dependencies.lock 获取。

```sh
git clone https://github.com/TerryHank/esp-emulator.git
cd esp-emulator/firmware/xiaozhi-waveshare
# 激活 ESP-IDF 6.0.2 环境后
python scripts/build.py waveshare/esp32-s3-touch-lcd-3.5 --name esp32-s3-touch-lcd-3.5-carcyd --language zh-CN --wake-word wn9l_nihaoxiaozhi_tts3
```

源码包含已生成字体和实际构建配置。63项主机测试、播放参考环形缓冲ASAN/UBSAN测试和资源一致性校验通过；通过这些测试不代表性能或实机功能已经验收。VESC尚未接线，真实遥测与车速标定未验证，不推测未知轮径/极对数/传动比；无油门、刹车或写电调参数功能。

组合代码遵循GPL-3.0，见[LICENSE](LICENSE)、[NOTICE.md](NOTICE.md)及[LICENSES](LICENSES/)。原始上游说明见[README.upstream.md](README.upstream.md)。
