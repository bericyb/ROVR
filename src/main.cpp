#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <ArduinoJson.h>


const int frequency = 5000; // PWM frequency
const int rightChannel = 0; // PWM channel for right motors
const int leftChannel = 1; // PWM channel for left motors
const int resolution = 8; // PWM resolution (8 bits)


int RIGHTDIR = 23;
int RIGHTFRONT = 33;
int RIGHTREAR = 32;
int LEFTDIR = 22;
int LEFTFRONT = 26;
int LEFTREAR = 25;


const char* ssid = "ROVR1";
const char* password = "redroverredrover";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JsonDocument wsInput;

typedef struct {
  int mode; // 0 - 2 slow, normal, zoomy
  int r; // -255 - 255
  int l; // -255 - 255
} Input;

Input currInput = { .r = 0, .l = 0 };

unsigned long lastTime = 0;
unsigned long timerDelay = 500;

// Function declarations
void outputPwm();

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");
}

void initWiFi() {
  WiFi.softAP(ssid, password);
  Serial.println("Enabling WiFi Access Point");
  Serial.print("SSID: ");
  Serial.println(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    // Log received message
    Serial.print("[WS_RX] Raw message: ");
    Serial.println(message);
    
    DeserializationError error = deserializeJson(wsInput, message);
    if (error) {
      Serial.print("[WS_ERROR] deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Extract and log parsed values
    int newR = wsInput["r"];
    int newL = wsInput["l"];
    int newMode = wsInput["mode"];
    
    Serial.printf("[WS_PARSED] r=%d, l=%d, mode=%d\n", newR, newL, newMode);
    
    // Update current input
    currInput.r = newR;
    currInput.l = newL;
    currInput.mode = newMode;
    
    Serial.println("[WS_SUCCESS] Message processed successfully");
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t * data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      currInput.r = 0;
      currInput.l = 0;
      currInput.mode = 0;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      currInput.r = 0;
      currInput.l = 0;
      currInput.mode = 0;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {
  pinMode(RIGHTDIR, OUTPUT);
  pinMode(LEFTDIR, OUTPUT);
  Serial.begin(115200);

  ledcSetup(rightChannel, frequency, resolution);
  ledcAttachPin(RIGHTFRONT, rightChannel);
  ledcAttachPin(RIGHTREAR, rightChannel);

  ledcSetup(leftChannel, frequency, resolution);
  ledcAttachPin(LEFTFRONT, leftChannel);
  ledcAttachPin(LEFTREAR, leftChannel);
  initWiFi();
  initLittleFS();

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("index", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.begin();
}

void loop() {
  outputPwm();
  ws.cleanupClients();
  delay(10); // Reduced delay for better responsiveness
}

void outputPwm() {
  int rDir = currInput.r >= 0 ? LOW : HIGH;
  int lDir = currInput.l >= 0 ? HIGH : LOW;

  int r = abs(currInput.r);
  int l = abs(currInput.l);

  int scale = currInput.mode == 0 ? 10 : (currInput.mode == 1 ? 5 : 1); // Scale based on mode

  digitalWrite(RIGHTDIR, rDir);
  digitalWrite(LEFTDIR, lDir);
  ledcWrite(rightChannel, r/scale);
  ledcWrite(leftChannel, l/scale);
}
