#include <SFML/System.hpp>
#include <queue>
#include "../bnDirection.h"
#include "../bnSpriteProxyNode.h"
#include "../bnAnimation.h"
#include "../bnCallback.h"
#include "../bnEngine.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;

  class TeleportController {
    struct Command {
      enum class state {
        teleport_in = 0,
        teleport_out
      } const state{};

      Callback<void()> onFinish;
    };

    std::queue<Command> sequence;

    bool animComplete{ true }, walkoutComplete{ true }, spin{ false };
    bool mute{ false };
    float spinProgress{};
    frame_time_t walkFrames{};
    Overworld::Actor* actor{ nullptr };
    SpriteProxyNode beam{};
    Animation beamAnim;
    sf::Vector2f startPos{};
    Direction startDir;

  public:
    TeleportController();
    ~TeleportController() = default;

    Command& TeleportOut(Actor& actor);
    Command& TeleportIn(Actor& actor, const sf::Vector2f& start, Direction dir);
    void Update(double elapsed);
    const bool IsComplete() const;
    SpriteProxyNode& GetBeam();
    void EnableSound(bool enable);
  };
}