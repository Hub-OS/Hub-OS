#pragma once
#include "bnComponent.h"
#include "bnBattleScene.h"

#include <functional>


<<<<<<< HEAD
// T must be type of Entity
template<typename T>
class BattleOverTrigger : public Component {
private:
  BattleScene* scene;
  std::function<void(BattleScene&, T&)> callback;

public:
  BattleOverTrigger(T* owner, std::function<void(BattleScene&, T&)> callback) : Component(owner) {
=======
/**
 * @class BattleOverTrigger
 * @author mav
 * @date 13/05/19
 * @brief If an entity uses this component, will trigger callback when the battle is over
 * 
 * @warning T must be an entity
 * @warning This component is not owned by the scene and must be removed manually
 */
 
 template<typename T>
class BattleOverTrigger : public Component {
private:
  BattleScene* scene; /*!< Pointer to the battle scene */
  std::function<void(BattleScene&, T&)> callback; /*!< Triggers when battle is over */
  T* owner;
public:
  /**
   * @brief Sets the component owner and callback
   * @param owner
   */
  BattleOverTrigger(T* owner, std::function<void(BattleScene&, T&)> callback) : owner(owner), Component(owner) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    this->callback = callback;
    this->scene = nullptr;
  }

  virtual ~BattleOverTrigger() {

  }

<<<<<<< HEAD
  void Update(float _elapsed) {
    if (callback && this->GetOwner() && scene) {
      if (this->scene->IsCleared()) {
        callback(*this->scene, *(dynamic_cast<T*>(this->GetOwner())));
=======
  /**
   * @brief Polls until the scene is clear and then triggers once.
   * @param _elapsed
   */
  void Update(float _elapsed) {
    if (callback && this->GetOwner() && scene) {
      if (this->scene->IsCleared()) {
        // Cast back to type T
        callback(*this->scene, *this->owner);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
        callback = nullptr;
      }
    }
  }

<<<<<<< HEAD
=======
  /**
   * @brief Stores the scene object ptr
   * @param scene BattleScene ref
   * @warning This component is not owned by the battle scene and must be removed manually
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Inject(BattleScene& scene) {
    this->scene = &scene;
  }
};
