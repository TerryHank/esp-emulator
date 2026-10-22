#小智 + VESC 仪表定制版

基于本机已验收的 v2.4.2 定制源码，ESP-IDF 6.0.2。原声音、摄像头、聊天记录滑动、IMU 自动旋转、触屏配网保持在同一工程中。

## 构建

```sh
python scripts/build.py waveshare/esp32-s3-touch-lcd-3.5 \
  --name esp32-s3-touch-lcd-3.5-vesc --language zh-CN --wake-word nihaoxiaozhi
python -m unittest discover -s scripts/tests -v
```

使用独立构建名称及固件类型 `esp32-s3-touch-lcd-3.5-vesc`。只创建一个 LVGL 显示实例；新增代码都位于本板目录，自动由现有板级 CMake glob 编译。UART 控制台改为 USB Serial/JTAG，USB 烧录与日志继续可用。

## UART 接线与设置

按微雪 A01 原理图，J8 排针：

| ESP32 板端 | VESC 端 |
| --- | --- |
| GPIO43 / ESP_TXD，J8 第25脚 | UART RX |
| GPIO44 / ESP_RXD，J8 第27脚 | UART TX |
| GND，J8 第29或30脚 | GND |

固件使用 UART1 映射到上述排针，3.3V TTL，默认 115200、8N1。电调侧需要启用 UART 并使用同样的波特率。仅连接信号及共地；电源不要通过 UART 信号脚相连。GPIO43/44 上不再打印应用日志；芯片 ROM 仍可能在启动时向 TX43 输出启动字符，VESC 数据解析会忽略非协议数据。

“设置”页可保存波特率、安装轮胎后的实测外径（毫米）、电机极对数、传动比（电机转数/轮胎转数，直驱为1）。NVS 命名空间为 `vesc_hud`，不会替换 Wi-Fi 或小智激活配置。没有实测值时填0。未确认具体电机型号，不采用 QS212/QS260 参考参数作为实际设置；10寸轮毂尺寸不能代替安装轮胎外径。

车速：`abs(eRPM) / 极对数 / 传动比 × π × 外径(m) × 60 / 1000`。
没有完整几何参数时，车速显示 `--`，MCP 返回 `speed_kmh:null`；电压、温度等原始遥测不依赖轮径。

## 界面与离线行为

- 启动默认“仪表”：车速、电池电压/电流、功率、电机/电调温度、电机电流；底部显示最新对话字幕。
- “小智”：原有聊天记录、图片预览与滑动；“设置”：仅仪表本地参数。长按 BOOT 的触屏配网、密码九宫格及短按 BOOT 返回不变。
- 横竖屏使用原有 IMU 与触摸校准。仪表布局随屏幕变化；摄像头继续使用原有同步旋转逻辑。
- 仪表/设置使用深色底、白色正文和数值；在线绿色、离线黄色、故障红色。触屏会恢复闲置降低的背光。新增页面持有字体资源，在小智加载/更换字体时同步重绑，防止遗留字体指针造成重启。
- UART 任务独立于网络与语音，以5Hz采集，LVGL以5Hz读取缓存，不在音频/主循环等待电调。Wi-Fi 断线不停止遥测。
- 超过1500ms没有有效 GET_VALUES 回复即失效：所有实时数值显示 `--`，不能将旧值当实时值，也不能用0替代未知故障。
- 收到新的非零故障时显示故障号/名称并播放本地提示音；持续同一故障不反复响。未增加未知的电流/温度阈值。
- VESC 在线时保持屏幕唤醒并避免原有闲置关机；未连接时沿用原来的60秒降亮度、300秒关机行为。UART 首次连接会唤醒已降亮度的屏幕。

## MCP

新增只读工具 `self.vesc.get_status`，无输入参数。返回 JSON 包含：

- `status`：`online` / `waiting`（尚无有效数据）/ `stale` / `uart_error`；`online`：布尔值。
- `sample_uptime_ms`、`sample_age_ms`、`stale_after_ms`、`sequence`、`crc_errors`；时间为设备启动后的单调毫秒，非 UTC。
- `speed_kmh`、`voltage_v`、`battery_current_a`、`motor_current_a`、`power_w`、`motor_temperature_c`、`controller_temperature_c`、`erpm`、`duty_ratio`。
- `amp_hours`、`amp_hours_charged`、`watt_hours`、`watt_hours_charged`、`tachometer`、`tachometer_abs`：电调返回的累计量，未冒称本仪表持久总里程。
- `fault_code`、`fault_name`；故障名称对应官方6.02枚举，未知枚举显示 `UNKNOWN` 并保留数字。
- `transport`、`tx_gpio`、`rx_gpio`、`baud`、`firmware_major/minor`、轮径/极对数/传动比、`speed_configured`。

离线时实时数值与故障都为 `null`；没有样本时样本时间也为 `null`。功率为电压乘电池输入电流，回充为负；车速取绝对值，原始 `erpm` 保留方向。温度为电调上报值；传感器是否安装/校准仍取决于实际控制器配置。

## 协议范围与验证

独立实现帧与固定点字段解析，参照 [VESC release_6_02 commands.c](https://github.com/vedderb/bldc/blob/release_6_02/comm/commands.c)、[packet.c](https://github.com/vedderb/bldc/blob/release_6_02/comm/packet.c)、[故障枚举](https://github.com/vedderb/bldc/blob/release_6_02/datatypes.h)。只发送 `COMM_GET_VALUES(4)` 和 `COMM_FW_VERSION(0)`；没有油门、刹车、电机参数写入、CAN 转发及 BLE 驱动。

解析器支持短/长帧、串流分片、CRC16-CCITT校验、有限长度512字节与串流失步恢复；固定前缀最少54字节，允许新版本附加尾字段，拒绝过短和选择性回复。100ms串流间隙会清除残帧。主机测试使用独立 Python `struct.pack` / `binascii.crc_hqx` 金样、回充/负温度、CRC错误、噪声、截断、超时和车速比例测试，并启用 ASan/UBSan 检查。

当前未连接真实 MKSESC，构建和离线测试不能代替控制器实测。接线后仍需核对实际固件版本、VESC UART应用、实时数值、车速标定、断线清空，以及对话/识图期间的刷新。VescHUD 参考项目的主目标是3.49寸和VESC7.0，不将其电机设置默认值和控制命令移植到这里。
