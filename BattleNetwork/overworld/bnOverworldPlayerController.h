#pragma once
#include "../bnGame.h"
#include "../bnInputHandle.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PlayerController : public InputHandle {
    std::shared_ptr<Overworld::Actor> actor;
    bool listen{ true };

    struct {
      frame_time_t up{}, left{}, right{}, down{};

      void reset() {
        up = left = right = down = frame_time_t{};
      }
    } frameDelay{}, releaseFrameDelay{};

  public:
    void ControlActor(std::shared_ptr<Actor> actor);
    void ReleaseActor();
    void Update(double elapsed);
    void ListenToInputEvents(const bool listen);
  };
}