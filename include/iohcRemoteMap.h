//   Copyright (c) 2025. CRIDP https://github.com/cridp
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//           http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifndef IOHC_REMOTE_MAP_H
#define IOHC_REMOTE_MAP_H

#include <iohcPacket.h>
#include <vector>
#include <string>

#define REMOTE_MAP_FILE "/RemoteMap.json"

namespace IOHC {
    class iohcRemoteMap {
    public:
        struct entry {
            address node;
            std::string name;
            std::vector<std::string> devices;
        };

        static iohcRemoteMap* getInstance();
        ~iohcRemoteMap() = default;

        const entry* find(const address node) const;
        bool load();

    private:
        iohcRemoteMap();
        static iohcRemoteMap* _instance;
        std::vector<entry> _entries;
    };
}

#endif //IOHC_REMOTE_MAP_H