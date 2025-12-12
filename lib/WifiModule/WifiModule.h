#pragma once
#include <Arduino.h>

// Enable WiFi only when building for the UNO R4 WiFi (or when manually defining WIFI_MODULE_ENABLED).
#ifndef WIFI_MODULE_ENABLED
#if defined(ARDUINO_UNOR4_WIFI)
#define WIFI_MODULE_ENABLED 1
#else
#define WIFI_MODULE_ENABLED 0
#endif
#endif

class WatchWinderApp;

#if WIFI_MODULE_ENABLED
#include <WiFiS3.h>
#include "MotorMetrics.h"
#endif

class WifiModule {
public:
  bool begin(WatchWinderApp *app);
  void disable();
  void tick(unsigned long now);
  bool isEnabled() const { return enabledFlag; }
  bool isConnected() const { return connectedFlag; }

private:
  static constexpr uint16_t IDLE_INTERVAL_MS = 50;   // normal loop cadence for connection upkeep
#if WIFI_MODULE_ENABLED
  void handleClient(WiFiClient &client);
  void sendResponse(WiFiClient &client, int code, const char *reason, const char *contentType, const String &body);
  void sendStatus(WiFiClient &client);
  void sendDashboard(WiFiClient &client);
  void sendHelp(WiFiClient &client);
  bool ensureConnected(unsigned long now);
  int extractPresetId(const String &path) const;
  String htmlPage() const;
  String jsonStatus() const;
#endif

  WatchWinderApp *app = nullptr;
  bool enabledFlag = false;
  bool connectedFlag = false;
#if WIFI_MODULE_ENABLED
  WiFiServer server{80};
  unsigned long lastConnectAttempt = 0;
  uint32_t requestCount = 0;
  uint32_t reconnectCount = 0;
  unsigned long lastTickMs = 0;
  unsigned long lastClientAt = 0;
#endif
};
