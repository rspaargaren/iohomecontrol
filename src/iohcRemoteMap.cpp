#include <iohcRemoteMap.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <iohcCryptoHelpers.h>
#include <iohcRemote1W.h>
#include <cstring>
#include <algorithm>

namespace IOHC {
    iohcRemoteMap* iohcRemoteMap::_instance = nullptr;

    static std::string resolveDevice(const std::string &device) {
        const auto &remotes = iohcRemote1W::getInstance()->getRemotes();
        for (const auto &r : remotes) {
            std::string id = bytesToHexString(r.node, sizeof(r.node));
            if (device == id || device == r.description) {
                return r.description;
            }
        }
        return device;
    }

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
                e.devices.push_back(resolveDevice(v.as<std::string>()));
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

    const std::vector<iohcRemoteMap::entry>& iohcRemoteMap::getEntries() const {
        return _entries;
    }

    bool iohcRemoteMap::save() {
        fs::File f = LittleFS.open(REMOTE_MAP_FILE, "w");
        if (!f) {
            Serial.println("Failed to open remote map for writing");
            return false;
        }
        JsonDocument doc;
        for (const auto &e : _entries) {
            auto jobj = doc[bytesToHexString(e.node, sizeof(e.node))].to<JsonObject>();
            jobj["name"] = e.name;
            auto jarr = jobj["devices"].to<JsonArray>();
            for (const auto &d : e.devices) {
                jarr.add(d);
            }
        }
        serializeJson(doc, f);
        f.close();
        return true;
    }

    bool iohcRemoteMap::add(const address node, const std::string &name) {
        if (find(node)) {
            Serial.println("Remote already exists");
            return false;
        }
        entry e{};
        memcpy(e.node, node, sizeof(address));
        e.name = name;
        _entries.push_back(e);
        return save();
    }

    bool iohcRemoteMap::linkDevice(const address node, const std::string &device) {
        std::string desc = resolveDevice(device);
        for (auto &e : _entries) {
            if (memcmp(e.node, node, sizeof(address)) == 0) {
                if (std::find(e.devices.begin(), e.devices.end(), desc) == e.devices.end()) {
                    e.devices.push_back(desc);
                    return save();
                }
                Serial.println("Device already linked");
                return false;
            }
        }
        Serial.println("Remote not found");
        return false;
    }

    bool iohcRemoteMap::unlinkDevice(const address node, const std::string &device) {
        std::string desc = resolveDevice(device);
        for (auto &e : _entries) {
            if (memcmp(e.node, node, sizeof(address)) == 0) {
                auto it = std::find(e.devices.begin(), e.devices.end(), desc);
                if (it != e.devices.end()) {
                    e.devices.erase(it);
                    return save();
                }
                Serial.println("Device not found");
                return false;
            }
        }
        Serial.println("Remote not found");
        return false;
    }

    bool iohcRemoteMap::renameDevice(const address node, const std::string &name) {
        auto it = std::find_if(_entries.begin(), _entries.end(),
                               [&](const entry &e) { return memcmp(e.node, node, sizeof(address)) == 0; });
        if (it == _entries.end()) {
            Serial.println("Remote not found");
            return false;
        }
        it->name = name;
        return save();
    }

    bool iohcRemoteMap::remove(const address node) {
        auto it = std::find_if(_entries.begin(), _entries.end(),
                               [&](const entry &e) { return memcmp(e.node, node, sizeof(address)) == 0; });
        if (it == _entries.end()) {
            Serial.println("Remote not found");
            return false;
        }
        _entries.erase(it);
        return save();
    }
}
