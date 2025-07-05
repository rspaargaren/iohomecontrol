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

#endif // IOHC_REMOTE_MAP_H
