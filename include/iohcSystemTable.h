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

#ifndef IOHC_SYSTEMTABLE_H
#define IOHC_SYSTEMTABLE_H

#include <map>
#include <string>
#include <iohcObject.h>

#define IOHC_SYS_TABLE  "/sysTable.json"

/*
    Singleton class to implement the System Object Table.
    System Object Table tracks all managed devices with their base info

    At this time this is only a container.
*/
namespace IOHC {
    class iohcSystemTable {
        public:
            using Objects = std::map<std::string, iohcObject *>;

            static iohcSystemTable *getInstance();
            virtual ~iohcSystemTable() = default;
            
            bool addObject(address node, address backbone, uint8_t actuator[2], uint8_t manufacturer, uint8_t flags);
            bool addObject(iohcObject *obj);
            bool addObject(std::string key, std::string serialized);

            bool empty();
            uint8_t size();
            void clear();
            inline Objects::iterator begin();
            inline Objects::iterator end();
            bool save(bool force = false);
            void dump1W();
            void dump2W();

        private:
            iohcSystemTable();
            bool load();
            bool changed = false;

            static iohcSystemTable *_iohcSystemTable;
            Objects _objects;

        protected:

    };
}
#endif