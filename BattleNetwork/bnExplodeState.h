#pragma once
#include "bnEntity.h"
#include "bnAIState.h"
#include "bnExplosion.h"
#include "bnShaderResourceManager.h"

/**
 * @class ExplodeState
 * @author mav
 * @date 04/05/19
 * @brief Locks an entity into this state and spawns explosions
 * 
 * This state can be used by any Entity in the engine. 
 * It uses constraints to ensure the type passed in Any 
 * is a subclass of Entity. 
 *
 * This state spawns an explosion and flickers the 
 * entity at it's current animation. Once the explosion
 * is finished, the entity is tried for deletion. Since 
 * this state is used when health < 0, the deletion will
 * succeed.
 */
template<typename Any>
class ExplodeState : public AIState<Any>
{
protected:
  Entity* explosion; /*!< The root explosion object */
  sf::Shader* whiteout; /*!< Flash the dying entity white */
  double elapsed;
  int numOfExplosions; /*!< Number of explosions to spawn */
  double playbackSpeed; /*!< how fast the animation should be */
public:

  ExplodeState(int _numOfExplosions=2, double _playbackSpeed=0.55);
  virtual ~ExplodeState();

  void OnEnter(Any& e);
  void OnUpdate(float _elapsed, Any& e);
  void OnLeave(Any& e);
};

#include "bnField.h"
#include "bnLogger.h"

template<typename Any>
ExplodeState<Any>::ExplodeState(int _numOfExplosions, double _playbackSpeed) 
  : numOfExplosions(_numOfExplosions), playbackSpeed(_playbackSpeed), AIState<Any>() {
  explosion = nullptr;

  whiteout = SHADERS.GetShader(ShaderType::WHITE);

  elapsed = 0;
}

template<typename Any>
ExplodeState<Any>::~ExplodeState() {
  /* explosion artifact is deleted by field */
}

template<typename Any>
void ExplodeState<Any>::OnEnter(Any& e) {
  e.LockState(); // Lock AI state. This is a final state.
  e.SetPassthrough(true); // Shoot through dying enemies

  /* Spawn an explosion */
  Battle::Tile* tile = e.GetTile();
  Field* field = e.GetField();
  explosion = new Explosion(field, e.GetTeam(), this->numOfExplosions, this->playbackSpeed);
  field->AddEntity(*(Artifact*)explosion, tile->GetX(), tile->GetY());
}

template<typename Any>
void ExplodeState<Any>::OnUpdate(float _elapsed, Any& e) {
  elapsed += _elapsed;

  /* freeze frame, flash white */
  if ((((int)(elapsed * 15))) % 2 == 0) {
    e.SetShader(whiteout);
  }
  else {
    e.SetShader(nullptr);
  }

  /* If explosion is over, delete the entity*/
  if (explosion->IsDeleted()) {
    e.TryDelete();
  }
}

template<typename Any>
void ExplodeState<Any>::OnLeave(Any& e) { }