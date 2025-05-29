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

// #pragma once
#ifndef FILESYSTEMHELPERS_H
#define FILESYSTEMHELPERS_H

//#include <Arduino.h>
#include <string>
#include <LittleFS.h>

#if defined(ESP32)
    #include <FS.h>
#endif

void listFS();
void cat(const char *fname);
void rm(const char *fname);
#endif // FILESYSTEMHELPERS_H
