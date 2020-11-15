
/*! \brief This state can be used by any Entity in the engine.
 *
 *
 * This state spawns an explosion and flickers the
 * entity at it's current animation frame. Once the explosion
 * is finished, the entity is tried for deletion. Since
 * this state is used when health < 0, the deletion will
 * succeed.
 */

#pragma once
#include "bnExplodeState.h"
#include "bnShineExplosion.h"
#include "bnTextureResourceManager.h"

template<typename Any>
class NaviExplodeState : public ExplodeState<Any>
{
protected:
  ShineExplosion* shine{ nullptr }; /*!< Shine X that appears over navi ranked enemies */

public:
  inline static const int PriorityLevel = 0; // Highest

  /**
   * @brief Constructor
   * @param _numOfExplosions
   * @param _playbackSpeed
   * 
   * Calls super ExplodeState<Any>(_numOfExplosions, _playbackSpeed)
   */
  NaviExplodeState(int _numOfExplosions = 5, double _playbackSpeed = 1.0);
  
  /**
   * @brief deconstructor
   */
  virtual ~NaviExplodeState();

  /**
   * @brief Calls ExplodeState<Any>::OnEnter(e)
   * @param e entity
   * 
   * Adds shine effect on field
   */
  void OnEnter(Any& e);
  
  /**
   * @brief Calls ExplodeState<Any>::Update() and if e is deleted, removes the shine from the field
   * @param _elapsed in seconds
   * @param e entity
   */
  void OnUpdate(float _elapsed, Any& e);
  
  /**
   * @brief Calls ExplodeState<Any>::OnLeave(e)
   * @param e entity
   */
  void OnLeave(Any& e);
};

template<typename Any>
NaviExplodeState<Any>::NaviExplodeState(int _numOfExplosions, double _playbackSpeed) : 
  ExplodeState<Any>(_numOfExplosions, _playbackSpeed) {
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

  auto animation = e.template GetFirstComponent<AnimationComponent>();

  if (animation) {
    animation->SetPlaybackSpeed(0);
    animation->CancelCallbacks();
  }
}

template<typename Any>
void NaviExplodeState<Any>::OnUpdate(float _elapsed, Any& e) {
  ExplodeState<Any>::OnUpdate(_elapsed, e);

  if (e.WillRemoveLater()) {
    shine->Remove();
  }
}

template<typename Any>
void NaviExplodeState<Any>::OnLeave(Any& e) { 
  ExplodeState<Any>::OnLeave(e);
}
