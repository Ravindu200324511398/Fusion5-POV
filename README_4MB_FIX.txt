This ZIP fixes the boot error:
partition invalid - exceeds flash chip size 0x400000

Your ESP32-S3 reports 4MB flash, so the old 16MB partitions.csv was invalid.
This package includes a 4MB-safe partitions.csv with OTA slots and LittleFS.

Arduino IDE settings:

Board: ESP32S3 Dev Module
Flash Size: 4MB
Partition Scheme: Custom Partition
PSRAM: OPI PSRAM
USB CDC On Boot: Enabled
Upload Mode: UART0 / Hardware CDC
Upload Speed: 921600
Erase All Flash Before Sketch Upload: Disabled after first clean erase
Serial Monitor: 115200 baud

If upload succeeds, expected boot log includes:
[BOOT] LittleFS Mount Success
[BOOT] Starting WiFi router mode...
Router WiFi connected.
Router IP: ...

