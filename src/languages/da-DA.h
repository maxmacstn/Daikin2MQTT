/*
  Daikin2mqtt - Daikin Heat Pump to MQTT control for Home Assistant.
  Copyright (c) 2024 - MaxMacSTN

  Danish localization
*/

//Main Menu
const char txt_control[] PROGMEM = "Kontrol";
const char txt_setup[] PROGMEM = "Indstillinger";
const char txt_status[] PROGMEM = "Status";
const char txt_firmware_upgrade[] PROGMEM = "Firmware Opgradering";
const char txt_reboot[] PROGMEM = "Genstart";

//Setup Menu
const char txt_MQTT[] PROGMEM = "MQTT";
const char txt_WIFI[] PROGMEM = "WIFI";
const char txt_unit[] PROGMEM = "Enhed";
const char txt_others[] PROGMEM = "Diverse";
const char txt_logging[] PROGMEM = "Logging";
const char txt_reset[] PROGMEM = "Reset konfiguration";
const char txt_reset_confirm[] PROGMEM = "Er du sikker på at du vil nulstille enheden?";

//Buttons
const char txt_back[] PROGMEM = "Tilbage";
const char txt_save[] PROGMEM = "Gem & Genstart";
const char txt_logout[] PROGMEM = "Log ud";
const char txt_upgrade[] PROGMEM = "Start opgradering";
const char txt_login[] PROGMEM = "LOGIN";

//Form choices
const char txt_f_on[] PROGMEM = "ON";
const char txt_f_off[] PROGMEM = "OFF";
const char txt_f_auto[] PROGMEM = "AUTO";
const char txt_f_quiet[] PROGMEM = "QUIET";
const char txt_f_heat[] PROGMEM = "HEAT";
const char txt_f_dry[] PROGMEM = "DRY";
const char txt_f_cool[] PROGMEM = "COOL";
const char txt_f_fan[] PROGMEM = "FAN";
const char txt_f_speed[] PROGMEM = "SPEED";
const char txt_f_swing[] PROGMEM = "SWING";
const char txt_f_hold[] PROGMEM = "HOLD";
const char txt_f_celsius[] PROGMEM = "Celsius";
const char txt_f_fh[] PROGMEM = "Fahrenheit";
const char txt_f_allmodes[] PROGMEM = "Alle tilstande";
const char txt_f_noheat[] PROGMEM = "Alle tilstande undtagen varme";
const char txt_f_5s[] PROGMEM = "5 sekunder (eksperimentel)";
const char txt_f_10s[] PROGMEM = "10 sekunder";
const char txt_f_15s[] PROGMEM = "15 sekunder";
const char txt_f_30s[] PROGMEM = "30 sekunder";
const char txt_f_45s[] PROGMEM = "45 sekunder";
const char txt_f_60s[] PROGMEM = "60 sekunder";
const char txt_f_beep_on[] PROGMEM = "ON";
const char txt_f_beep_off[] PROGMEM = "OFF";
const char txt_f_led_on[] PROGMEM = "ON";
const char txt_f_led_off[] PROGMEM = "OFF";

//Page Reboot, save & Resetting
const char txt_m_reboot[] PROGMEM = "Genstarter... Opdaterer om";
const char txt_m_reset[] PROGMEM = "Nulstiller... Forbind til SSID";
const char txt_m_save[] PROGMEM = "Gemmer konfiguration og genstarter... Opdaterer om";

//Page MQTT
const char txt_mqtt_title[] PROGMEM = "MQTT Parametre";
const char txt_mqtt_fn[] PROGMEM = "Topic ID";
const char txt_mqtt_device_name[] PROGMEM = "Enhedsnavn (vist i HA)";
const char txt_mqtt_host[] PROGMEM = "Host";
const char txt_mqtt_port[] PROGMEM = "Port (standard 1883)";
const char txt_mqtt_user[] PROGMEM = "Bruger";
const char txt_mqtt_password[] PROGMEM = "Adgangskode";
const char txt_mqtt_topic[] PROGMEM = "Topic";

//Page Others
const char txt_others_title[] PROGMEM = "Andre Parametre";
const char txt_others_haauto[] PROGMEM = "HA Autodiscovery";
const char txt_others_hatopic[] PROGMEM = "HA Autodiscovery topic";
const char txt_others_availability_report[] PROGMEM = "HA Availability report";
const char txt_others_debug[] PROGMEM = "Debug";

//Page Status
const char txt_status_title[] PROGMEM = "Status";
const char txt_status_hvac[] PROGMEM = "HVAC Status";
const char txt_retries_hvac[] PROGMEM = "HVAC Forbindelsesforsøg";
const char txt_status_mqtt[] PROGMEM = "MQTT Status";
const char txt_status_wifi[] PROGMEM = "WIFI RSSI";
const char txt_status_connect[] PROGMEM = "FORBUNDET";
const char txt_status_disconnect[] PROGMEM = "AFBRUDT";

//Page WIFI
const char txt_wifi_title[] PROGMEM = "WIFI Parametre";
const char txt_wifi_hostname[] PROGMEM = "Hostname";
const char txt_wifi_SSID[] PROGMEM = "Wi-Fi SSID";
const char txt_wifi_psk[] PROGMEM = "Wi-Fi Adgangskode";
const char txt_wifi_otap[] PROGMEM = "OTA Adgangskode";

//Page Control
const char txt_ctrl_title[] PROGMEM = "Kontrol Enhed";
const char txt_ctrl_temp[] PROGMEM = "Temperatur";
const char txt_ctrl_power[] PROGMEM = "Strøm";
const char txt_ctrl_mode[] PROGMEM = "Tilstand";
const char txt_ctrl_fan[] PROGMEM = "Ventilator";
const char txt_ctrl_vane[] PROGMEM = "Vertikal Lamelle";
const char txt_ctrl_wvane[] PROGMEM = "Horisontal Lamelle";
const char txt_ctrl_ctemp[] PROGMEM = "Aktuel temperatur";

//Page Unit
const char txt_unit_title[] PROGMEM = "Enhedskonfiguration";
const char txt_unit_temp[] PROGMEM = "Temperaturenhed";
const char txt_unit_maxtemp[] PROGMEM = "Maksimum temperatur";
const char txt_unit_mintemp[] PROGMEM = "Minimum temperatur";
const char txt_unit_steptemp[] PROGMEM = "Temperatur trin";
const char txt_unit_update_interval[] PROGMEM = "Opdateringsinterval";
const char txt_unit_modes[] PROGMEM = "Tilstandsunderstøttelse";
const char txt_unit_password[] PROGMEM = "Web adgangskode";
const char txt_unit_beep[] PROGMEM = "Beep";
const char txt_unit_led[] PROGMEM = "LED";
const char txt_unit_inside_offset[] PROGMEM = "Indvendig temp. offset";
const char txt_unit_outside_offset[] PROGMEM = "Udvendig temp. offset";
const char txt_unit_fan_levels[] PROGMEM = "Antal ventilatorhastigheder";

//Page Login
const char txt_login_title[] PROGMEM = "Autentificering";
const char txt_login_password[] PROGMEM = "Adgangskode";
const char txt_login_sucess[] PROGMEM = "Login lykkedes, du bliver omdirigeret om et øjeblik.";
const char txt_login_fail[] PROGMEM = "Forkert brugernavn/adgangskode! Prøv igen.";

//Page Upgrade
const char txt_upgrade_title[] PROGMEM = "Opgradering";
const char txt_upgrade_info[] PROGMEM = "Firmware OTA opgradering via bin fil upload";
const char txt_upgrade_start[] PROGMEM = "Upload startet";

//Page Upload
const char txt_upload_nofile[] PROGMEM = "Ingen fil valgt";
const char txt_upload_filetoolarge[] PROGMEM = "Filstørrelsen er større end tilgængelig ledig plads";
const char txt_upload_fileheader[] PROGMEM = "Fil magic header starter ikke med 0xE9";
const char txt_upload_flashsize[] PROGMEM = "Fil flash størrelse er større end enhedens flash størrelse";
const char txt_upload_buffer[] PROGMEM = "Fil upload buffer uoverensstemmelse";
const char txt_upload_failed[] PROGMEM = "Upload fejlede. Aktiver logging mulighed 3 for mere information";
const char txt_upload_aborted[] PROGMEM = "Upload afbrudt";
const char txt_upload_code[] PROGMEM = "Upload fejlkode ";
const char txt_upload_error[] PROGMEM = "Opdateringsfejlkode (se Updater.cpp) ";
const char txt_upload_sucess[] PROGMEM = "Succesfuld";
const char txt_upload_refresh[] PROGMEM = "Opdaterer om";

//Page Init
const char txt_init_title[] PROGMEM = "Førstegangsopsætning";
const char txt_init_reboot_mes[] PROGMEM = "Genstarter og forbinder til dit WiFi netværk! Du bør se den listet på dit access point.";
const char txt_init_reboot[] PROGMEM = "Genstarter...";

//Page logging
const char txt_logging_title[] PROGMEM = "Logging";
