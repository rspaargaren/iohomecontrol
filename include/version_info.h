#ifndef VERSION_INFO_H
#define VERSION_INFO_H

#include <ArduinoJson.h>
#include <string>

struct VersionInfoSnapshot {
  std::string version;
  std::string latestVersion;
  std::string releaseUrl;
  std::string error;
  bool currentIsDev;
  bool checkCompleted;
  bool checkOk;
  bool updateAvailable;
};

void initVersionInfo();
VersionInfoSnapshot getVersionInfo();
void appendVersionInfo(JsonObject &root);

#endif // VERSION_INFO_H
