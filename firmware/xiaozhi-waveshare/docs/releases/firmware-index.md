# 固件版本索引

所有版本均针对微雪 ESP32-S3-Touch-LCD-3.5 非 B。历史版本不等于推荐版本。

| 版本 | 状态 | 固件 | 源码 |
|---|---|---|---|
| 触屏配网正式版（VESC 之前） | 此前确认正常 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/touch-wifi-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/touch-wifi-source.zip) |
| VESC 仪表首版 | 此前确认正常；未连接实际 VESC | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/vesc-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/vesc-source.zip) |
| 早期六种皮肤版 | 历史界面版本，随后被完整 CarCYD 布局替代 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/skins-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/skins-source.zip) |
| CarCYD 原版中文对照 | 原版界面对照，不包含完整小智融合功能 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-original-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-original-source.zip) |
| 中文 CarCYD 功能融合版 | 此前确认显示、触摸、对话、识图正常 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-functional-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-functional-source.zip) |
| 首轮高刷新版 | 此前确认流畅度改善，声音识图正常 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-fast-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-fast-source.zip) |
| 刷新性能优化正式版 | 此前确认流畅、颜色、触摸、声音、识图、配网正常 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-tuning-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-tuning-source.zip) |
| 光球动效初版 | 历史版本，声音响应及状态过渡随后优化 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-orb-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-orb-source.zip) |
| 光球平滑版 | 过渡自然，但光球遮字问题随后修复 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-orb-smooth-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/carcyd-orb-smooth-source.zip) |
| 光球布局稳定版 | 用户确认横竖屏长回复、历史滑动、照片均正常；语音优化前稳定基线 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-orb-layout-20261022/carcyd-orb-layout-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-orb-layout-20261022/carcyd-orb-layout-source.zip) |
| 第一轮语音优化（当前回退运行） | 用户此前确认正常/轻声对话、主动打断、识图正常；2026-10-22 已重新回退到此版 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-history-20261022/speech-stage1-firmware.zip) | 该阶段未单独保留精确源码快照 |
| 最新语音优化候选（已知卡顿） | 编译和63项测试通过，但用户报告整机严重卡顿；运行日志存在编码队列满/音频丢帧；已回退，不作为稳定推荐 | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-speech-20261022-rc1/speech-rc1-firmware.zip) | [下载](https://github.com/TerryHank/esp-emulator/releases/download/waveshare-speech-20261022-rc1/speech-rc1-source.zip) |

各历史版本的对应代码以附带的 source.zip 为准；GitHub 自动生成的仓库 Source code 包是本次归档提交，不表示所有历史固件使用相同代码。源码包未重新构建复现历史二进制，原固件保持原字节。第一轮语音优化没有独立源码快照，明确作为已验收二进制归档。
