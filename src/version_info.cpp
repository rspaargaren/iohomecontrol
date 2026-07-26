#include <version_info.h>

#include <firmware_version.h>
#include <user_config.h>
#include <wifi_helper.h>

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <functional>

#if defined(MQTT)
#include <mqtt_handler.h>
#endif

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
}

namespace {

constexpr uint32_t VERSION_CHECK_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;
constexpr uint32_t VERSION_CHECK_RETRY_MS = 30000UL;
constexpr char GITHUB_RELEASE_URL_PREFIX[] =
    "https://api.github.com/repos/";

struct VersionInfoState {
  std::string version = firmwareVersion();
  std::string latestVersion;
  std::string releaseUrl;
  std::string error;
  bool currentIsDev = version == "DEV";
  bool checkCompleted = false;
  bool checkOk = false;
  bool updateAvailable = false;
  uint32_t lastCheckAtMs = 0;
};

SemaphoreHandle_t s_versionInfoMutex = nullptr;
TaskHandle_t s_versionInfoTask = nullptr;
VersionInfoState s_versionInfoState;
std::string s_globalCaPem;

// See extras/github-ca.md for how to refresh and validate this pinned CA.
extern const uint8_t _binary_extras_github_ca_pem_start[] asm("_binary_extras_github_ca_pem_start");
extern const uint8_t _binary_extras_github_ca_pem_end[] asm("_binary_extras_github_ca_pem_end");

bool hasElapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

std::string normalizeVersion(const std::string &version) {
  if (version.size() > 1 && (version[0] == 'v' || version[0] == 'V')) {
    return version.substr(1);
  }
  return version;
}

VersionInfoSnapshot snapshotLocked() {
  return {
      s_versionInfoState.version,
      s_versionInfoState.latestVersion,
      s_versionInfoState.releaseUrl,
      s_versionInfoState.error,
      s_versionInfoState.currentIsDev,
      s_versionInfoState.checkCompleted,
      s_versionInfoState.checkOk,
      s_versionInfoState.updateAvailable,
  };
}

VersionInfoSnapshot snapshot() {
  if (!s_versionInfoMutex) {
    return {firmwareVersion(), {}, {}, {}, std::string(firmwareVersion()) == "DEV",
            false, false, false};
  }

  xSemaphoreTake(s_versionInfoMutex, portMAX_DELAY);
  VersionInfoSnapshot result = snapshotLocked();
  xSemaphoreGive(s_versionInfoMutex);
  return result;
}

std::string buildReleaseApiUrl() {
  if (github_release_owner.empty() || github_release_repo.empty()) {
    return {};
  }

  return std::string(GITHUB_RELEASE_URL_PREFIX) + github_release_owner + "/" +
         github_release_repo + "/releases/latest";
}

void updateState(const std::function<void(VersionInfoState &)> &updater) {
  if (!s_versionInfoMutex) {
    return;
  }

  xSemaphoreTake(s_versionInfoMutex, portMAX_DELAY);
  updater(s_versionInfoState);
  xSemaphoreGive(s_versionInfoMutex);
}

void publishVersionInfoUpdate() {
#if defined(MQTT)
  publishVersionInfo();
#endif

  const auto versionInfo = getVersionInfo();
  Serial.printf(
      "Firmware update check results:\n"
      " - latestVersion: %s\n"
      " - hasUpdate: %s\n"
      " - checkSucceeded: %s\n"
      " - checkError: %s\n",
      versionInfo.latestVersion.c_str(),
      versionInfo.updateAvailable ? "true" : "false",
      versionInfo.checkOk ? "true" : "false",
      versionInfo.error.c_str()
    );
}

const char *githubCaPem() {
  const auto *bundleStart = _binary_extras_github_ca_pem_start;
  const auto *bundleEnd = _binary_extras_github_ca_pem_end;
  const auto bundleSize = static_cast<unsigned int>(bundleEnd - bundleStart);
  if (bundleSize == 0) {
    Serial.println("Failed to locate embedded CA bundle");
    return nullptr;
  }

  if (s_globalCaPem.empty()) {
    // Keep a stable NUL-terminated copy for WiFiClientSecure::setCACert().
    s_globalCaPem.assign(reinterpret_cast<const char *>(bundleStart), bundleSize);
  }

  if (s_globalCaPem.empty()) {
    Serial.println("Embedded CA bundle is empty");
    return nullptr;
  }

  if (s_globalCaPem.back() != '\0') {
    s_globalCaPem.push_back('\0');
  }

  return s_globalCaPem.c_str();
}

void checkLatestRelease() {
  const std::string url = buildReleaseApiUrl();
  const uint32_t now = millis();
  if (url.empty()) {
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = "GitHub release check not configured";
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  const char *caPem = githubCaPem();
  if (!caPem) {
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = "Failed to load GitHub root CA";
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  WiFiClientSecure client;
  client.setTimeout(15000);
  client.setCACert(caPem);

  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setUserAgent("iohomecontrol");
  if (!http.begin(client, url.c_str())) {
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = "Failed to initialize GitHub request";
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  http.addHeader("Accept", "application/vnd.github+json");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    const std::string requestErrorText = code > 0
        ? std::string("GitHub API returned HTTP ") + std::to_string(code)
        : std::string("GitHub request failed: ") + http.errorToString(code).c_str();
    http.end();
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = requestErrorText;
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  JsonDocument filter;
  filter["tag_name"] = true;
  filter["html_url"] = true;

  JsonDocument doc;
  Stream &responseStream = http.getStream();
  const auto error = deserializeJson(doc, responseStream,
                                     DeserializationOption::Filter(filter));
  http.end();

  if (error != DeserializationError::Ok) {
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = std::string(("Failed to parse GitHub release response: ") + std::string(error.c_str()));
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  const char *latestVersionValue = doc["tag_name"] | "";
  const char *releaseUrlValue = doc["html_url"] | "";
  const std::string latestVersion = latestVersionValue;
  const std::string releaseUrl = releaseUrlValue;
  if (latestVersion.empty()) {
    updateState([&](VersionInfoState &state) {
      state.checkCompleted = true;
      state.checkOk = false;
      state.updateAvailable = false;
      state.error = "GitHub release response missing tag_name";
      state.lastCheckAtMs = now;
    });
    publishVersionInfoUpdate();
    return;
  }

  const std::string normalizedCurrent = normalizeVersion(firmwareVersion());
  const std::string normalizedLatest = normalizeVersion(latestVersion);
  const bool currentIsDev = std::string(firmwareVersion()) == "DEV";
  const bool updateAvailable = !currentIsDev && !normalizedLatest.empty() &&
                               normalizedCurrent != normalizedLatest;

  updateState([&](VersionInfoState &state) {
    state.version = firmwareVersion();
    state.latestVersion = latestVersion;
    state.releaseUrl = releaseUrl;
    state.error.clear();
    state.currentIsDev = currentIsDev;
    state.checkCompleted = true;
    state.checkOk = true;
    state.updateAvailable = updateAvailable;
    state.lastCheckAtMs = now;
  });

  Serial.printf("Version check: current=%s latest=%s update=%s\n",
                firmwareVersion(), latestVersion.c_str(),
                updateAvailable ? "yes" : "no");
  publishVersionInfoUpdate();
}

void versionInfoTask(void * /*arg*/) {
  while (true) {
    uint32_t waitMs = VERSION_CHECK_RETRY_MS;
    const uint32_t now = millis();

    bool shouldCheck = false;
    if (s_versionInfoMutex && WiFi.status() == WL_CONNECTED) {
      xSemaphoreTake(s_versionInfoMutex, portMAX_DELAY);
      shouldCheck = !s_versionInfoState.checkCompleted ||
                    hasElapsed(now, s_versionInfoState.lastCheckAtMs,
                               VERSION_CHECK_INTERVAL_MS);
      if (s_versionInfoState.checkCompleted &&
          !hasElapsed(now, s_versionInfoState.lastCheckAtMs,
                      VERSION_CHECK_INTERVAL_MS)) {
        waitMs = VERSION_CHECK_INTERVAL_MS -
                 static_cast<uint32_t>(now - s_versionInfoState.lastCheckAtMs);
      }
      xSemaphoreGive(s_versionInfoMutex);
    }

    if (shouldCheck) {
      checkLatestRelease();
      waitMs = VERSION_CHECK_INTERVAL_MS;
    }

    vTaskDelay(pdMS_TO_TICKS(std::max<uint32_t>(1000UL, waitMs)));
  }
}

} // namespace

void initVersionInfo() {
  if (s_versionInfoTask) {
    return;
  }

  if (std::string(firmwareVersion()) == "DEV") {
    Serial.println("Disabling automatic version check for DEV versions. Only officially released versions have automated version checking.");
  } else {
    s_versionInfoMutex = xSemaphoreCreateMutex();
    if (!s_versionInfoMutex) {
      Serial.println("Failed to create version info mutex");
      return;
    }

    if (xTaskCreatePinnedToCore(versionInfoTask, "versionInfo", 8192, nullptr, 1,
                                &s_versionInfoTask, tskNO_AFFINITY) != pdPASS) {
      Serial.println("Failed to create version info task");
      vSemaphoreDelete(s_versionInfoMutex);
      s_versionInfoMutex = nullptr;
      s_versionInfoTask = nullptr;
    }
  }
}

VersionInfoSnapshot getVersionInfo() { return snapshot(); }

void appendVersionInfo(JsonObject &root) {
  const VersionInfoSnapshot info = snapshot();
  root["version"] = info.version;
  root["latest_version"] = info.latestVersion;
  root["release_url"] = info.releaseUrl;
  root["version_check_error"] = info.error;
  root["current_is_dev"] = info.currentIsDev;
  root["version_check_completed"] = info.checkCompleted;
  root["version_check_ok"] = info.checkOk;
  root["update_available"] = info.updateAvailable;
}
