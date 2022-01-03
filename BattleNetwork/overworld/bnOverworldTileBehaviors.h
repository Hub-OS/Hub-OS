#pragma once

#include "bnOverworldSceneBase.h"
#include "bnOverworldActorPropertyAnimator.h"

namespace Overworld {
  namespace TileBehaviors {
    void UpdateActor(SceneBase&, Actor&, ActorPropertyAnimator&);
    void HandleConveyor(SceneBase&, Actor&, ActorPropertyAnimator&, TileMeta&, Tile&);
    void HandleIce(SceneBase&, Actor&, ActorPropertyAnimator&, TileMeta&, Tile&);
    void HandleTreadmill(SceneBase&, Actor&, ActorPropertyAnimator&, TileMeta&, Tile&);
  };
}
