# 小智 + VESC / CarCYD 光球布局稳定基线

本分支 `stable/orb-layout` 保存 **语音增强试验之前**、用户已确认显示/触摸/旋转/声音/识图/聊天滑动正常的源码，与 [光球布局稳定固件](https://github.com/TerryHank/esp-emulator/releases/tag/waveshare-orb-layout-20261022) 对应。

应用 SHA256：`0b794561322defa928f5e95837798fd5823d13de27b07436791cc890a9e6aa87`。
资源 SHA256：`a24da1d7717491063ddee34ca736cb979314115c7ef841ce726958350d403a24`。

保留中文 CarCYD 界面、VESC 只读遥测/MCP、小智语音与拍照、真实声音响应光球、聊天滑动、IMU 四向旋转、相机方向联动、可选竖屏进入小智、触屏配网九宫格输入、BOOT 返回、屏幕常亮和默认100%音量。

本基线使用该历史版本原有的非实时对话模式。新的实时 AEC/降噪试验尚在实机排查自我打断，**不能把“界面/功能稳定”解读成“实时语音打断已经无回声”**。第一轮回退版虽已确认不卡顿，也仍复现自我打断；各版状态见 [版本索引](docs/releases/firmware-index.md)。开发快照保留在 [development/speech-rc1](https://github.com/TerryHank/esp-emulator/tree/development/speech-rc1/firmware/xiaozhi-waveshare)。

## 构建

适用微雪 ESP32-S3-Touch-LCD-3.5 非 B，ESP-IDF6.0.2 / ESP-SR2.4.7 / LVGL9.5.0。

```sh
git clone --branch stable/orb-layout https://github.com/TerryHank/esp-emulator.git
cd esp-emulator/firmware/xiaozhi-waveshare
# 激活 ESP-IDF6.0.2 后
python scripts/build.py waveshare/esp32-s3-touch-lcd-3.5 --name esp32-s3-touch-lcd-3.5-carcyd --language zh-CN --wake-word nihaoxiaozhi
```

源码恢复了公开打包时误遗漏的16个板级 UI 组件，功能代码与原始对应源码档案保持一致。构建依赖由组件管理器按 dependencies.lock 获取，sdkconfig 保存对应构建配置。普通升级时 app 写入0x20000、配套资源写入0x800000；全量镜像会覆盖空白NVS区域，需重新配网。具体见固件包说明。

组合代码按GPL-3.0分发，原组件版权许可保留；见 LICENSE 与 PUBLICATION-NOTICE.md。上游说明见 README.upstream.md。
