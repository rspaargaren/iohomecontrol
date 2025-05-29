/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <fileSystemHelpers.h>
#include <board-config.h>

/**
 * The function `printFileInfo` prints information about a file in a specified directory with a given
 * indentation level.
 * 
 * @param dirName The `dirName` parameter is a pointer to a constant character array that represents
 * the directory name where the file is located.
 * @param filePath The `filePath` parameter is a string that represents the path to a file within a
 * directory. It is used to specify the location of the file whose information needs to be printed.
 * @param level The `level` parameter in the `printFileInfo` function is used to indicate the depth of
 * the file in the directory structure. It is of type `uint8_t`, which means it is an unsigned 8-bit
 * integer used to represent the level of nesting or depth in the directory structure.
 */
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

/**
 * The function `traverseDirectory` recursively traverses a directory on an ESP32 device, printing
 * information about files and subdirectories at different levels.
 * 
 * @param dirName The `dirName` parameter is a pointer to a constant character array that represents
 * the name of the directory to be traversed.
 * @param level The `level` parameter in the `traverseDirectory` function is used to keep track of the
 * depth of the directory traversal. It is incremented by 1 each time a subdirectory is encountered,
 * allowing the function to print the appropriate indentation for better visualization of the directory
 * structure.
 */
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

/**
 * The function `cat` reads and prints the contents of a file specified by the filename in C++.
 * 
 * @param fname The `fname` parameter in the `cat` function is a pointer to a constant character array,
 * which represents the file name of the file to be read and displayed.
 * 
 * @return The function `cat` is returning `void`, which means it does not return any value. It simply
 * prints the contents of the file specified by the `fname` parameter to the Serial monitor.
 */
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

/**
 * The function `rm` removes a file with the given filename if it exists in the LittleFS file system.
 * 
 * @param fname The parameter `fname` is a pointer to a constant character array, which represents the
 * file name that you want to remove using the `rm` function.
 * 
 * @return The function `rm` is returning `void`, which means it does not return any value.
 */
void rm(const char *fname) {
    if (!LittleFS.exists(fname)) {
      Serial.printf("File %s does not exists\n\n", fname);
      return;
    }
    LittleFS.remove(fname);
}