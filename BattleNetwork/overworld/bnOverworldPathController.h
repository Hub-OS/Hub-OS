#include <Swoosh/ActionList.h>

#include "../bnDrawWindow.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PathController {
    Overworld::Actor* actor{ nullptr };
    swoosh::ActionList actions;
    std::vector<std::function<void()>> commands;
    std::function<bool()> interruptCondition;
  public:
    class MoveToCommand : public swoosh::BlockingActionItem {
    private:
      sf::Vector2f dest;
      Overworld::Actor* actor{ nullptr };
    public:
      MoveToCommand(Overworld::Actor* actor, const sf::Vector2f& dest);
      void update(double elapsed) override;
      void draw(sf::RenderTexture& surface) override;
    };

    class WaitCommand : public swoosh::BlockingActionItem {
    private:
      frame_time_t frames;
      Overworld::Actor* actor{ nullptr };
    public:
      WaitCommand(Overworld::Actor* actor, const frame_time_t& frames);
      void update(double elapsed) override;
      void draw(sf::RenderTexture& surface) override;
    };

    void ControlActor(Actor& actor);
    void Update(double elapsed);
    void AddPoint(const sf::Vector2f& point);
    void AddWait(const frame_time_t& frames);
    void ClearPoints();
    void InterruptUntil(const std::function<bool()>& condition);
  };
}