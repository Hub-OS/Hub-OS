#pragma once
#include "../bnInputHandle.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PlayerController : public InputHandle {
    std::shared_ptr<Overworld::Actor> actor;
    bool listen{ true };
  public:
    void ControlActor(std::shared_ptr<Actor> actor);
    void ReleaseActor();
    void Update(double elapsed);
    void ListenToInputEvents(const bool listen);
  };
}