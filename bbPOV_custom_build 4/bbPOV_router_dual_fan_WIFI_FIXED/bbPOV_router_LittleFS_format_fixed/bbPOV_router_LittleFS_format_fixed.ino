#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

// Fix compile issue in older NeoPixelBus on ESP32 Core 3.x
#ifndef gpio_hal_iomux_func_sel
#define gpio_hal_iomux_func_sel(reg, func) ((void)0)
#endif

#include "JPEGDEC.h"
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"
#include "webpage.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <NeoPixelBus.h>
#include <SPI.h>
#include <vector>

// Display & Hardware Config
#define PixelCount 100            // Number of LEDs per half-blade
#define NumLeds 200               // Total physical LEDs on the strip
#define Div 320                   // Number of angular divisions per rotation
#define BufferNum 2               // Double buffering
#define MaxStreamBuffer 32 * 1024 // Max JPEG stream size (32KB)

// LED pins (ESP32-S3 default hardware FSPI pins)
#define DATA_PIN 11
#define CLOCK_PIN 12
#define HALL_SENSOR 4

// Global Variables
uint16_t (*imgBuffer)[Div][PixelCount] = nullptr;
uint8_t streamBuffer[MaxStreamBuffer];

JPEGDEC jpeg;
File myfile;
TaskHandle_t nextFileHandle = NULL;
volatile int bufferRot = 0; // Starts at 0
WebServer server(80);

volatile int numDiv = 0;
volatile int stateDiv = 0;
volatile int spinstae = 1; // 1 = play, 0 = pause
volatile unsigned long rotTime =
    100000; // Time of last rotation in microseconds
volatile unsigned long timeOld = 0;
volatile unsigned long timeNow = 0;
volatile unsigned long lastHallTrigger = 0;
volatile unsigned long rpm = 0;

// Driving APA102 strip on hardware SPI (SCLK=12, MISO=13 (dummy), MOSI=11,
// CS=10 (dummy))
NeoPixelBus<DotStarBgrFeature, DotStarSpiMethod> strip(NumLeds);

DynamicJsonDocument doc(8192);
JsonArray avaliableMedia = doc.to<JsonArray>();
int curMedia = 0;

WiFiServer tcpStream(22333);
WiFiClient client;
bool autoNext = true;
RgbColor black(0);

// LED quality tuning
uint8_t gammaTable[256];

void initGammaTable(float gamma = 2.2f) {
  for (int i = 0; i < 256; i++) {
    float x = i / 255.0f;
    gammaTable[i] = (uint8_t)(powf(x, gamma) * 255.0f + 0.5f);
  }
}

inline RgbColor color565ToRgbGamma(uint16_t color) {
  // Expand RGB565 to 8-bit with bit replication, then gamma correct for APA102 visual quality.
  uint8_t r = ((color >> 11) & 0x1F);
  uint8_t g = ((color >> 5) & 0x3F);
  uint8_t b = (color & 0x1F);
  r = (r << 3) | (r >> 2);
  g = (g << 2) | (g >> 4);
  b = (b << 3) | (b >> 2);
  return RgbColor(gammaTable[r], gammaTable[g], gammaTable[b]);
}
volatile bool isDecoding = false;
volatile bool frameReady = false;
volatile int decodeBuffer = 0;
volatile bool uploadInProgress = false;
bool uploadSaveOK = true;
String uploadSaveError = "";

// File helper representing media folders on LittleFS
struct MediaDirectory {
  String folderName;
  int currentFrame;
  File currentFile;

  void begin(String name) {
    folderName = name;
    currentFrame = 0;
  }

  void rewind() { currentFrame = 0; }

  File openNext() {
    if (currentFile) {
      currentFile.close();
    }
    // Search for sequentially numbered JPEGs: /bbPOV-P/folderName/0.jpg, 1.jpg,
    // ...
    String path =
        "/bbPOV-P/" + folderName + "/" + String(currentFrame) + ".jpg";
    if (LittleFS.exists(path)) {
      currentFile = LittleFS.open(path, FILE_READ);
      currentFrame++;
      return currentFile;
    }
    return File(); // Returns empty file if we reach the end
  }
};

MediaDirectory activeMediaDir;

// ISR (Interrupt Service Routine) for Hall Sensor
void IRAM_ATTR RotCount() {
  unsigned long interrupt_time = millis();
  // Simple deBounce (5ms for high RPM support)
  if (interrupt_time - lastHallTrigger > 5) {
    numDiv = 0;
    // Swap buffer only if a new frame is decoded and ready
    if (frameReady) {
      bufferRot = 1 - bufferRot;
      frameReady = false;
    }

    timeNow = micros();
    if (timeOld > 0) {
      rotTime = timeNow - timeOld;
      if (rotTime > 0) {
        rpm = 60000000ULL / rotTime;
      }
    }
    timeOld = timeNow;
    lastHallTrigger = interrupt_time;

    // Notify decode thread to decode the next frame
    if (nextFileHandle != NULL) {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      vTaskNotifyGiveFromISR(nextFileHandle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}



bool ensureDir(const String &dirPath) {
  if (dirPath.length() == 0 || dirPath == "/") return true;
  String current = "";
  int start = 1;
  while (true) {
    int slash = dirPath.indexOf('/', start);
    String part = (slash == -1) ? dirPath.substring(start) : dirPath.substring(start, slash);
    if (part.length() > 0) {
      current += "/" + part;
      if (!LittleFS.exists(current)) {
        if (!LittleFS.mkdir(current)) {
          Serial.printf("mkdir failed: %s\n", current.c_str());
          return false;
        }
      }
    }
    if (slash == -1) break;
    start = slash + 1;
  }
  return true;
}

bool ensureParentDir(const String &filePath) {
  int slash = filePath.lastIndexOf('/');
  if (slash <= 0) return true;
  return ensureDir(filePath.substring(0, slash));
}

void scanMediaRecursive(File dir, std::vector<String> &folders) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    String path = entry.path();
    if (path.length() == 0) path = entry.name();
    if (entry.isDirectory()) {
      scanMediaRecursive(entry, folders);
    } else if (path.startsWith("/bbPOV-P/")) {
      String sub = path.substring(9);
      int slashIdx = sub.indexOf('/');
      if (slashIdx > 0) {
        String folder = sub.substring(0, slashIdx);
        bool exists = false;
        for (const String &f : folders) {
          if (f == folder) { exists = true; break; }
        }
        if (!exists) {
          folders.push_back(folder);
          avaliableMedia.add(folder);
        }
      }
    }
    entry.close();
  }
}

// Media list updates on LittleFS
void updateMediaList() {
  avaliableMedia.clear();

  if (!LittleFS.exists("/bbPOV-P")) {
    LittleFS.mkdir("/bbPOV-P");
  }

  File root = LittleFS.open("/bbPOV-P");
  if (!root) {
    Serial.println("Failed to open /bbPOV-P directory");
    return;
  }

  std::vector<String> folders;
  scanMediaRecursive(root, folders);
  root.close();

  // Do NOT create a fake default media file. It is not a valid JPEG and it hides
  // the real problem. Empty list means no uploaded image yet.
}

// Delete folder and all its contents on flat LittleFS
void deleteFolder(String folderName) {
  String prefix = "/bbPOV-P/" + folderName + "/";
  File root = LittleFS.open(prefix);
  if (!root) return;

  std::vector<String> filesToDelete;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String path = entry.path();
    if (path.length() == 0) path = entry.name();
    if (!entry.isDirectory()) filesToDelete.push_back(path);
    entry.close();
  }
  root.close();

  for (const String &path : filesToDelete) {
    LittleFS.remove(path);
  }
  LittleFS.rmdir(prefix);
}

// Web Server API Handlers
void handleRoot() { server.send_P(200, "text/html", index_html); }

void handleStatus() {
  DynamicJsonDocument docStatus(512);
  docStatus["rpm"] = rpm;
  docStatus["playing"] = spinstae;
  docStatus["auto_next"] = autoNext;
  docStatus["current_media"] = (avaliableMedia.size() > 0) ? avaliableMedia[curMedia].as<String>() : "";
  docStatus["free_heap"] = ESP.getFreeHeap();
  docStatus["total_flash"] = LittleFS.totalBytes();
  docStatus["free_flash"] = LittleFS.totalBytes() - LittleFS.usedBytes();
  docStatus["storage_ok"] = LittleFS.totalBytes() > 0;
  docStatus["pixel_count"] = PixelCount;
  docStatus["div"] = Div;

  String json;
  serializeJson(docStatus, json);
  server.send(200, "application/json", json);
}

void handleList() {
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSelect() {
  String name = server.arg("name");
  for (size_t i = 0; i < avaliableMedia.size(); i++) {
    if (avaliableMedia[i].as<String>() == name) {
      curMedia = i;
      activeMediaDir.begin(name);
      break;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleDelete() {
  String name = server.arg("name");
  deleteFolder(name);
  updateMediaList();
  server.send(200, "text/plain", "Deleted");
}

void handlePlayPause() {
  spinstae = 1 - spinstae;
  server.send(200, "text/plain", spinstae ? "1" : "0");
}

void handleAutoNext() {
  autoNext = !autoNext;
  server.send(200, "text/plain", autoNext ? "1" : "0");
}

void handleNext() {
  if (avaliableMedia.size() == 0) { server.send(200, "text/plain", "NO_MEDIA"); return; }
  curMedia = (curMedia + 1) % avaliableMedia.size();
  activeMediaDir.begin(avaliableMedia[curMedia].as<String>());
  server.send(200, "text/plain", "OK");
}

void handlePrev() {
  if (avaliableMedia.size() == 0) { server.send(200, "text/plain", "NO_MEDIA"); return; }
  curMedia = (curMedia - 1 + avaliableMedia.size()) % avaliableMedia.size();
  activeMediaDir.begin(avaliableMedia[curMedia].as<String>());
  server.send(200, "text/plain", "OK");
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

void handleUpdateGet() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Firmware Update</title>
  <style>
    body { font-family: sans-serif; background: #0f172a; color: #f8fafc; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
    .card { background: #1e293b; padding: 30px; border-radius: 16px; box-shadow: 0 8px 32px rgba(0,0,0,0.5); text-align: center; border: 1px solid rgba(255,255,255,0.08); width: 320px; }
    h2 { margin-bottom: 20px; font-size: 22px; color: #3b82f6; }
    input[type=file] { margin: 20px 0; display: block; width: 100%; color: #94a3b8; }
    button { background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%); border: none; padding: 12px 20px; color: white; border-radius: 8px; font-weight: bold; cursor: pointer; width: 100%; transition: opacity 0.2s; }
    button:hover { opacity: 0.9; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Firmware Update</h2>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update" accept=".bin">
      <button type="submit">Upload Firmware</button>
    </form>
  </div>
</body>
</html>
)rawhtml";
  server.send(200, "text/html", html);
}

void handleUpdatePost() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  delay(1000);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());

    uploadInProgress = true;
    uploadSaveOK = true;
    uploadSaveError = "";
    spinstae = 0;
    detachInterrupt(digitalPinToInterrupt(HALL_SENSOR));
    strip.ClearTo(black);
    strip.Show();

    // Pause decoding task safely without suspending
    unsigned long startWait = millis();
    while (isDecoding && millis() - startWait < 150) {
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    uploadInProgress = false;
    spinstae = 1;
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR), RotCount, FALLING);
  }
}

// Upload chunk handler
void handleFileUpload() {
  HTTPUpload &upload = server.upload();
  static File file;
  if (upload.status == UPLOAD_FILE_START) {
    String path = server.arg("path");
    if (path.length() == 0) {
      path = "/upload.tmp";
    }

    uploadInProgress = true;
    spinstae = 0;
    detachInterrupt(digitalPinToInterrupt(HALL_SENSOR));
    strip.ClearTo(black);
    strip.Show();

    // Pause decoding task safely without suspending
    unsigned long startWait = millis();
    while (isDecoding && millis() - startWait < 150) {
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }

    uploadSaveOK = true;
    uploadSaveError = "";

    if (LittleFS.totalBytes() == 0) {
      uploadSaveOK = false;
      uploadSaveError = "LittleFS storage is 0 bytes. Select 16MB flash + Custom/partition with SPIFFS/LittleFS.";
      Serial.println(uploadSaveError);
    } else if (!ensureParentDir(path)) {
      uploadSaveOK = false;
      uploadSaveError = "Failed to create upload folder";
      Serial.println(uploadSaveError);
    }
    if (uploadSaveOK && LittleFS.exists(path)) LittleFS.remove(path);

    if (uploadSaveOK) {
      file = LittleFS.open(path, FILE_WRITE);
      if (!file) {
        uploadSaveOK = false;
        uploadSaveError = "Failed to open file for writing";
        Serial.println(uploadSaveError);
      }
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (file && uploadSaveOK) {
      size_t written = file.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        uploadSaveOK = false;
        uploadSaveError = "Upload write failed";
        Serial.printf("Upload write failed: %u/%u bytes\n", (unsigned)written, (unsigned)upload.currentSize);
      }
      yield();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (file) {
      file.close();
    }
    // Safe recovery if connection terminates
    uploadInProgress = false;
    spinstae = 1;
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR), RotCount, FALLING);
  }
}

void handleUploadEnd() {
  updateMediaList();
  String name = server.arg("name");
  if (name.length() > 0) {
    for (size_t i = 0; i < avaliableMedia.size(); i++) {
      if (avaliableMedia[i].as<String>() == name) {
        curMedia = i;
        activeMediaDir.begin(name);
        break;
      }
    }
  }

  // Re-attach interrupt and clear variables for a clean display restart
  timeOld = 0;
  numDiv = 0;
  rotTime = 100000;
  frameReady = false;
  uploadInProgress = false;
  spinstae = 1; // Resume playing

  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR), RotCount, FALLING);

  if (nextFileHandle != NULL) {
    xTaskNotifyGive(nextFileHandle);
  }
  server.send(200, "text/plain", "OK");
}

// Web Loop Task (Core 0)
void webloop(void *pvParameters) {
  for (;;) {
    server.handleClient();
    ArduinoOTA.handle();
    vTaskDelay(2 / portTICK_PERIOD_MS); // Prevent starving standard WiFi tasks
  }
}

// JPEGDEC Hooks
void *myOpen(const char *filename, int32_t *size) {
  myfile = activeMediaDir.openNext();
  if (!myfile) {
    if (autoNext) {
      curMedia++;
      if (curMedia >= avaliableMedia.size())
        curMedia = 0;
      activeMediaDir.begin(avaliableMedia[curMedia].as<String>());
    } else {
      activeMediaDir.rewind();
    }
    myfile = activeMediaDir.openNext();
  }
  if (myfile) {
    *size = myfile.size();
    return &myfile;
  }
  return nullptr;
}

void myClose(void *handle) {
  if (myfile)
    myfile.close();
}

int32_t myRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile)
    return 0;
  return myfile.read(buffer, length);
}

int32_t mySeek(JPEGFILE *handle, int32_t position) {
  if (!myfile)
    return 0;
  return myfile.seek(position);
}

int JPEGDraw(JPEGDRAW *pDraw) {
  // Write to the back buffer (decodeBuffer)
  int writeBuffer = decodeBuffer;

  // Copy row by row to support any MCU size (e.g. 8x8, 16x16) correctly
  for (int y = 0; y < pDraw->iHeight; y++) {
    int targetY = pDraw->y + y;
    if (targetY >= Div)
      break;

    int copyWidth = pDraw->iWidth;
    if (pDraw->x + copyWidth > PixelCount) {
      copyWidth = PixelCount - pDraw->x;
    }

    if (copyWidth > 0) {
      memcpy(&imgBuffer[writeBuffer][targetY][pDraw->x],
             &pDraw->pPixels[y * pDraw->iWidth], copyWidth * sizeof(uint16_t));
    }
  }
  return 1;
}

// Live streaming JPEG receiver.
// Important fix: do NOT wait for a Hall interrupt before reading TCP data.
// The previous logic blocked on ulTaskNotifyTake() while a PC client was connected,
// causing sender.py to time out. This version reads incoming stream frames
// continuously and only uses Hall pulses to swap/display the latest decoded frame.
bool readStreamFrame(WiFiClient &c) {
  if (!c.connected()) return false;

  char header[6] = {0, 0, 0, 0, 0, 0}; // 5 ASCII digits/spaces + '\r'
  int got = c.readBytes(header, 6);
  if (got <= 0) return false;
  if (got < 6) {
    // Incomplete header; ignore this partial packet.
    return false;
  }

  // sender.py sends: str(len).ljust(5) + "\r"
  if (header[5] != '\r') {
    // Resync by discarding until CR.
    c.readStringUntil('\r');
    return false;
  }

  header[5] = '\0';
  int len = atoi(header);
  if (len <= 0 || len > MaxStreamBuffer) {
    Serial.printf("[Stream] Bad frame length: %d\n", len);
    return false;
  }

  int received = 0;
  unsigned long startMs = millis();
  while (received < len && c.connected()) {
    int n = c.read(streamBuffer + received, len - received);
    if (n > 0) {
      received += n;
      startMs = millis();
    } else if (millis() - startMs > 1500) {
      Serial.printf("[Stream] Frame timeout: %d/%d bytes\n", received, len);
      return false;
    } else {
      delay(1);
      yield();
    }
  }

  if (received != len) return false;

  isDecoding = true;
  decodeBuffer = 1 - bufferRot;

  bool ok = false;
  if (jpeg.openRAM(streamBuffer, len, JPEGDraw)) {
    jpeg.decode(0, 0, 0);
    jpeg.close();
    frameReady = true;
    ok = true;
  }

  isDecoding = false;
  return ok;
}

// Background Decoder Task (Core 0)
void nextFile(void *pvParameters) {
  for (;;) {
    // Highest priority: live TCP streaming from sender.py
    WiFiClient newClient = tcpStream.available();
    if (newClient) {
      if (client && client.connected()) client.stop();
      client = newClient;
      client.setTimeout(1200);
      Serial.println("[PC Client connected on TCP 22333]");

      while (client.connected()) {
        if (client.available() >= 6) {
          readStreamFrame(client);
        } else {
          delay(1);
          yield();
        }
      }

      client.stop();
      Serial.println("[PC Client disconnected]");
      continue;
    }

    // Normal internal media playback: decode next frame on Hall sync
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
    if (spinstae == 1 && avaliableMedia.size() > 0) {
      isDecoding = true;
      decodeBuffer = 1 - bufferRot;
      if (jpeg.open("", myOpen, myClose, myRead, mySeek, JPEGDraw)) {
        jpeg.decode(0, 0, 0);
        jpeg.close();
        frameReady = true;
      }
      isDecoding = false;
    }

    delay(1);
    yield();
  }
}




void bootLog(const char *msg) {
  Serial.println(msg);
  Serial.flush();
  delay(20);
}

void setupArduinoOTA() {
  ArduinoOTA.setHostname(WiFi.getHostname() ? WiFi.getHostname() : "bbPOV-Fan");

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] End");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u\n", error);
  });

  ArduinoOTA.begin();

  Serial.println("[OTA] Ready");
}


void clearLedStripOnBoot() {
  // Clear APA102 startup garbage after the strip is initialized.
  // This does NOT create a startup animation; it forces all LEDs OFF.
  for (int i = 0; i < 6; i++) {
    strip.ClearTo(black);
    strip.Show();
    delay(80);
    yield();
  }
  Serial.println("LED strip cleared on boot");
  Serial.flush();
}


void setup() {
  Serial.begin(115200);
  delay(1200);
  bootLog("\n[BOOT] bbPOV application started");

  pinMode(HALL_SENSOR, INPUT_PULLUP);
  bootLog("[BOOT] Hall pin configured");

  // Allocate rendering buffer in PSRAM
  bootLog("[BOOT] Allocating frame buffer...");
  imgBuffer = (uint16_t (*)[Div][PixelCount])heap_caps_calloc(
      BufferNum * Div * PixelCount, sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (imgBuffer != nullptr) {
    bootLog("[BOOT] Allocated double frame buffer in PSRAM");
  } else {
    bootLog("[BOOT] PSRAM allocation failed. Trying internal RAM...");
    imgBuffer = (uint16_t (*)[Div][PixelCount])calloc(
        BufferNum * Div * PixelCount, sizeof(uint16_t));
    if (imgBuffer == nullptr) {
      bootLog("[BOOT] FATAL: frame buffer allocation failed");
      while (true) delay(1000);
    }
  }

  // Mount Internal LittleFS with manual format recovery
  bootLog("[BOOT] Mounting LittleFS...");

  if (!LittleFS.begin(false)) {
    bootLog("[BOOT] LittleFS mount failed. Formatting...");

    LittleFS.format();

    if (!LittleFS.begin(false)) {
      bootLog("[BOOT] LittleFS still failed! Continuing without stored media.");
    } else {
      bootLog("[BOOT] LittleFS Mount Success after format.");
    }
  } else {
    bootLog("[BOOT] LittleFS Mount Success.");
  }

  if (!LittleFS.exists("/bbPOV-P")) {
    LittleFS.mkdir("/bbPOV-P");
  }

  updateMediaList();

  if (avaliableMedia.size() > 0) {
    activeMediaDir.begin(avaliableMedia[0].as<String>());
  }

  // Setup WiFi: Router mode first, AP fallback if router fails
  bootLog("[BOOT] Starting WiFi router mode...");

  const char *routerSsid = "Dialog 4G 512";
  const char *routerPass = "A959D8d1";

  const char *apSsid = "bbPOV-Fan";
  const char *apPass = "12345678";

  WiFi.disconnect(true, true);
  delay(300);
  WiFi.mode(WIFI_OFF);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  String hostName = "bbpov-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  WiFi.setHostname(hostName.c_str());

  WiFi.begin(routerSsid, routerPass);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Router WiFi connected.");
    Serial.print("SSID: ");
    Serial.println(routerSsid);
    Serial.print("Device hostname: ");
    Serial.println(hostName);
    Serial.print("Router IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("Use this IP in dual sender.py");
  } else {
    Serial.println("Router WiFi failed. Starting fallback AP mode...");
    WiFi.disconnect(true, true);
    delay(300);
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    bool apOK = WiFi.softAP(apSsid, apPass, 6, 0, 4);
    delay(500);

    if (apOK) {
      Serial.print("AP active. SSID: ");
      Serial.print(apSsid);
      Serial.print("  Password: ");
      Serial.print(apPass);
      Serial.print("  IP: ");
      Serial.println(WiFi.softAPIP());
      Serial.flush();
    } else {
      bootLog("ERROR: WiFi AP failed to start!");
    }
  }

  MDNS.begin(hostName.c_str());
  MDNS.addService("http", "tcp", 80);

  // Arduino IDE / network OTA update support
  setupArduinoOTA();

  // Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/list", HTTP_GET, handleList);
  server.on("/select", HTTP_GET, handleSelect);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/playpause", HTTP_GET, handlePlayPause);
  server.on("/autonext", HTTP_GET, handleAutoNext);
  server.on("/next", HTTP_GET, handleNext);
  server.on("/prev", HTTP_GET, handlePrev);
  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/upload_end", HTTP_POST, handleUploadEnd);
  server.on("/upload_end", HTTP_GET, handleUploadEnd);
  server.on(
      "/upload_file", HTTP_POST, []() {
        if (uploadSaveOK) server.send(200, "text/plain", "OK");
        else server.send(500, "text/plain", uploadSaveError.length() ? uploadSaveError : "Upload failed");
      },
      handleFileUpload);

  server.on("/update", HTTP_GET, handleUpdateGet);
  server.on("/update", HTTP_POST, handleUpdatePost, handleUpdateUpload);

  server.begin();
  tcpStream.begin(22333);
  bootLog("HTTP and TCP Stream servers started");
  Serial.println("Live stream TCP server: 192.168.4.1:22333");
  Serial.flush();

  initGammaTable();

  // Init NeoPixelBus, then clear startup garbage safely.
  bootLog("[BOOT] Starting APA102 strip...");
  strip.Begin(CLOCK_PIN, 13, DATA_PIN, 10); // SCLK=12, MISO=13, MOSI=11, CS=10
  clearLedStripOnBoot();

  // Load first image only when media exists
  if (avaliableMedia.size() > 0) {
    decodeBuffer = 1 - bufferRot;
    if (jpeg.open("", myOpen, myClose, myRead, mySeek, JPEGDraw)) {
      jpeg.decode(0, 0, 0);
      jpeg.close();
      frameReady = true;
    }
  }

  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR), RotCount, FALLING);

  xTaskCreatePinnedToCore(nextFile, "nextFile", 8192, NULL, 4, &nextFileHandle, 0);
  xTaskCreatePinnedToCore(webloop, "webloop", 8192, NULL, 2, NULL, 0);

  vTaskPrioritySet(NULL, 5);
  bootLog("Setup Complete");
}

void loop() {
  ArduinoOTA.handle();
  if (uploadInProgress) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return;
  }
  if (spinstae == 1 && rotTime > 0) {
    unsigned long elapsed = micros() - timeOld;
    if (elapsed < rotTime) {
      int expectedDiv = (elapsed * Div) / rotTime;
      if (expectedDiv > numDiv) {
        numDiv = expectedDiv;
        int showNumDiv = numDiv;

        // Calculate opposing division (180 degrees offset)
        int showNumDivOpp = (showNumDiv + Div / 2) % Div;

        int currentBuf = bufferRot;

        // Draw both halves of the single blade crossing the center (100 LEDs on
        // each half)
        for (int i = 0; i < PixelCount; i++) {
          // First half (LEDs 0-99): mapping radius outwards in reverse
          uint16_t color = imgBuffer[currentBuf][showNumDiv][99 - i];
          strip.SetPixelColor(i, color565ToRgbGamma(color));

          // Second half (LEDs 100-199): mapping radius outwards in forward
          uint16_t color2 = imgBuffer[currentBuf][showNumDivOpp][i];
          strip.SetPixelColor(100 + i, color565ToRgbGamma(color2));
        }

        strip.Show();

        // Switch buffer at half rotation (180 degrees)
        static bool swappedThisRotation = false;
        if (numDiv >= (Div / 2) && !swappedThisRotation) {
          if (frameReady) {
            bufferRot = 1 - bufferRot;
            frameReady = false;
          }
          if (nextFileHandle != NULL) {
            xTaskNotifyGive(nextFileHandle);
          }
          swappedThisRotation = true;
        }

        // Reset the swapped flag at the start of a rotation
        if (numDiv < (Div / 2)) {
          swappedThisRotation = false;
        }
      }
    }
  } else {
    // If stopped, draw a static preview at 5 FPS to reduce CPU consumption
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh > 200) {
      for (int i = 0; i < PixelCount; i++) {
        // Display frame 0 as preview
        uint16_t color = imgBuffer[bufferRot][0][99 - i];
        RgbColor corrected = color565ToRgbGamma(color);
        strip.SetPixelColor(i, corrected);
        strip.SetPixelColor(100 + i, corrected);
      }
      strip.Show();
      lastRefresh = millis();
    }
  }
}
