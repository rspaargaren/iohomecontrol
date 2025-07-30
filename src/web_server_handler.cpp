#include <web_server_handler.h>

#if defined(WEBSERVER)
#include "ESPAsyncWebServer.h" // Or WebServer.h if that's preferred for memory
#include "ArduinoJson.h"       // For creating JSON responses
#include <LittleFS.h>
#include <AsyncJson.h>
#include <log_buffer.h>
#include <iohcRemote1W.h>
#include <iohcCryptoHelpers.h>
// #include "main.h" // Or other relevant headers to access device data and command functions

// Assume ESPAsyncWebServer for now.
// If you use WebServer.h, the setup and request handling will be different.
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

// Placeholder for actual device data.
// In a real scenario, this would come from your device management logic.
struct Device {
    String id;
    String name;
};
Device devices[] = {
    {"dev1", "Living Room Thermostat"},
    {"dev2", "Bedroom Blind"},
    {"cmd_if", "Command Interface"} // A way to send generic commands if no specific device
};
const int numDevices = sizeof(devices) / sizeof(Device);

void handleApiDevices(AsyncWebServerRequest *request) {
    auto response = new AsyncJsonResponse();
    if(!response){
        request->send(500, "text/plain", "Out of memory");
        return;
    }
    auto root = response->getRoot().to<JsonArray>();

    const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
    for (const auto &r : remotes) { //(int i = 0; i < numDevices; i++) {
        auto deviceObj = root.add<JsonObject>();
        deviceObj["id"] = bytesToHexString(r.node, sizeof(r.node)).c_str(); //devices[i].id;
        deviceObj["name"] = r.name.c_str(); //devices[i].name;
    }

    // Provide a generic command interface as last entry
    auto cmdObj = root.add<JsonObject>();
    cmdObj["id"] = "cmd_if";
    cmdObj["name"] = "Command Interface";

    response->setLength();
    request->send(response);
    // log_i("Sent device list"); // Requires a logging library
}

void handleApiCommand(AsyncWebServerRequest *request, JsonVariant &json) {
    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }

    auto doc = json.as<JsonObject>();
    if (doc.isNull()) {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Invalid JSON\"}");
        return;
    }

    String deviceId = doc["deviceId"] | "";
    String command = doc["command"] | "";

    if (deviceId.isEmpty() || command.isEmpty()) {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Missing deviceId or command\"}");
        return;
    }

    // TODO: Process the command here
    // bool success = processDeviceCommand(deviceId, command);

    String message = "Command received for device " + deviceId + ": '" + command + "'. Processing not yet implemented.";
    bool success = true;

    auto response = new AsyncJsonResponse();
    if(!response){
        request->send(500, "application/json", "{\"success\":false,\"message\":\"OOM\"}");
        return;
    }
    auto root = response->getRoot().to<JsonObject>();
    root["success"] = success;
    root["message"] = message;

    response->setLength();
    request->send(response);
}

void handleApiLogs(AsyncWebServerRequest *request) {
    auto response = new AsyncJsonResponse();
    if(!response){
        request->send(500, "text/plain", "OOM");
        return;
    }
    auto root = response->getRoot().to<JsonArray>();
    auto logs = getLogMessages();
    for(const auto &msg : logs){
        root.add(msg);
    }
    response->setLength();
    request->send(response);
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

    server.serveStatic("/", LittleFS, "/web_interface_data/").setDefaultFile("index.html");
    // You might need to explicitly serve each file if serveStatic with directory isn't working as expected
    // or if files are not in a subdirectory of the data dir.
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //     request->send(LittleFS, "/web_interface_data/index.html", "text/html");
    // });
    // server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    //     request->send(LittleFS, "/web_interface_data/style.css", "text/css");
    // });
    // server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    //     request->send(LittleFS, "/web_interface_data/script.js", "application/javascript");
    // });


    // API Endpoints
// server.on("/api/devices", HTTP_GET, handleApiDevices);
// server.addHandler(new AsyncCallbackJsonWebHandler("/api/command", handleApiCommand)); // For POST with JSON
// server.on("/api/logs", HTTP_GET, handleApiLogs);

    server.onNotFound([](AsyncWebServerRequest *request){
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
