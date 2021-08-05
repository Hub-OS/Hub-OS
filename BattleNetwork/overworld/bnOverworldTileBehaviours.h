#pragma once

#include "bnOverworldSceneBase.h"
#include "bnOverworldActorPropertyAnimator.h"

namespace Overworld {
  namespace TileBehaviours {
    void UpdateActor(SceneBase&, Actor&, ActorPropertyAnimator&);
    void HandleConveyor(SceneBase&, Actor&, ActorPropertyAnimator&, TileMeta&, Tile&);
  };
}
