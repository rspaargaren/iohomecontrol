#include <web_server_handler.h>

#if defined(WEBSERVER)
#include "ArduinoJson.h"       // For creating JSON responses
#include "ESPAsyncWebServer.h" // Or WebServer.h if that's preferred for memory
#include <AsyncJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <cstdlib>
#include <interact.h>
#include <iohcCryptoHelpers.h>
#include <iohcRemote1W.h>
#include <iohcRemoteMap.h>
#include <iohcPacket.h>
#include <log_buffer.h>
#include <mqtt_handler.h>
#include <nvs_helpers.h>
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif
#include <tokens.h>
// #include "main.h" // Or other relevant headers to access device data and
// command functions

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
    AsyncJsonResponse *response = new AsyncJsonResponse();
    if (!response) {
      request->send(500, "text/plain", "Internal Server Error");
      return;
    }

    handler(request, response->getRoot());

    if (!request->isSent()) {
      response->setLength();
      request->send(response);  // transfers response delete responsibility to request
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

    AsyncJsonResponse *response = new AsyncJsonResponse();
    if (!response) {
      request->send(500, "text/plain", "Internal Server Error");
      return;
    }

    handler(request, doc, response->getRoot());

    if (!request->isSent()) {
      response->setLength();
      request->send(response); // transfers response delete responsibility to request
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
  std::sort(remotes.begin(), remotes.end(), [](const auto r1, const auto r2) { return r1.name.compare(r2.name) < 0; });
  for (const auto &r : remotes) {
    JsonObject deviceObj = root.add<JsonObject>();
    deviceObj["id"] = bytesToHexString(r.node, sizeof(r.node)).c_str();
    deviceObj["name"] = r.name.c_str();
    deviceObj["description"] = r.description.c_str();
    deviceObj["position"] = r.positionTracker.getPosition();
    deviceObj["travel_time"] = r.travelTime;
    deviceObj["paired"] = r.paired;
  }

  // Provide a generic command interface as last entry
  // JsonObject cmdObj = root.add<JsonObject>();
  // cmdObj["id"] = "cmd_if";
  // cmdObj["name"] = "Command Interface";

  // log_i("Sent device list"); // Requires a logging library
}

void handleApiRemotes(AsyncWebServerRequest *request, JsonArray &root) {
  auto entries = IOHC::iohcRemoteMap::getInstance()->getEntries();
  std::sort(entries.begin(), entries.end(), [](const auto e1, const auto e2) { return e1.name.compare(e2.name) < 0; });
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
    auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
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
  auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
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

void handleApiLogs(AsyncWebServerRequest *request, JsonArray &root) {
  auto logs = getLogMessages();
  for (const auto &msg : logs) {
    root.add(msg);
  }
}

void handleApiLastAddr(AsyncWebServerRequest *request, JsonObject &root) {
  root["address"] = bytesToHexString(IOHC::lastFromAddress, sizeof(IOHC::lastFromAddress)).c_str();
}

#if defined(SYSLOG)
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

void handleApiSyslogGet(AsyncWebServerRequest *request, JsonObject &root) {
  initSyslog();

  root["enabled"] = syslog_enabled;
  root["server"] = syslog_server.c_str();
  root["port"] = syslog_port;
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
    delay(100);
    ESP.restart();
  }
}

void handleFirmwareUpload(AsyncWebServerRequest *request, String filename,
                          size_t index, uint8_t *data, size_t len,
                          bool final) {
  if (!index) {
    Serial.printf("Firmware upload start: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }
  if (final) {
    if (!Update.end(true)) {
      Update.printError(Serial);
    }
  }
}

void handleFilesystemUpdate(AsyncWebServerRequest *request) {
  if (Update.hasError()) {
    request->send(500, "application/json",
                  "{\"message\":\"Filesystem update failed\"}");
  } else {
    request->send(200, "application/json",
                  "{\"message\":\"Filesystem update successful, rebooting\"}");
    delay(100);
    ESP.restart();
  }
}

void handleFilesystemUpload(AsyncWebServerRequest *request, String filename,
                            size_t index, uint8_t *data, size_t len,
                            bool final) {
  if (!index) {
    Serial.printf("Filesystem upload start: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
      Update.printError(Serial);
    }
  }
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }
  if (final) {
    if (!Update.end(true)) {
      Update.printError(Serial);
    }
  }
}

void setupWebServer() {
  Serial.println("Initializing HTTP server ...");

  // Serve static files from /web_interface_data
  // Ensure this path matches where your platformio.ini places data files
  // or how you upload them (e.g., SPIFFS, LittleFS).
  // The path "/" serves index.html from the data directory.
  if (!LittleFS.exists("/web_interface_data/index.html")) {
    Serial.println("Warning: /web_interface_data/index.html not found");
  }

  // API Endpoints
  server.on("/api/devices", HTTP_GET, jsonGet(handleApiDevices));
  server.on("/api/remotes", HTTP_GET, jsonGet(handleApiRemotes));
  server.on("/api/logs", HTTP_GET, jsonGet(handleApiLogs));
  server.on("/api/lastaddr", HTTP_GET, jsonGet(handleApiLastAddr));
#if defined(SYSLOG)
  server.on("/api/syslog", HTTP_GET, jsonGet(handleApiSyslogGet));
#endif
#if defined(MQTT)
  server.on("/api/mqtt", HTTP_GET, jsonGet(handleApiMqttGet));
#endif
  server.on("/api/command", HTTP_POST, jsonPost(handleApiCommand));
  server.on("/api/action", HTTP_POST, jsonPost(handleApiAction));
#if defined(MQTT)
  server.on("/api/mqtt", HTTP_POST, jsonPost(handleApiMqttSet));
#endif
#if defined(SYSLOG)
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
  Serial.println("HTTP server started");
}

void loopWebServer() {
  // For ESPAsyncWebServer, most work is done asynchronously.
  ws.cleanupClients();
}

#endif // defined(WEBSERVER)
