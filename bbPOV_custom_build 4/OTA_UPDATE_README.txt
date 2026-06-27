bbPOV Custom Build - QUALITY + OTA v2

OTA update methods included:

1) Web OTA, easiest:
   - Connect to WiFi: bbPOV-Fan
   - Password: 12345678
   - Open: http://192.168.4.1/update
   - Upload Arduino compiled .bin file

2) Arduino IDE Network OTA:
   - Connect laptop to WiFi: bbPOV-Fan
   - Arduino IDE -> Port should show: bbpov-fan
   - OTA password: 12345678

Important:
- Keep ESP32 power stable during OTA.
- Stop/slow motor before firmware update.
- Do not power off until update finishes and ESP32 restarts.
- If OTA port does not appear, use web OTA at /update.
