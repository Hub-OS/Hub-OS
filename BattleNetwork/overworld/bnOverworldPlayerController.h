#pragma once
#include "../bnGame.h"
#include "../bnInputHandle.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PlayerController : public InputHandle {
    Overworld::Actor* actor{ nullptr };
    bool listen{ true };

    struct {
      frame_time_t up{}, left{}, right{}, down{};
    } frameDelay{};

  public:
    void ControlActor(Actor& actor);
    void ReleaseActor();
    void Update(double elapsed);
    void ListenToInputEvents(const bool listen);
  };
}