/**
 * @file i18n.h
 * User interface strings (English / Turkish).
 *
 * Strings are declared once in I18N_STRINGS as X(id, english, turkish), which
 * keeps both languages in lock-step at compile time. Source files are UTF-8;
 * the UI fonts contain the Turkish glyphs (see tools/fonts/generate_fonts.py).
 */
#pragma once

#include <stdint.h>

#include "types.h"

// clang-format off
#define I18N_STRINGS(X) \
  /* ---- Common ---------------------------------------------------------- */ \
  X(APP_TAGLINE, "车辆仪表",              "Araç Gösterge Paneli") \
  X(OK, "确定",                             "Tamam") \
  X(CANCEL, "取消",                         "İptal") \
  X(CLOSE, "关闭",                          "Kapat") \
  X(ON, "开",                             "Açık") \
  X(OFF, "关",                            "Kapalı") \
  X(RESET, "重置",                          "Sıfırla") \
  X(AUTO, "自动",                           "Oto") \
  X(JUST_NOW, "刚刚",                       "az önce") \
  X(FMT_SECONDS_AGO, "%lu秒前",                      "%lu sn önce") \
  X(FMT_MINUTES_AGO, "%lu分钟前",                    "%lu dk önce") \
  X(FMT_HOURS_AGO, "%lu小时前",                      "%lu sa önce") \
  /* ---- Boot ------------------------------------------------------------ */ \
  X(BOOT_SETTINGS, "加载设置",               "Ayarlar yükleniyor") \
  X(BOOT_HARDWARE, "启动硬件",              "Donanım başlatılıyor") \
  X(BOOT_VEHICLE, "连接车辆",          "Araca bağlanılıyor") \
  X(BOOT_READY, "就绪",                          "Hazır") \
  /* ---- Pages ----------------------------------------------------------- */ \
  X(PAGE_DASHBOARD, "仪表",                      "Gösterge") \
  X(PAGE_LIVE, "实时数据",                      "Canlı Veri") \
  X(PAGE_DIAG, "故障诊断",                    "Arıza Teşhis") \
  X(PAGE_TRIP, "行程",                           "Yolculuk") \
  X(PAGE_SETTINGS, "设置",                       "Ayarlar") \
  /* ---- Signals: full names -------------------------------------------- */ \
  X(SIG_SPEED, "车速",                  "Araç hızı") \
  X(SIG_RPM, "电气转速",                   "Motor devri") \
  X(SIG_COOLANT, "电机温度",            "Motor suyu sıcaklığı") \
  X(SIG_OIL_TEMP, "电调温度",                "Yağ sıcaklığı") \
  X(SIG_INTAKE_TEMP, "进气温度",         "Emme havası sıcaklığı") \
  X(SIG_AMBIENT_TEMP, "车外温度",            "Dış sıcaklık") \
  X(SIG_BATTERY, "电池电压",                "Akü voltajı") \
  X(SIG_FUEL_LEVEL, "实时功率",                     "Yakıt seviyesi") \
  X(SIG_LOAD, "占空比",                    "Motor yükü") \
  X(SIG_THROTTLE, "节气门开度",              "Gaz kelebeği konumu") \
  X(SIG_MAP, "进气歧管压力",              "Manifold basıncı") \
  X(SIG_MAF, "电机电流",                  "Hava akış miktarı") \
  X(SIG_TIMING, "点火提前角",                "Ateşleme avansı") \
  X(SIG_FUEL_RATE, "电池电流",                      "Anlık yakıt tüketimi") \
  X(SIG_STFT, "短期燃油修正",           "Kısa dönem yakıt düzeltme") \
  X(SIG_LTFT, "长期燃油修正",            "Uzun dönem yakıt düzeltme") \
  X(SIG_RUN_TIME, "发动机运行时间",                "Motor çalışma süresi") \
  X(SIG_GEAR, "挡位",                           "Vites") \
  /* ---- Signals: short names (tiles) ----------------------------------- */ \
  X(SHORT_SPEED, "车速",                          "HIZ") \
  X(SHORT_RPM, "电气转速",                            "DEVİR") \
  X(SHORT_COOLANT, "电机",                        "SU") \
  X(SHORT_OIL_TEMP, "电调",                            "YAĞ") \
  X(SHORT_INTAKE_TEMP, "进气",                         "EMME") \
  X(SHORT_AMBIENT_TEMP, "车外",                        "DIŞ") \
  X(SHORT_BATTERY, "电池",                        "AKÜ") \
  X(SHORT_FUEL_LEVEL, "功率",                           "YAKIT") \
  X(SHORT_LOAD, "占空比",                           "YÜK") \
  X(SHORT_THROTTLE, "节气门",                       "GAZ") \
  X(SHORT_MAP, "歧管压力",                            "MAP") \
  X(SHORT_MAF, "电机电流",                            "MAF") \
  X(SHORT_TIMING, "点火角",                         "AVANS") \
  X(SHORT_FUEL_RATE, "电池电流",                      "TÜKETİM") \
  X(SHORT_STFT, "短期修正",                           "STFT") \
  X(SHORT_LTFT, "长期修正",                           "LTFT") \
  X(SHORT_RUN_TIME, "运行时间",                       "SÜRE") \
  X(SHORT_GEAR, "挡位",                           "VİTES") \
  /* ---- Dashboard ------------------------------------------------------- */ \
  X(DASH_PICK_TITLE, "选择显示项目",             "Bu göstergede göster") \
  /* ---- Live data ------------------------------------------------------- */ \
  X(LIVE_MIN, "最低",                            "Min") \
  X(LIVE_AVG, "平均",                            "Ort") \
  X(LIVE_MAX, "最高",                            "Maks") \
  X(LIVE_NO_DATA, "暂无数据",                        "Veri yok") \
  /* ---- Diagnostics ----------------------------------------------------- */ \
  X(DIAG_MIL_CAPTION, "控制器故障",                   "Arıza lambası") \
  X(DIAG_MIL_ON, "存在故障",                             "YANIYOR") \
  X(DIAG_MIL_OFF, "无故障",                            "SÖNÜK") \
  X(DIAG_MIL_UNKNOWN, "未连接",                        "Bilinmiyor") \
  X(DIAG_FMT_CODE_ONE, "%u个故障",                  "%u arıza kodu") \
  X(DIAG_FMT_CODE_MANY, "%u个故障",                 "%u arıza kodu") \
  X(DIAG_FMT_LAST_SCAN, "上次扫描 %s",                   "Son tarama %s") \
  X(DIAG_NEVER_SCANNED, "尚未扫描",                "Henüz taranmadı") \
  X(DIAG_SCAN, "扫描",                           "Tara") \
  X(DIAG_CLEAR, "清除",                          "Sil") \
  X(DIAG_SCANNING, "读取故障码…",           "Arıza kodları okunuyor…") \
  X(DIAG_CLEARING, "清除故障码…",          "Arıza kodları siliniyor…") \
  X(DIAG_EMPTY_TITLE, "无故障码",                 "Arıza kodu yok") \
  X(DIAG_EMPTY_TEXT, "控制器当前没有报告故障", "ECU kayıtlı veya bekleyen kod bildirmedi.") \
  X(DIAG_IDLE_TITLE, "准备扫描",                  "Taramaya hazır") \
  X(DIAG_IDLE_TEXT, "连接 VESC 后可读取当前故障", "ECU'daki arıza kodlarını okumak için Tara'ya dokunun.") \
  X(DIAG_CLEAR_TITLE, "清除故障码？",             "Arıza kodları silinsin mi?") \
  X(DIAG_CLEAR_TEXT, "将清除已存故障及冻结帧，并重置就绪监测。请接通点火开关并停止发动机。", "Kayıtlı kodlar ve donmuş veri silinir, hazırlık monitörleri sıfırlanır. Kontak açık, motor kapalı olmalı.") \
  X(DIAG_CLEARED, "故障码已清除",            "Arıza kodları silindi") \
  X(DIAG_SCAN_DONE, "扫描完成",                  "Tarama tamamlandı") \
  X(DIAG_FAILED, "控制器未响应",        "ECU yanıt vermedi") \
  X(DIAG_NOT_CONNECTED, "未连接车辆",   "Araca bağlı değil") \
  X(DTC_STORED, "当前",                         "Kayıtlı") \
  X(DTC_PENDING, "待定",                        "Bekleyen") \
  X(DTC_PERMANENT, "永久",                      "Kalıcı") \
  X(DTC_SYS_POWERTRAIN, "动力系统",                     "Motor / şanzıman") \
  X(DTC_SYS_CHASSIS, "底盘",                        "Şasi") \
  X(DTC_SYS_BODY, "车身",                           "Gövde") \
  X(DTC_SYS_NETWORK, "通信系统",                        "Ağ iletişimi") \
  X(DTC_GENERIC, "控制器故障码",            "Genel (SAE J2012)") \
  X(DTC_MANUFACTURER, "厂家专用",          "Üreticiye özel") \
  X(DTC_SYSTEM, "系统",                         "Sistem") \
  X(DTC_TYPE, "故障类型",                      "Kod tipi") \
  X(DTC_STATUS, "状态",                         "Durum") \
  X(DTC_ADVICE_CRITICAL, "严重故障，请安全停车并检查车辆。", "Kritik: güvenli şekilde durun ve aracı kontrol ettirin.") \
  X(DTC_ADVICE_WARNING, "请尽快检查车辆。", "Aracı en kısa sürede kontrol ettirin.") \
  X(DTC_ADVICE_INFO, "轻微故障，请留意并在下次保养时检查。", "Küçük arıza. Takip edin, ilk bakımda kontrol ettirin.") \
  /* ---- Trip ------------------------------------------------------------ */ \
  X(TRIP_DISTANCE, "里程",                       "Mesafe") \
  X(TRIP_TIME, "行驶时间",                     "Sürüş süresi") \
  X(TRIP_AVG_SPEED, "平均速度",                      "Ort. hız") \
  X(TRIP_MAX_SPEED, "最高速度",                      "Maks. hız") \
  X(TRIP_AVG_CONS, "平均油耗",                "Ort. tüketim") \
  X(TRIP_FUEL_USED, "消耗燃油",                      "Harcanan yakıt") \
  X(TRIP_ACCEL, "加速计时",                   "Hızlanma") \
  X(TRIP_LAST, "上次",                           "Son") \
  X(TRIP_BEST, "最佳",                           "En iyi") \
  X(TRIP_ACCEL_IDLE, "停车后就绪",                    "Durunca hazır") \
  X(TRIP_ACCEL_ARMED, "已就绪，起步！",                     "Hazır, gaz!") \
  X(TRIP_ACCEL_RUNNING, "测量中…",                     "Ölçülüyor…") \
  X(TRIP_RESET_TITLE, "重置行程？",                    "Yolculuk sıfırlansın mı?") \
  X(TRIP_RESET_TEXT, "将清空里程、时间、油耗和速度统计。", "Mesafe, süre, tüketim ve hız istatistikleri sıfırlanacak.") \
  X(TRIP_RESET_DONE, "行程已重置",                     "Yolculuk sıfırlandı") \
  /* ---- Settings: categories -------------------------------------------- */ \
  X(SET_DISPLAY, "显示",                        "Ekran") \
  X(SET_UNITS, "单位",                          "Birimler") \
  X(SET_GAUGES, "仪表",                         "Göstergeler") \
  X(SET_WARNINGS, "警报",                       "Uyarılar") \
  X(SET_SOUND, "声音灯光",                  "Ses ve ışık") \
  X(SET_CONNECTION, "连接",                     "Bağlantı") \
  X(SET_SYSTEM, "系统",                         "Sistem") \
  X(SET_ABOUT, "关于",                          "Hakkında") \
  /* ---- Settings: display ----------------------------------------------- */ \
  X(SET_BRIGHTNESS, "亮度",                     "Parlaklık") \
  X(SET_AUTO_BRIGHTNESS, "自动亮度",                "Otomatik parlaklık") \
  X(SET_AUTO_BRIGHTNESS_SUB, "根据环境光调节", "Ortam ışığı sensörünü kullanır") \
  X(SET_DAY_BRIGHTNESS, "日间亮度",                 "Gündüz parlaklığı") \
  X(SET_NIGHT_BRIGHTNESS, "夜间亮度",               "Gece parlaklığı") \
  X(SET_AMBIENT, "环境光",                  "Ortam ışığı") \
  X(SET_FLIP, "翻转一百八十度",             "Ekranı 180° döndür") \
  X(SET_ACCENT, "主题颜色",                   "Vurgu rengi") \
  /* ---- Settings: units ------------------------------------------------- */ \
  X(SET_UNIT_SPEED, "速度里程",               "Hız ve mesafe") \
  X(SET_UNIT_TEMP, "温度",                    "Sıcaklık") \
  X(SET_UNIT_PRESSURE, "压力",                       "Basınç") \
  /* ---- Settings: gauges ------------------------------------------------ */ \
  X(SET_RPM_MAX, "电气转速量程",               "Devir saati aralığı") \
  X(SET_REDLINE, "电气转速红区",                        "Kırmızı bölge") \
  X(SET_SHIFT_LIGHT, "换挡提示",                    "Vites ışığı") \
  X(SET_SHIFT_RPM, "换挡转速",                    "Vites değiştirme devri") \
  X(SET_LAYOUT_RESET, "恢复仪表布局",             "Gösterge düzenini sıfırla") \
  X(SET_LAYOUT_HINT, "长按仪表数据即可更换显示项目。", "Değiştirmek için gösterge değerine basılı tutun.") \
  X(SET_LAYOUT_DONE, "仪表布局已恢复",          "Gösterge düzeni sıfırlandı") \
  /* ---- Settings: warnings ---------------------------------------------- */ \
  X(SET_COOLANT_WARN, "冷却液温度",            "Motor suyu sıcaklığı") \
  X(SET_VOLTAGE_WARN, "低电压",            "Düşük akü voltajı") \
  X(SET_FUEL_WARN, "低油量",                 "Düşük yakıt seviyesi") \
  X(SET_OVERSPEED, "超速提示",                    "Hız sınırı") \
  /* ---- Settings: sound & light ----------------------------------------- */ \
  X(SET_TOUCH_SOUND, "触摸音效",                   "Dokunma sesleri") \
  X(SET_ALERT_SOUND, "警报音效",                 "Uyarı sesleri") \
  X(SET_VOLUME, "音量",                         "Ses seviyesi") \
  X(SET_STATUS_LED, "状态灯",                     "Durum LED'i") \
  X(SET_STATUS_LED_SUB, "换挡提示和警报",       "Vites ışığı ve uyarılar") \
  /* ---- Settings: connection -------------------------------------------- */ \
  X(SET_DATA_SOURCE, "数据来源",                    "Veri kaynağı") \
  X(SET_SRC_DEMO, "演示",                           "Demo") \
  X(SET_SRC_LINK, "VESC 串口",                     "ESP32 bağlantısı") \
  X(SET_LINK_STATUS, "状态",                         "Durum") \
  X(SET_LINK_INTERFACE, "接口",                      "Arayüz") \
  X(SET_DEMO_HINT, "演示模式模拟车辆行驶。连接车辆接口后才能使用真实车辆数据。", "Demo modu sürüşü simüle eder. Gerçek araç verisi için ESP32 bağlantısını seçin.") \
  X(LINK_CONNECTED, "已连接",                      "Bağlı") \
  X(LINK_CONNECTING, "连接中…",                    "Bağlanıyor…") \
  X(LINK_DISCONNECTED, "未连接",                  "Bağlı değil") \
  X(LINK_ERROR, "错误",                          "Hata") \
  /* ---- Settings: system ------------------------------------------------ */ \
  X(SET_LANGUAGE, "语言",                       "Dil") \
  X(SET_CALIBRATE, "触摸校准",              "Dokunmatik kalibrasyonu") \
  X(SET_RESTART, "重启设备",                 "Cihazı yeniden başlat") \
  X(SET_RESTART_TEXT, "设备即将重启。",   "Cihaz şimdi yeniden başlatılacak.") \
  X(SET_FACTORY_RESET, "重置仪表设置",                  "Fabrika ayarları") \
  X(SET_FACTORY_TEXT, "恢复仪表默认设置，不删除小智和网络配置。", "Tüm ayarlar, yolculuk verileri ve dokunmatik kalibrasyonu silinecek. Cihaz yeniden başlayacak.") \
  /* ---- Settings: about ------------------------------------------------- */ \
  X(ABOUT_FIRMWARE, "固件",                       "Yazılım") \
  X(ABOUT_BUILD, "构建",                          "Derleme") \
  X(ABOUT_BOARD, "开发板",                          "Kart") \
  X(ABOUT_DISPLAY, "显示屏",                        "Ekran") \
  X(ABOUT_CHIP, "处理器",                      "İşlemci") \
  X(ABOUT_FLASH, "闪存",                          "Flash") \
  X(ABOUT_MEMORY, "可用内存",                    "Boş bellek") \
  X(ABOUT_UPTIME, "运行时间",                         "Çalışma süresi") \
  X(ABOUT_GRAPHICS, "图形库",                       "Grafik") \
  /* ---- Touch calibration ----------------------------------------------- */ \
  X(CAL_TITLE, "触摸校准",              "Dokunmatik kalibrasyonu") \
  X(CAL_TAP, "请点击目标中心",   "Hedefin tam ortasına dokunun") \
  X(CAL_VERIFY, "请再次点击目标确认", "Doğrulamak için hedefe tekrar dokunun") \
  X(CAL_FMT_STEP, "第%u点，共%u点",                 "Nokta %u / %u") \
  X(CAL_HINT, "准确点击十字中心", "En iyi sonuç için kalem ucu veya tırnak kullanın") \
  X(CAL_DONE, "校准已保存",              "Kalibrasyon kaydedildi") \
  X(CAL_FAILED, "精度不足，请重试", "Yeterince hassas değil, lütfen tekrar deneyin") \
  /* ---- Alerts ---------------------------------------------------------- */ \
  X(ALERT_COOLANT, "发动机温度过高",        "Motor sıcaklığı yüksek") \
  X(ALERT_OIL, "机油温度过高",           "Yağ sıcaklığı yüksek") \
  X(ALERT_BATTERY_LOW, "电池电压过低",            "Akü voltajı düşük") \
  X(ALERT_BATTERY_HIGH, "充电电压过高",          "Şarj voltajı yüksek") \
  X(ALERT_FUEL, "燃油不足",                 "Yakıt seviyesi düşük") \
  X(ALERT_OVERSPEED, "超过设定车速",           "Hız sınırı aşıldı") \
  X(ALERT_MIL, "控制器故障",          "Motor arıza lambası yandı") \
  X(ALERT_MIL_TEXT, "VESC 报告故障，请查看故障页", "Kodlar için Arıza Teşhis sayfasını açın") \
  X(ALERT_LINK, "车辆连接中断",        "Araç bağlantısı kesildi") \
  X(ALERT_LINK_TEXT, "请检查车辆接口",    "Araç arayüzünü kontrol edin") \
  X(ALERT_TAP_HINT, "点击关闭",                 "Kapatmak için dokunun") \
  X(SETTINGS_RESTORED, "已恢复默认设置",  "Ayarlar varsayılana döndü")
// clang-format on

namespace core {

enum class Str : uint16_t {
#define I18N_ENUM(id, en, tr) id,
  I18N_STRINGS(I18N_ENUM)
#undef I18N_ENUM
  Count
};

namespace i18n {

void set_language(Language lang);
Language language();

/** Returns the UTF-8 text of `id` in the active language. Never returns null. */
const char* get(Str id);

/** Native name of a language (e.g. "Türkçe"), independent of the active language. */
const char* language_name(Language lang);

}  // namespace i18n

/** Shorthand used throughout the UI. */
inline const char* tr(Str id) { return i18n::get(id); }

}  // namespace core
