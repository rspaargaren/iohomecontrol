#include <web_server_handler.h>

#if defined(WEBSERVER)
#include "ArduinoJson.h"       // For creating JSON responses
#include "ESPAsyncWebServer.h" // Or WebServer.h if that's preferred for memory
#include <AsyncJson.h>
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <LittleFS.h>
#include <Update.h>
#include <cstdlib>
#include <interact.h>
#include <firmware_version.h>
#include <iohcCryptoHelpers.h>
#include <iohcRemote1W.h>
#include <iohcRemoteMap.h>
#include <iohcPacket.h>
#include <log_buffer.h>
#include <mqtt_handler.h>
#include <nvs_helpers.h>
#include <oled_display.h>
#include <version_info.h>
#include <WiFi.h>
#include <wifi_helper.h>
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif
#include <tokens.h>
// #include "main.h" // Or other relevant headers to access device data and
// command functions

#define LOG_TAG "http"

// Assume ESPAsyncWebServer for now.
// If you use WebServer.h, the setup and request handling will be different.
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data,
                      size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Ensure we broadcast the current position before sending init message
    IOHC::iohcRemote1W::getInstance()->updatePositions();

    // Build a compact init message containing only device information
    JsonDocument doc;
    doc["type"] = "init";

    JsonArray devices = doc["devices"].to<JsonArray>();
    const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
    for (const auto &r : remotes) {
      JsonObject d = devices.add<JsonObject>();
      d["id"] = bytesToHexString(r.node, sizeof(r.node)).c_str();
      d["name"] = r.name.c_str();
      d["position"] = r.positionTracker.getPosition();
    }

    String payload;
    serializeJson(doc, payload);
    client->text(payload);

    // Stream cached log messages individually to avoid a large JSON payload
    auto logMsgs = getLogMessages();
    for (const auto &m : logMsgs) {
      JsonDocument logDoc;
      logDoc["type"] = "log";
      logDoc["message"] = m;
      String logPayload;
      serializeJson(logDoc, logPayload);
      client->text(logPayload);
    }
  }
}

void broadcastLog(const String &msg) {
  JsonDocument doc;
  doc["type"] = "log";
  doc["message"] = msg;
  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
}

void broadcastDevicePosition(const String &id, int position) {
  JsonDocument doc;
  doc["type"] = "position";
  doc["id"] = id;
  doc["position"] = position;
  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
}

void broadcastLastAddress(const String &addr) {
  JsonDocument doc;
  doc["type"] = "lastaddr";
  doc["address"] = addr;
  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
}

// Structure describing a device entry returned to the web UI
struct Device {
  String id;
  String name;
};

template<class T>
using ArGetRequestHandlerFunction = std::function<void(AsyncWebServerRequest *request, T &root)>;
ArRequestHandlerFunction _jsonGet(const ArGetRequestHandlerFunction<JsonVariant> handler) {
  return [handler] (AsyncWebServerRequest *request) {
      std::unique_ptr<AsyncJsonResponse> response(new AsyncJsonResponse());
      if (!response.get()) {
        request->send(500, "text/plain", "Internal Server Error");
        return;
      }

      handler(request, response->getRoot());

      if (!request->isSent()) {
        response->setLength();
        request->send(response.release());
      }
  };
}

ArRequestHandlerFunction jsonGet(const ArGetRequestHandlerFunction<JsonObject> handler) {
  return _jsonGet([handler] (AsyncWebServerRequest *request, JsonVariant &root) {
    JsonObject json = root.to<JsonObject>();
    handler(request, json);
  });
}

ArRequestHandlerFunction jsonGet(const ArGetRequestHandlerFunction<JsonArray> handler) {
  return _jsonGet([handler] (AsyncWebServerRequest *request, JsonVariant &root) {
    JsonArray json = root.to<JsonArray>();
    handler(request, json);
  });
}

template<class T>
using ArPostRequestHandlerFunction = std::function<void(AsyncWebServerRequest *request, JsonObject &doc, T &root)>;
ArJsonRequestHandlerFunction _jsonPost(const ArPostRequestHandlerFunction<JsonVariant> handler) {
  return [handler] (AsyncWebServerRequest *request, JsonVariant &json) -> void {
    if (request->method() != HTTP_POST) {
      request->send(405, "text/plain", "Method Not Allowed");
      return;
    }

    JsonObject doc = json.as<JsonObject>();
    if (doc.isNull()) {
      request->send(400, "application/json",
                    "{\"success\":false, \"message\":\"Invalid JSON\"}");
      return;
    }

    std::unique_ptr<AsyncJsonResponse> response(new AsyncJsonResponse());
    if (!response.get()) {
      request->send(500, "text/plain", "Internal Server Error");
      return;
    }

    handler(request, doc, response->getRoot());

    if (!request->isSent()) {
      response->setLength();
      request->send(response.release());
    }
  };
}

ArJsonRequestHandlerFunction jsonPost(const ArPostRequestHandlerFunction<JsonObject> handler) {
  return _jsonPost([handler] (AsyncWebServerRequest *request, JsonObject &doc, JsonVariant &root) {
    JsonObject json = root.to<JsonObject>();
    handler(request, doc, json);
  });
}

ArJsonRequestHandlerFunction jsonPost(const ArPostRequestHandlerFunction<JsonArray> handler) {
  return _jsonPost([handler] (AsyncWebServerRequest *request, JsonObject &doc, JsonVariant &root) {
    JsonArray json = root.to<JsonArray>();
    handler(request, doc, json);
  });
}

void handleApiDevices(AsyncWebServerRequest *request, JsonArray &root) {
  // Update device positions before returning them to the web client
  IOHC::iohcRemote1W::getInstance()->updatePositions();

  auto remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
  std::sort(remotes.begin(), remotes.end(),
            [](const IOHC::iohcRemote1W::remote &r1,
               const IOHC::iohcRemote1W::remote &r2) {
              return r1.name.compare(r2.name) < 0;
            });
  for (const auto &r : remotes) {
    JsonObject deviceObj = root.add<JsonObject>();
    deviceObj["id"] = bytesToHexString(r.node, sizeof(r.node)).c_str();
    deviceObj["name"] = r.name.c_str();
    deviceObj["description"] = r.description.c_str();
    deviceObj["position"] = r.positionTracker.getPosition();
    deviceObj["travel_time"] = r.travelTime;
    deviceObj["paired"] = r.paired;
    deviceObj["repeatOnNoResponse"] = r.repeatOnNoResponse;
  }

  // Provide a generic command interface as last entry
  // JsonObject cmdObj = root.add<JsonObject>();
  // cmdObj["id"] = "cmd_if";
  // cmdObj["name"] = "Command Interface";

  // log_i("Sent device list"); // Requires a logging library
}

void handleApiRemotes(AsyncWebServerRequest *request, JsonArray &root) {
  auto entries = IOHC::iohcRemoteMap::getInstance()->getEntries();
  std::sort(entries.begin(), entries.end(),
            [](const IOHC::iohcRemoteMap::entry &e1,
               const IOHC::iohcRemoteMap::entry &e2) {
              return e1.name.compare(e2.name) < 0;
            });
  for (const auto &e : entries) {
    JsonObject obj = root.add<JsonObject>();
    obj["id"] = bytesToHexString(e.node, sizeof(e.node)).c_str();
    obj["name"] = e.name.c_str();
    JsonArray devs = obj["devices"].to<JsonArray>();
    for (const auto &d : e.devices) {
      devs.add(d.c_str());
    }
  }
}

void handleDownloadDevices(AsyncWebServerRequest *request) {
  if (LittleFS.exists(IOHC_1W_REMOTE)) {
    request->send(LittleFS, IOHC_1W_REMOTE, "application/json", true);
  } else {
    request->send(404, "application/json",
                  "{\"message\":\"1W.json not found\"}");
  }
}

void handleDownloadRemotes(AsyncWebServerRequest *request) {
  if (LittleFS.exists(REMOTE_MAP_FILE)) {
    request->send(LittleFS, REMOTE_MAP_FILE, "application/json", true);
  } else {
    request->send(404, "application/json",
                  "{\"message\":\"RemoteMap.json not found\"}");
  }
}

void handleUploadDevicesDone(AsyncWebServerRequest *request) {
  request->send(200, "application/json",
                "{\"message\":\"Devices file uploaded\"}");
  IOHC::iohcRemote1W::getInstance()->load();
  IOHC::iohcRemoteMap::getInstance()->load();
}

void handleUploadDevicesFile(AsyncWebServerRequest *request, String filename,
                             size_t index, uint8_t *data, size_t len,
                             bool final) {
  if (!index) {
    request->_tempFile = LittleFS.open(IOHC_1W_REMOTE, "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
  }
  if (final) {
    request->_tempFile.close();
  }
}

void handleUploadRemotesDone(AsyncWebServerRequest *request) {
  request->send(200, "application/json",
                "{\"message\":\"Remotes file uploaded\"}");
  IOHC::iohcRemoteMap::getInstance()->load();
}

void handleUploadRemotesFile(AsyncWebServerRequest *request, String filename,
                             size_t index, uint8_t *data, size_t len,
                             bool final) {
  if (!index) {
    request->_tempFile = LittleFS.open(REMOTE_MAP_FILE, "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
  }
  if (final) {
    request->_tempFile.close();
  }
}

void handleApiCommand(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  String deviceId = doc["deviceId"] | "";
  String command = doc["command"] | "";

  if (command.isEmpty()) {
    request->send(400, "application/json",
                  "{\"success\":false, \"message\":\"Missing command\"}");
    return;
  }

  Tokens segments;
  tokenize(command.c_str(), ' ', segments);
  if (segments.empty()) {
    request->send(400, "application/json",
                  "{\"success\":false, \"message\":\"Invalid command\"}");
    return;
  }

  deviceId.toLowerCase();
  if (!deviceId.isEmpty()) {
    const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
    auto it = std::find_if(remotes.begin(), remotes.end(),
                           [&](const IOHC::iohcRemote1W::remote &r) {
      return bytesToHexString(r.node, sizeof(r.node)) == deviceId.c_str();
    });
    if (it == remotes.end()) {
      request->send(400, "application/json",
                    "{\"success\":false, \"message\":\"Unknown device\"}");
      return;
    }
    segments.insert(segments.begin() + 1, it->description);
  }

  bool success = false;
  String message;
  for (uint8_t idx = 0; idx <= lastEntry; ++idx) {
    if (_cmdHandler[idx] == nullptr)
      continue;
    if (strcmp(_cmdHandler[idx]->cmd, segments[0].c_str()) == 0) {
      _cmdHandler[idx]->handler(&segments);
      success = true;
      break;
    }
  }

  if (success)
    message = "Command executed";
  else
    message = "Unknown command";

  addLogMessage(message);

  root["success"] = success;
  root["message"] = message;
}

void handleApiAction(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  String deviceId = doc["deviceId"] | "";
  String action = doc["action"] | "";

  deviceId.toLowerCase();
  action.toLowerCase();

  if (deviceId.isEmpty() || action.isEmpty()) {
    request->send(
        400, "application/json",
        "{\"success\":false, \"message\":\"Missing deviceId or action\"}");
    return;
  }

  const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
  auto it = std::find_if(remotes.begin(), remotes.end(),
                         [&](const IOHC::iohcRemote1W::remote &r) {
    return bytesToHexString(r.node, sizeof(r.node)) == deviceId.c_str();
  });
  if (it == remotes.end()) {
    request->send(400, "application/json",
                  "{\"success\":false, \"message\":\"Unknown device\"}");
    return;
  }

  IOHC::RemoteButton btn;
  if (action == "open")
    btn = IOHC::RemoteButton::Open;
  else if (action == "close")
    btn = IOHC::RemoteButton::Close;
  else if (action == "stop")
    btn = IOHC::RemoteButton::Stop;
  else {
    request->send(400, "application/json",
                  "{\"success\":false, \"message\":\"Invalid action\"}");
    return;
  }

  Tokens t;
  t.push_back(action.c_str());
  t.push_back(it->description);
  IOHC::iohcRemote1W::getInstance()->cmd(btn, &t);
  broadcastDevicePosition(deviceId,
                          static_cast<int>(it->positionTracker.getPosition()));

  String msg = "Action " + action + " sent to " + String(it->name.c_str());
  addLogMessage(msg);

  root["success"] = true;
  root["message"] = msg;
}

void handleApiInfo(AsyncWebServerRequest *request, JsonObject &root) {
  appendVersionInfo(root);
}

void handleApiLogs(AsyncWebServerRequest *request, JsonArray &root) {
  auto logs = getLogMessages();
  for (const auto &msg : logs) {
    root.add(msg);
  }
}

void handleApiLastAddr(AsyncWebServerRequest *request, JsonObject &root) {
  const IOHC::Address3 addr = IOHC::lastFromAddress.load();
  root["address"] = bytesToHexString(addr.b, sizeof(addr.b)).c_str();
}

static void scheduleRestart(const char *taskName) {
  static std::atomic<bool> rebootScheduled{false};
  if (!rebootScheduled.exchange(true)) {
    xTaskCreate(
      [](void *) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP.restart();
      },
      taskName,
      2048,
      nullptr,
      5,
      nullptr
    );
  }
}

static bool isValidHostname(const String &hostname) {
  if (hostname.length() == 0 || hostname.length() > 31) {
    return false;
  }
  for (size_t i = 0; i < hostname.length(); ++i) {
    const char c = hostname.charAt(i);
    if (!isalnum(c) && c != '-') {
      return false;
    }
  }
  return hostname.charAt(0) != '-' && hostname.charAt(hostname.length() - 1) != '-';
}

static bool isValidIpString(const String &value) {
  IPAddress ip;
  return value.length() > 0 && ip.fromString(value);
}

void handleApiNetworkGet(AsyncWebServerRequest *request, JsonObject &root) {
  root["hostname"] = WiFi.getHostname() ? WiFi.getHostname() : "MiOpenIO";
  root["dhcp"] = true;
  root["connected"] = WiFi.status() == WL_CONNECTED;
  root["ip"] = WiFi.localIP().toString();
  root["mask"] = WiFi.subnetMask().toString();
  root["gateway"] = WiFi.gatewayIP().toString();
  root["dns1"] = WiFi.dnsIP(0).toString();
  root["dns2"] = WiFi.dnsIP(1).toString();
  root["sntp"] = "pool.ntp.org";
}

void handleApiNetworkSet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  String hostname = doc["hostname"] | "MiOpenIO";
  bool dhcp = doc["dhcp"] | true;
  String ip = doc["ip"] | "";
  String mask = doc["mask"] | "";
  String gateway = doc["gateway"] | "";
  String dns1 = doc["dns1"] | "";
  String dns2 = doc["dns2"] | "";
  String sntp = doc["sntp"] | "";
  hostname.trim(); ip.trim(); mask.trim(); gateway.trim(); dns1.trim(); dns2.trim(); sntp.trim();

  if (!isValidHostname(hostname)) {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid hostname\"}");
    return;
  }

  if (!dhcp) {
    if (!isValidIpString(ip) || !isValidIpString(mask) || !isValidIpString(gateway)) {
      request->send(400, "application/json", "{\"success\":false,\"message\":\"Static IP, mask and gateway are required\"}");
      return;
    }
    if ((!dns1.isEmpty() && !isValidIpString(dns1)) || (!dns2.isEmpty() && !isValidIpString(dns2))) {
      request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid DNS address\"}");
      return;
    }
  }

  nvs_write_string(NVS_KEY_NET_HOST, std::string(hostname.c_str()));
  nvs_write_bool(NVS_KEY_NET_DHCP, dhcp);
  nvs_write_string(NVS_KEY_NET_IP, std::string(ip.c_str()));
  nvs_write_string(NVS_KEY_NET_MASK, std::string(mask.c_str()));
  nvs_write_string(NVS_KEY_NET_GW, std::string(gateway.c_str()));
  nvs_write_string(NVS_KEY_NET_DNS1, std::string(dns1.c_str()));
  nvs_write_string(NVS_KEY_NET_DNS2, std::string(dns2.c_str()));
  nvs_write_string(NVS_KEY_NET_SNTP, std::string(sntp.c_str()));

  root["success"] = true;
  root["message"] = "Network config saved, rebooting";
  root["dhcp"] = dhcp;
  root["hostname"] = hostname;
  scheduleRestart("net-reboot");
}

void handleApiFallbackGet(AsyncWebServerRequest *request, JsonObject &root) {
  bool enabled = true;
  uint16_t bootRetries = 3;
  uint16_t runRetries = 3;
  uint16_t timeout = 600;
  nvs_read_bool(NVS_KEY_FB_ENABLED, enabled);
  nvs_read_u16(NVS_KEY_FB_BOOT, bootRetries);
  nvs_read_u16(NVS_KEY_FB_RUN, runRetries);
  nvs_read_u16(NVS_KEY_FB_TIMEOUT, timeout);
  root["enabled"] = enabled;
  root["retriesBoot"] = bootRetries;
  root["retriesRunning"] = runRetries;
  root["timeout"] = timeout;
}

void handleApiFallbackSet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  nvs_write_bool(NVS_KEY_FB_ENABLED, doc["enabled"] | true);
  nvs_write_u16(NVS_KEY_FB_BOOT, static_cast<uint16_t>(doc["retriesBoot"] | 3));
  nvs_write_u16(NVS_KEY_FB_RUN, static_cast<uint16_t>(doc["retriesRunning"] | 3));
  nvs_write_u16(NVS_KEY_FB_TIMEOUT, static_cast<uint16_t>(doc["timeout"] | 600));
  root["success"] = true;
  root["message"] = "Fallback AP settings saved";
}

void handleApiWifiGet(AsyncWebServerRequest *request, JsonObject &root) {
  root["ssid"] = getConfiguredWiFiSSID();
  root["connected"] = WiFi.status() == WL_CONNECTED;
  root["currentSsid"] = WiFi.SSID();
  root["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  root["rssi"] = wifiStatus.rssi.load();
  root["quality"] = wifiStatus.signalStrengthPercent.load();
}

void handleApiWifiScan(AsyncWebServerRequest *request, JsonObject &root) {
  WiFi.scanDelete();
  WiFi.mode(WIFI_STA);

  int count = WiFi.scanNetworks(false, false);
  if (count <= 0) {
    WiFi.scanDelete();
    delay(250);
    count = WiFi.scanNetworks(false, true);
  }

  root["count"] = count;
  root["status"] = WiFi.status();
  root["connected"] = WiFi.status() == WL_CONNECTED;
  root["ssid"] = WiFi.SSID();
  JsonArray networks = root["networks"].to<JsonArray>();

  if (count < 0) {
    WiFi.scanDelete();
    return;
  }

  for (int i = 0; i < count; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) {
      continue;
    }
    JsonObject item = networks.add<JsonObject>();
    item["ssid"] = ssid;
    item["rssi"] = WiFi.RSSI(i);
    item["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    item["channel"] = WiFi.channel(i);
  }
  WiFi.scanDelete();
}

void handleApiWifiSet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  String ssid = doc["ssid"] | "";
  String password = doc["password"] | "";
  ssid.trim();

  if (ssid.isEmpty() || ssid.length() > 32) {
    request->send(400, "application/json",
                  "{\"success\":false,\"message\":\"SSID is required and must be 32 characters or less\"}");
    return;
  }
  if (password.length() > 64) {
    request->send(400, "application/json",
                  "{\"success\":false,\"message\":\"Password is too long\"}");
    return;
  }

  saveWiFiCredentials(ssid, password);
  root["success"] = true;
  root["message"] = "WiFi settings saved, rebooting";
  root["ssid"] = ssid;
  scheduleRestart("wifi-reboot");
}

static bool jsonToBool(JsonVariant variant, bool &value) {
  if (variant.is<bool>()) {
    value = variant.as<bool>();
    return true;
  }
  if (variant.is<int>() || variant.is<long>() || variant.is<unsigned long>() ||
      variant.is<uint8_t>() || variant.is<uint16_t>() || variant.is<uint32_t>()) {
    value = variant.as<long>() != 0;
    return true;
  }
  if (variant.is<const char *>()) {
    const char *text = variant.as<const char *>();
    if (!text) {
      return false;
    }
    String normalized = String(text);
    normalized.toLowerCase();
    if (normalized == "true" || normalized == "1" || normalized == "yes" ||
        normalized == "on") {
      value = true;
      return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no" ||
        normalized == "off") {
      value = false;
      return true;
    }
  }
  return false;
}

#if defined(SSD1306_DISPLAY)
void handleApiDisplayGet(AsyncWebServerRequest *request, JsonObject &root) {
  const bool enabled = isDisplayEnabled();
  root["enabled"] = enabled;
}

void handleApiDisplaySet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  bool enabled = isDisplayEnabled();
  if (!doc["enabled"].is<JsonVariant>() || !jsonToBool(doc["enabled"], enabled)) {
    request->send(400, "application/json",
                  "{\"success\":false,\"message\":\"Invalid enabled value\"}");
    return;
  }

  setDisplayEnabled(enabled);

  root["success"] = true;
  root["message"] = "Display configuration updated";
  root["enabled"] = isDisplayEnabled();
}
#endif

#if defined(SYSLOG)
void handleApiSyslogGet(AsyncWebServerRequest *request, JsonObject &root) {
  initSyslog();

  root["enabled"] = syslog_enabled;
  root["server"] = syslog_server.c_str();
  root["port"] = syslog_port;
  root["tag"] = syslog_tag.c_str();
}

void handleApiSyslogSet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  initSyslog();

  bool enabledChanged = false;
  bool serverChanged = false;
  bool portChanged = false;

  if (doc["enabled"].is<JsonVariant>()) {
    bool newEnabled = syslog_enabled;
    if (!jsonToBool(doc["enabled"], newEnabled)) {
      request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"Invalid enabled value\"}");
      return;
    }
    if (newEnabled != syslog_enabled) {
      syslog_enabled = newEnabled;
      nvs_write_bool(NVS_KEY_SYSLOG_ENABLED, syslog_enabled);
      enabledChanged = true;
    }
  }

  if (doc["server"].is<JsonVariant>()) {
    String newServer = doc["server"] | "";
    if (newServer != syslog_server.c_str()) {
      syslog_server = newServer.c_str();
      nvs_write_string(NVS_KEY_SYSLOG_SERVER, syslog_server);
      serverChanged = true;
    }
  }

  if (doc["port"].is<JsonVariant>()) {
    int portValue = -1;
    JsonVariant portVariant = doc["port"];
    if (portVariant.is<uint16_t>() || portVariant.is<int>() ||
        portVariant.is<long>() || portVariant.is<unsigned long>()) {
      portValue = portVariant.as<int>();
    } else if (portVariant.is<const char *>()) {
      portValue = atoi(portVariant.as<const char *>());
    }

    if (portValue <= 0 || portValue > 65535) {
      request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"Invalid port value\"}");
      return;
    }

    if (syslog_port != static_cast<uint16_t>(portValue)) {
      syslog_port = static_cast<uint16_t>(portValue);
      nvs_write_u16(NVS_KEY_SYSLOG_PORT, syslog_port);
      portChanged = true;
    }
  }

  if (doc["tag"].is<JsonVariant>()) {
    String newTag = doc["tag"] | "";
    // Sanitise: keep only alphanumeric and hyphen, truncate to 20 chars
    String sanitised;
    for (char c : newTag) {
      if (isalnum(c) || c == '-') sanitised += c;
      if (sanitised.length() >= 20) break;
    }
    if (sanitised != syslog_tag.c_str()) {
      syslog_tag = sanitised.c_str();
      nvs_write_string(NVS_KEY_SYSLOG_TAG, syslog_tag);
    }
  }

  if (enabledChanged || serverChanged || portChanged) {
    resetSyslog();
    if (syslog_enabled) {
      initSyslog();
    }
  }

  root["success"] = true;
  root["message"] = "Syslog configuration updated";
  root["enabled"] = syslog_enabled;
  root["server"] = syslog_server.c_str();
  root["port"] = syslog_port;
  root["tag"] = syslog_tag.c_str();
}

void handleApiSyslogTest(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  if (!syslog_enabled) {
    root["success"] = false;
    root["message"] = "Syslog is disabled";
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    root["success"] = false;
    root["message"] = "WiFi not connected";
    return;
  }
  sendSyslog("Test message from web UI", 6);
  root["success"] = true;
  root["message"] = "Test message sent";
}
#endif

#if defined(MQTT)
void handleApiMqttGet(AsyncWebServerRequest *request, JsonObject &root) {
  root["server"] = mqtt_server.c_str();
  root["user"] = mqtt_user.c_str();
  root["password"] = mqtt_password.c_str();
  root["discovery"] = mqtt_discovery_topic.c_str();
  root["clientId"] = mqtt_client_id.c_str();
  root["port"] = mqtt_port;
}

void handleApiMqttSet(AsyncWebServerRequest *request, JsonObject &doc, JsonObject &root) {
  String server = doc["server"] | "";
  String user = doc["user"] | "";
  String password = doc["password"] | "";
  String discovery = doc["discovery"] | "";
  String clientId = doc["clientId"] | "";
  int portValue = -1;
  if (doc["port"].is<JsonVariant>()) {
    JsonVariant portVariant = doc["port"];
    if (portVariant.is<uint16_t>() || portVariant.is<int>() || portVariant.is<long>()) {
      portValue = portVariant.as<int>();
    } else if (portVariant.is<const char*>()) {
      portValue = atoi(portVariant.as<const char*>());
    }
  }

  bool mqttChanged = false;
  bool discChanged = false;

  if (!server.isEmpty()) {
    mqtt_server = server.c_str();
    nvs_write_string(NVS_KEY_MQTT_SERVER, mqtt_server);
    mqttChanged = true;
  }
  if (!user.isEmpty()) {
    mqtt_user = user.c_str();
    nvs_write_string(NVS_KEY_MQTT_USER, mqtt_user);
    mqttChanged = true;
  }
  if (!password.isEmpty()) {
    mqtt_password = password.c_str();
    nvs_write_string(NVS_KEY_MQTT_PASSWORD, mqtt_password);
    mqttChanged = true;
  }
  if (!discovery.isEmpty()) {
    mqtt_discovery_topic = discovery.c_str();
    nvs_write_string(NVS_KEY_MQTT_DISCOVERY, mqtt_discovery_topic);
    discChanged = true;
  }
  if (!clientId.isEmpty() && mqtt_client_id != clientId.c_str()) {
    mqtt_client_id = clientId.c_str();
    nvs_write_string(NVS_KEY_MQTT_CLIENT_ID, mqtt_client_id);
    mqttChanged = true;
  }

  if (portValue > 0 && portValue <= 65535 && mqtt_port != static_cast<uint16_t>(portValue)) {
    mqtt_port = static_cast<uint16_t>(portValue);
    nvs_write_u16(NVS_KEY_MQTT_PORT, mqtt_port);
    mqttChanged = true;
  }

  if (mqttChanged) {
    mqttClient.disconnect();
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_password.c_str());
    mqttClient.setClientId(mqtt_client_id.c_str());
  }

  if (discChanged && mqttStatus == ConnState::Connected) {
    handleMqttConnect();
  }

  root["success"] = true;
  root["message"] = "MQTT configuration updated";
}
#endif

void handleFirmwareUpdate(AsyncWebServerRequest *request) {
  if (Update.hasError()) {
    request->send(500, "application/json",
                  "{\"message\":\"Firmware update failed\"}");
  } else {
    request->send(200, "application/json",
                  "{\"message\":\"Firmware update successful, rebooting\"}");
    static std::atomic<bool> rebootScheduled{false};
    if (!rebootScheduled.exchange(true)) {
      xTaskCreate(
        [](void *) {
          vTaskDelay(pdMS_TO_TICKS(1000));
          ESP.restart();
        },
        "reboot",
        2048,
        nullptr,
        5,
        nullptr
      );
    }
  }
}

void handleFirmwareUpload(AsyncWebServerRequest *request, String filename,
                          size_t index, uint8_t *data, size_t len,
                          bool final) {
  if (!index) {
    ESP_LOGI(LOG_TAG, "Firmware upload start: %s", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      ESP_LOGE(LOG_TAG, "Update begin error: %s", Update.errorString());
    }
  }
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      ESP_LOGE(LOG_TAG, "Update error: %s", Update.errorString());
    }
  }
  if (final) {
    if (!Update.end(true)) {
      ESP_LOGE(LOG_TAG, "Update end error: %s", Update.errorString());
    } else {
      ESP_LOGI(LOG_TAG, "Firmware upload complete: %u bytes", index + len);
    }
  }
}

void handleFilesystemUpdate(AsyncWebServerRequest *request) {
  if (Update.hasError()) {
    request->send(500, "application/json",
                  "{\"message\":\"Filesystem update failed\"}");
    LittleFS.begin();
  } else {
    request->send(200, "application/json",
                  "{\"message\":\"Filesystem update successful, rebooting\"}");
    static std::atomic<bool> rebootScheduled{false};
    if (!rebootScheduled.exchange(true)) {
      xTaskCreate(
        [](void *) {
          vTaskDelay(pdMS_TO_TICKS(1000));
          ESP.restart();
        },
        "reboot",
        2048,
        nullptr,
        5,
        nullptr
      );
    }
  }
}

void handleFilesystemUpload(AsyncWebServerRequest *request, String filename,
                            size_t index, uint8_t *data, size_t len,
                            bool final) {
  if (!index) {
    ESP_LOGI(LOG_TAG, "Filesystem upload start: %s", filename.c_str());
    LittleFS.end();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
      ESP_LOGE(LOG_TAG, "Filesystem update begin error: %s", Update.errorString());
      return;
    }
  }
  if (!Update.hasError()) {
    if (len && Update.write(data, len) != len) {
      ESP_LOGE(LOG_TAG, "Filesystem update write error: %s", Update.errorString());
    }
  }
  if (final) {
    if (!Update.end(true)) {
      ESP_LOGE(LOG_TAG, "Filesystem update end error: %s", Update.errorString());
    } else {
      ESP_LOGI(LOG_TAG, "Filesystem upload complete: %u bytes", index + len);
    }
  }
}

void setupWebServer() {
  ESP_LOGI(LOG_TAG, "Initializing HTTP server ...");

  // Serve static files from /web_interface_data
  // Ensure this path matches where your platformio.ini places data files
  // or how you upload them (e.g., SPIFFS, LittleFS).
  // The path "/" serves index.html from the data directory.
  if (!LittleFS.exists("/web_interface_data/index.html")) {
    ESP_LOGW(LOG_TAG, "/web_interface_data/index.html not found");
  }

  // API Endpoints
  server.on("/api/info", HTTP_GET, jsonGet(handleApiInfo));
  server.on("/api/devices", HTTP_GET, jsonGet(handleApiDevices));
  server.on("/api/remotes", HTTP_GET, jsonGet(handleApiRemotes));
  server.on("/api/logs", HTTP_GET, jsonGet(handleApiLogs));
  server.on("/api/lastaddr", HTTP_GET, jsonGet(handleApiLastAddr));
  server.on("/api/wifi-scan", HTTP_GET, jsonGet(handleApiWifiScan));
  server.on("/api/wifi", HTTP_GET, jsonGet(handleApiWifiGet));
  server.on("/api/network", HTTP_GET, jsonGet(handleApiNetworkGet));
  server.on("/api/fallback", HTTP_GET, jsonGet(handleApiFallbackGet));
#if defined(SSD1306_DISPLAY)
  server.on("/api/display", HTTP_GET, jsonGet(handleApiDisplayGet));
#endif
#if defined(SYSLOG)
  server.on("/api/syslog", HTTP_GET, jsonGet(handleApiSyslogGet));
#endif
#if defined(MQTT)
  server.on("/api/mqtt", HTTP_GET, jsonGet(handleApiMqttGet));
#endif
  server.on("/api/command", HTTP_POST, jsonPost(handleApiCommand));
  server.on("/api/action", HTTP_POST, jsonPost(handleApiAction));
  server.on("/api/wifi", HTTP_POST, jsonPost(handleApiWifiSet));
  server.on("/api/network", HTTP_POST, jsonPost(handleApiNetworkSet));
  server.on("/api/fallback", HTTP_POST, jsonPost(handleApiFallbackSet));
#if defined(SSD1306_DISPLAY)
  server.on("/api/display", HTTP_POST, jsonPost(handleApiDisplaySet));
#endif
#if defined(MQTT)
  server.on("/api/mqtt", HTTP_POST, jsonPost(handleApiMqttSet));
#endif
#if defined(SYSLOG)
  server.on("/api/syslog/test", HTTP_POST, jsonPost(handleApiSyslogTest));
  server.on("/api/syslog", HTTP_POST, jsonPost(handleApiSyslogSet));
#endif
  server.on("/api/firmware", HTTP_POST, handleFirmwareUpdate,
            handleFirmwareUpload);
  server.on("/api/filesystem", HTTP_POST, handleFilesystemUpdate,
            handleFilesystemUpload);
  server.on("/api/download/devices", HTTP_GET, handleDownloadDevices);
  server.on("/api/download/remotes", HTTP_GET, handleDownloadRemotes);
  server.on("/api/upload/devices", HTTP_POST, handleUploadDevicesDone,
            handleUploadDevicesFile);
  server.on("/api/upload/remotes", HTTP_POST, handleUploadRemotesDone,
            handleUploadRemotesFile);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  auto &staticHandler =
      server.serveStatic("/", LittleFS, "/web_interface_data/");
  staticHandler.setDefaultFile("index.html");
  staticHandler.setFilter([](AsyncWebServerRequest *request) {
    return !request->url().startsWith("/api");
  });
  // You might need to explicitly serve each file if serveStatic with directory
  // isn't working as expected or if files are not in a subdirectory of the data
  // dir. server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //     request->send(LittleFS, "/web_interface_data/index.html", "text/html");
  // });
  // server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  //     request->send(LittleFS, "/web_interface_data/style.css", "text/css");
  // });
  // server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
  //     request->send(LittleFS, "/web_interface_data/script.js",
  //     "application/javascript");
  // });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  ESP_LOGI(LOG_TAG, "HTTP server started");
}

void loopWebServer() {
  // For ESPAsyncWebServer, most work is done asynchronously.
  ws.cleanupClients();
}

#endif // defined(WEBSERVER)
