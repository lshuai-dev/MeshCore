# 查看所有 LVGL 环境
python scripts\publish_lvgl_firmware.py --list

# 只发布指定环境
python scripts\publish_lvgl_firmware.py v1.16.0.3_beta `
  --env Heltec_t1_companion_radio_ble `
  --env Heltec_v3_companion_radio_ble

# 只检查流程，不实际构建
python scripts\publish_lvgl_firmware.py v1.16.0.3_beta --dry-run

# 指定其它输出目录
python scripts\publish_lvgl_firmware.py v1.16.0.3_beta `
  --release-root D:\Temp\meshcore-release

# 覆盖已有版本目录
python scripts\publish_lvgl_firmware.py v1.16.0.3_beta --force