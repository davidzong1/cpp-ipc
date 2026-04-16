#include <cstdint>
#include <cstring>
#include "libipc/export.h"
#include "libipc/debug.h"
#include "libipc/buffer.h"
namespace ipc
{
    namespace socket
    {
        class UDPNode
        {
            UDPNode(const UDPNode &) = delete;
            UDPNode &operator=(const UDPNode &) = delete;

        public:
            UDPNode() {}
            /* Instantiation */
            UDPNode(const char *name, const char *ip, uint16_t port);
            void create(const char *name, const char *ip, uint16_t port) Exception;
            bool connect() Exception;
            bool send(std::vector<char> &data) Exception;
            bool receive(std::vector<char> &buffer, uint64_t tm = ipc::invalid_value) Exception;
            bool close() Exception;

        private:
            class UDPNode_;
            UDPNode_ *p_;
        };
    }
}