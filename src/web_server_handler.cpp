#include "include/web_server_handler.h"
#include "ESPAsyncWebServer.h" // Or WebServer.h if that's preferred for memory
#include "ArduinoJson.h"       // For creating JSON responses
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
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonArray& root = response->getRoot().to<JsonArray>();

    for (int i = 0; i < numDevices; i++) {
        JsonObject deviceObj = root.createNestedObject();
        deviceObj["id"] = devices[i].id;
        deviceObj["name"] = devices[i].name;
    }
    
    response->setLength();
    request->send(response);
    // log_i("Sent device list"); // Requires a logging library
}

void handleApiCommand(AsyncWebServerRequest *request) {
    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }

    // Assuming data is sent as JSON. Max size 1024 bytes.
    // Adjust size if necessary.
    // The last argument (true) indicates that the data is JSON.
    // This is specific to ESPAsyncWebServer's JSON body parsing.
    // If not using ESPAsyncWebServer or its JSON plugin, parse manually.
    if (request->hasParam("body", true)) {
        JsonDocument doc; // Using ArduinoJson v6 syntax
        DeserializationError error = deserializeJson(doc, (uint8_t*)request->getParam("body", true)->value().c_str(), request->getParam("body", true)->value().length());

        if (error) {
            request->send(400, "application/json", "{\"success\":false, \"message\":\"Invalid JSON\"}");
            return;
        }

        String deviceId = doc["deviceId"];
        String command = doc["command"];

        if (deviceId.isEmpty() || command.isEmpty()) {
            request->send(400, "application/json", "{\"success\":false, \"message\":\"Missing deviceId or command\"}");
            return;
        }

        // TODO: Process the command here
        // For example, call a function from your main logic:
        // bool success = processDeviceCommand(deviceId, command);
        // String message = success ? "Command processed." : "Command failed.";
        
        // Placeholder response:
        String message = "Command received for device " + deviceId + ": '" + command + "'. Processing not yet implemented.";
        bool success = true; 
        // log_i("Received command: %s for device %s", command.c_str(), deviceId.c_str());


        AsyncJsonResponse* response = new AsyncJsonResponse();
        JsonObject& root = response->getRoot().to<JsonObject>();
        root["success"] = success;
        root["message"] = message;
        
        response->setLength();
        request->send(response);

    } else {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"No body\"}");
    }
}


void setupWebServer() {
    // Serve static files from /web_interface_data
    // Ensure this path matches where your platformio.ini places data files
    // or how you upload them (e.g., SPIFFS, LittleFS).
    // The path "/" serves index.html from the data directory.
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
    server.on("/api/devices", HTTP_GET, handleApiDevices);
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/command", handleApiCommand)); // For POST with JSON

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    // log_i("HTTP server started");
}

void loopWebServer() {
    // For ESPAsyncWebServer, most work is done asynchronously.
    // For the basic WebServer.h, you would need server.handleClient() here.
}
