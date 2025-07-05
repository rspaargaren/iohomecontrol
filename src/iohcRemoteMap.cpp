#include <iohcRemoteMap.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <iohcCryptoHelpers.h>

namespace IOHC {
    iohcRemoteMap* iohcRemoteMap::_instance = nullptr;

    iohcRemoteMap* iohcRemoteMap::getInstance() {
        if (!_instance) {
            _instance = new iohcRemoteMap();
            _instance->load();
        }
        return _instance;
    }

    iohcRemoteMap::iohcRemoteMap() = default;

    bool iohcRemoteMap::load() {
        _entries.clear();
        if (!LittleFS.exists(REMOTE_MAP_FILE)) {
            Serial.printf("*remote map not available\n");
            return false;
        }
        fs::File f = LittleFS.open(REMOTE_MAP_FILE, "r");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, f);
        f.close();
        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return false;
        }
        for (JsonPair kv : doc.as<JsonObject>()) {
            entry e{};
            hexStringToBytes(kv.key().c_str(), e.node);
            JsonObject obj = kv.value().as<JsonObject>();
            e.name = obj["name"].as<std::string>();
            JsonArray jarr = obj["devices"].as<JsonArray>();
            for (auto v : jarr) {
                e.devices.push_back(v.as<std::string>());
            }
            _entries.push_back(e);
        }
        Serial.printf("Loaded %d remotes map\n", _entries.size());
        return true;
    }

    const iohcRemoteMap::entry* iohcRemoteMap::find(const address node) const {
        for (const auto &e : _entries) {
            if (memcmp(e.node, node, sizeof(address)) == 0)
                return &e;
        }
        return nullptr;
    }
}
