#pragma once
#include "../bnInputHandle.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PlayerController : public InputHandle {
    Overworld::Actor* actor{ nullptr };
    bool listen{ true };
  public:
    void ControlActor(Actor& actor);
    void ReleaseActor();
    void Update(double elapsed);
    void ListenToInputEvents(const bool listen);
  };
}