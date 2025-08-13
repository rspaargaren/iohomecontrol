#include <web_server_handler.h>

#if defined(WEBSERVER)
#include "ArduinoJson.h"       // For creating JSON responses
#include "ESPAsyncWebServer.h" // Or WebServer.h if that's preferred for memory
#include <AsyncJson.h>
#include <LittleFS.h>
#include <interact.h>
#include <iohcCryptoHelpers.h>
#include <iohcRemote1W.h>
#include <log_buffer.h>
#include <mqtt_handler.h>
#include <nvs_helpers.h>
#include <tokens.h>
// #include "main.h" // Or other relevant headers to access device data and
// command functions

// Assume ESPAsyncWebServer for now.
// If you use WebServer.h, the setup and request handling will be different.
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

// Structure describing a device entry returned to the web UI
struct Device {
  String id;
  String name;
};

void handleApiDevices(AsyncWebServerRequest *request) {
  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "text/plain", "Out of memory");
    return;
  }
  JsonArray root = response->getRoot().to<JsonArray>();

  const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
  for (const auto &r : remotes) {
    JsonObject deviceObj = root.add<JsonObject>();
    deviceObj["id"] = bytesToHexString(r.node, sizeof(r.node)).c_str();
    deviceObj["name"] = r.name.c_str();
    deviceObj["position"] = r.positionTracker.getPosition();
    deviceObj["travel_time"] = r.travelTime;
  }

  // Provide a generic command interface as last entry
  // JsonObject cmdObj = root.add<JsonObject>();
  // cmdObj["id"] = "cmd_if";
  // cmdObj["name"] = "Command Interface";

  response->setLength();
  request->send(response);
  // log_i("Sent device list"); // Requires a logging library
}

void handleApiCommand(AsyncWebServerRequest *request, JsonVariant &json) {
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

  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "application/json",
                  "{\"success\":false,\"message\":\"OOM\"}");
    return;
  }
  JsonObject root = response->getRoot().to<JsonObject>();
  root["success"] = success;
  root["message"] = message;

  response->setLength();
  request->send(response);
}

void handleApiAction(AsyncWebServerRequest *request, JsonVariant &json) {
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

  String msg = "Action " + action + " sent to " + deviceId;
  addLogMessage(msg);

  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "application/json",
                  "{\"success\":false, \"message\":\"OOM\"}");
    return;
  }
  JsonObject root = response->getRoot().to<JsonObject>();
  root["success"] = true;
  root["message"] = msg;

  response->setLength();
  request->send(response);
}

void handleApiLogs(AsyncWebServerRequest *request) {
  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "text/plain", "OOM");
    return;
  }
  JsonArray root = response->getRoot().to<JsonArray>();
  auto logs = getLogMessages();
  for (const auto &msg : logs) {
    root.add(msg);
  }
  response->setLength();
  request->send(response);
}

#if defined(MQTT)
void handleApiMqttGet(AsyncWebServerRequest *request) {
  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "text/plain", "OOM");
    return;
  }
  JsonObject root = response->getRoot().to<JsonObject>();
  root["server"] = mqtt_server.c_str();
  root["user"] = mqtt_user.c_str();
  root["password"] = mqtt_password.c_str();
  root["discovery"] = mqtt_discovery_topic.c_str();
  response->setLength();
  request->send(response);
}

void handleApiMqttSet(AsyncWebServerRequest *request, JsonVariant &json) {
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

  String server = doc["server"] | "";
  String user = doc["user"] | "";
  String password = doc["password"] | "";
  String discovery = doc["discovery"] | "";

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

  if (mqttChanged) {
    mqttClient.disconnect();
    mqttClient.setServer(mqtt_server.c_str(), 1883);
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_password.c_str());
  }

  if (discChanged && mqttStatus == ConnState::Connected) {
    handleMqttConnect();
  }

  AsyncJsonResponse *response = new AsyncJsonResponse();
  if (!response) {
    request->send(500, "application/json",
                  "{\"success\":false,\"message\":\"OOM\"}");
    return;
  }
  JsonObject root = response->getRoot().to<JsonObject>();
  root["success"] = true;
  root["message"] = "MQTT configuration updated";
  response->setLength();
  request->send(response);
}
#endif

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
  server.on("/api/devices", HTTP_GET, handleApiDevices);
  server.on("/api/logs", HTTP_GET, handleApiLogs);
#if defined(MQTT)
  server.on("/api/mqtt", HTTP_GET, handleApiMqttGet);
#endif
  server.addHandler(new AsyncCallbackJsonWebHandler(
      "/api/command", handleApiCommand)); // For POST with JSON
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/action", handleApiAction));
#if defined(MQTT)
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/mqtt", handleApiMqttSet));
#endif

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
  // For the basic WebServer.h, you would need server.handleClient() here.
}

#endif // defined(WEBSERVER)
