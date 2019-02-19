#pragma once
#include "bnExplodeState.h"
#include "bnShineExplosion.h"
#include "bnTextureResourceManager.h"

/*
  This state can be used by any Entity in the engine.
  It uses constraints to ensure the type passed in Any
  is a subclass of Entity.

  This state spawns an explosion and flickers the
  entity at it's current animation. Once the explosion
  is finished, the entity is tried for deletion. Since
  this state is used when health < 0, the deletion will
  succeed.
*/
template<typename Any>
class NaviExplodeState : public ExplodeState<Any>
{
protected:
  ShineExplosion* shine;

public:

  NaviExplodeState(int _numOfExplosions = 2, double _playbackSpeed = 0.55);
  virtual ~NaviExplodeState();

  void OnEnter(Any& e);
  void OnUpdate(float _elapsed, Any& e);
  void OnLeave(Any& e);
};

template<typename Any>
NaviExplodeState<Any>::NaviExplodeState(int _numOfExplosions, double _playbackSpeed)
  : ExplodeState(_numOfExplosions, _playbackSpeed) {
}

template<typename Any>
NaviExplodeState<Any>::~NaviExplodeState() {
  /* explosion artifact is deleted by field */
}

template<typename Any>
void NaviExplodeState<Any>::OnEnter(Any& e) {
  ExplodeState<Any>::OnEnter(e);

  /* Spawn shine artifact */
  Battle::Tile* tile = e.GetTile();
  Field* field = e.GetField();
  shine = new ShineExplosion(field, e.GetTeam());
  field->AddEntity(*shine, tile->GetX(), tile->GetY());
}

template<typename Any>
void NaviExplodeState<Any>::OnUpdate(float _elapsed, Any& e) {
  ExplodeState<Any>::OnUpdate(_elapsed, e);

  if (e.IsDeleted()) {
    shine->Delete();
  }
}

template<typename Any>
void NaviExplodeState<Any>::OnLeave(Any& e) { 
  ExplodeState<Any>::OnLeave(e);
}