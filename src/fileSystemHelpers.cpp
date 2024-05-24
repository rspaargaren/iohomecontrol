#include <fileSystemHelpers.h>
#include <board-config.h>

void printFileInfo(const char* dirName, const char* filePath, uint8_t level) {
  File file = LittleFS.open((String(dirName) + String(filePath)).c_str(), "r");
  if (!file.isDirectory()) {
    std::string depth((level), '\t');
    Serial.printf("%s", depth.c_str());
    Serial.print(filePath);
    Serial.print("\t\t");
    Serial.println(file.size());
  }
  file.close();
}

void traverseDirectory(const char* dirName, uint8_t level) {

#if defined(ESP32)
  File root = LittleFS.open(dirName);
  File fileName;
  while (fileName = root.openNextFile()) {
    if(fileName.isDirectory()){
      std::string depth((level), '\t');
      Serial.printf("%s", depth.c_str());
      Serial.printf("%s\n", fileName.name());
      traverseDirectory((String(dirName) + "/" + fileName).c_str(), level+1);
    } else {
      printFileInfo(dirName, fileName.name(), level);
    }
  }
#endif
}

void listFS() {
    traverseDirectory("/", 0);
}

void cat(const char *fname) {
    if (!LittleFS.exists(fname)) {
      Serial.printf("File %s does not exists\n\n", fname);
      return;
    }
    File file = LittleFS.open(fname, "r");
    String record = file.readString();
    Serial.printf("%s\n", record.c_str());
    file.close();
}

void rm(const char *fname) {
    if (!LittleFS.exists(fname)) {
      Serial.printf("File %s does not exists\n\n", fname);
      return;
    }
    LittleFS.remove(fname);
}