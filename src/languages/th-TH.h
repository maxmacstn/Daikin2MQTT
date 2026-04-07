/*
  Daikin2mqtt - Daikin Heat Pump to MQTT control for Home Assistant.
  Copyright (c) 2024 - MaxMacSTN

  Thai language localization
*/
//Main Menu
const char txt_control[] PROGMEM = "ควบคุม";
const char txt_setup[] PROGMEM = "ตั้งค่า";
const char txt_status[] PROGMEM = "สถานะ";
const char txt_firmware_upgrade[] PROGMEM = "อัปเดตเฟิร์มแวร์";
const char txt_reboot[] PROGMEM = "รีบูต";

//Setup Menu
const char txt_MQTT[] PROGMEM = "MQTT";
const char txt_WIFI[] PROGMEM = "WIFI";
const char txt_unit[] PROGMEM = "ตั้งค่าเครื่อง";
const char txt_others[] PROGMEM = "อื่นๆ";
const char txt_logging[] PROGMEM = "บันทึก";
const char txt_reset[] PROGMEM = "รีเซ็ตการตั้งค่า";
const char txt_reset_confirm[] PROGMEM = "คุณต้องการรีเซ็ตเครื่องนี้จริงหรือ?";

//Buttons
const char txt_back[] PROGMEM = "กลับ";
const char txt_save[] PROGMEM = "บันทึกและรีบูต";
const char txt_logout[] PROGMEM = "ออกจากระบบ";
const char txt_upgrade[] PROGMEM = "เริ่มอัปเดต";
const char txt_login[] PROGMEM = "เข้าสู่ระบบ";

//Form choices
const char txt_f_on[] PROGMEM = "เปิด";
const char txt_f_off[] PROGMEM = "ปิด";
const char txt_f_auto[] PROGMEM = "อัตโนมัติ";
const char txt_f_quiet[] PROGMEM = "เงียบ";
const char txt_f_heat[] PROGMEM = "ทำความร้อน";
const char txt_f_dry[] PROGMEM = "ลดความชื้น";
const char txt_f_cool[] PROGMEM = "ทำความเย็น";
const char txt_f_fan[] PROGMEM = "พัดลม";
const char txt_f_speed[] PROGMEM = "ความเร็ว";
const char txt_f_swing[] PROGMEM = "สวิง";
const char txt_f_hold[] PROGMEM = "คงที่";
const char txt_f_celsius[] PROGMEM = "เซลเซียส";
const char txt_f_fh[] PROGMEM = "ฟาเรนไฮต์";
const char txt_f_allmodes[] PROGMEM = "ทุกโหมด";
const char txt_f_noheat[] PROGMEM = "ทุกโหมดยกเว้นทำความร้อน";
const char txt_f_5s[] PROGMEM = "5 วินาที (ทดลอง)";
const char txt_f_10s[] PROGMEM = "10 วินาที";
const char txt_f_15s[] PROGMEM = "15 วินาที";
const char txt_f_30s[] PROGMEM = "30 วินาที";
const char txt_f_45s[] PROGMEM = "45 วินาที";
const char txt_f_60s[] PROGMEM = "60 วินาที";
const char txt_f_beep_on[] PROGMEM = "เปิด";
const char txt_f_beep_off[] PROGMEM = "ปิด";
const char txt_f_led_on[] PROGMEM = "เปิด";
const char txt_f_led_off[] PROGMEM = "ปิด";

//Page Reboot, save & Resseting
const char txt_m_reboot[] PROGMEM = "กำลังรีบูต... รีเฟรชใน";
const char txt_m_reset[] PROGMEM = "กำลังรีเซ็ต... เชื่อมต่อกับ SSID";
const char txt_m_save[] PROGMEM = "กำลังบันทึกการตั้งค่าและรีบูต... รีเฟรชใน";

//Page MQTT
const char txt_mqtt_title[] PROGMEM = "ตั้งค่า MQTT";
const char txt_mqtt_fn[] PROGMEM = "Topic ID";
const char txt_mqtt_device_name[] PROGMEM = "ชื่ออุปกรณ์ (แสดงใน HA)";
const char txt_mqtt_host[] PROGMEM = "โฮสต์";
const char txt_mqtt_port[] PROGMEM = "พอร์ต (ค่าเริ่มต้น 1883)";
const char txt_mqtt_user[] PROGMEM = "ชื่อผู้ใช้";
const char txt_mqtt_password[] PROGMEM = "รหัสผ่าน";
const char txt_mqtt_topic[] PROGMEM = "หัวข้อ";

//Page Others
const char txt_others_title[] PROGMEM = "ตั้งค่าอื่นๆ";
const char txt_others_haauto[] PROGMEM = "ค้นหาอัตโนมัติ HA";
const char txt_others_hatopic[] PROGMEM = "หัวข้อค้นหาอัตโนมัติ HA";
const char txt_others_availability_report[] PROGMEM = "รายงานสถานะ HA";
const char txt_others_debug[] PROGMEM = "ดีบัก";

//Page Status
const char txt_status_title[] PROGMEM = "สถานะ";
const char txt_status_hvac[] PROGMEM = "สถานะเครื่องปรับอากาศ";
const char txt_retries_hvac[] PROGMEM = "จำนวนครั้งที่ลองเชื่อมต่อ";
const char txt_status_mqtt[] PROGMEM = "สถานะ MQTT";
const char txt_status_wifi[] PROGMEM = "สัญญาณ WIFI";
const char txt_status_connect[] PROGMEM = "เชื่อมต่อแล้ว";
const char txt_status_disconnect[] PROGMEM = "ไม่ได้เชื่อมต่อ";

//Page WIFI
const char txt_wifi_title[] PROGMEM = "ตั้งค่า WIFI";
const char txt_wifi_hostname[] PROGMEM = "ชื่อโฮสต์";
const char txt_wifi_SSID[] PROGMEM = "SSID Wi-Fi";
const char txt_wifi_psk[] PROGMEM = "รหัสผ่าน Wi-Fi";
const char txt_wifi_otap[] PROGMEM = "รหัสผ่าน OTA";

//Page Control
const char txt_ctrl_title[] PROGMEM = "ควบคุมเครื่อง";
const char txt_ctrl_temp[] PROGMEM = "อุณหภูมิ";
const char txt_ctrl_power[] PROGMEM = "เปิด/ปิด";
const char txt_ctrl_mode[] PROGMEM = "โหมด";
const char txt_ctrl_fan[] PROGMEM = "พัดลม";
const char txt_ctrl_vane[] PROGMEM = "บานเกล็ดแนวตั้ง";
const char txt_ctrl_wvane[] PROGMEM = "บานเกล็ดแนวนอน";
const char txt_ctrl_ctemp[] PROGMEM = "อุณหภูมิปัจจุบัน";

//Page Unit
const char txt_unit_title[] PROGMEM = "ตั้งค่าเครื่อง";
const char txt_unit_temp[] PROGMEM = "หน่วยอุณหภูมิ";
const char txt_unit_maxtemp[] PROGMEM = "อุณหภูมิสูงสุด";
const char txt_unit_mintemp[] PROGMEM = "อุณหภูมิต่ำสุด";
const char txt_unit_steptemp[] PROGMEM = "ขั้นอุณหภูมิ";
const char txt_unit_update_interval[] PROGMEM = "ช่วงเวลาอัปเดต";
const char txt_unit_modes[] PROGMEM = "โหมดที่รองรับ";
const char txt_unit_password[] PROGMEM = "รหัสผ่านเว็บ";
const char txt_unit_beep[] PROGMEM = "เสียงบี๊ป";
const char txt_unit_led[] PROGMEM = "LED";
const char txt_unit_inside_offset[] PROGMEM = "ชดเชยอุณหภูมิภายใน";
const char txt_unit_outside_offset[] PROGMEM = "ชดเชยอุณหภูมิภายนอก";
const char txt_unit_fan_levels[] PROGMEM = "ระดับความเร็วพัดลม";

//Page Login
const char txt_login_title[] PROGMEM = "ยืนยันตัวตน";
const char txt_login_password[] PROGMEM = "รหัสผ่าน";
const char txt_login_sucess[] PROGMEM = "เข้าสู่ระบบสำเร็จ จะเปลี่ยนหน้าในอีกสักครู่";
const char txt_login_fail[] PROGMEM = "ชื่อผู้ใช้/รหัสผ่านไม่ถูกต้อง ลองอีกครั้ง";

//Page Upgrade
const char txt_upgrade_title[] PROGMEM = "อัปเดต";
const char txt_upgrade_info[] PROGMEM = "อัปเดตเฟิร์มแวร์ OTA โดยอัปโหลดไฟล์ bin";
const char txt_upgrade_start[] PROGMEM = "เริ่มอัปโหลดแล้ว";

//Page Upload
const char txt_upload_nofile[] PROGMEM = "ไม่ได้เลือกไฟล์";
const char txt_upload_filetoolarge[] PROGMEM = "ขนาดไฟล์ใหญ่เกินพื้นที่ว่าง";
const char txt_upload_fileheader[] PROGMEM = "ส่วนหัวไฟล์ไม่ขึ้นต้นด้วย 0xE9";
const char txt_upload_flashsize[] PROGMEM = "ขนาดแฟลชของไฟล์ใหญ่กว่าอุปกรณ์";
const char txt_upload_buffer[] PROGMEM = "บัฟเฟอร์อัปโหลดไม่ตรงกัน";
const char txt_upload_failed[] PROGMEM = "อัปโหลดล้มเหลว เปิดตัวเลือกบันทึก 3 เพื่อดูข้อมูลเพิ่มเติม";
const char txt_upload_aborted[] PROGMEM = "ยกเลิกการอัปโหลด";
const char txt_upload_code[] PROGMEM = "รหัสข้อผิดพลาดอัปโหลด ";
const char txt_upload_error[] PROGMEM = "รหัสข้อผิดพลาดอัปเดต (ดู Updater.cpp) ";
const char txt_upload_sucess[] PROGMEM = "สำเร็จ";
const char txt_upload_refresh[] PROGMEM = "รีเฟรชใน";

//Page Init
const char txt_init_title[] PROGMEM = "ตั้งค่าเริ่มต้น";
const char txt_init_reboot_mes[] PROGMEM = "กำลังรีบูตและเชื่อมต่อกับ WiFi ของคุณ! คุณจะเห็นรายการนี้ในจุดเข้าถึงของคุณ";
const char txt_init_reboot[] PROGMEM = "กำลังรีบูต...";

//Page logging
const char txt_logging_title[] PROGMEM = "บันทึก";
