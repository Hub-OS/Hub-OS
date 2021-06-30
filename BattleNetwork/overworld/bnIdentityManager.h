#include <string>

namespace Overworld {
  /**
   * Identities should not be shared beyond a single server
   */
  class IdentityManager {
    public:
      IdentityManager(const std::string& address, uint16_t port);

      std::string GetIdentity();
    private:
      std::string path;
      std::string identity;
  };
}
