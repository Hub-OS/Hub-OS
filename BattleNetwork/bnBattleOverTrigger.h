#pragma once
#include "bnComponent.h"
#include "bnBattleScene.h"

#include <functional>


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
    this->callback = callback;
    this->scene = nullptr;
  }

  virtual ~BattleOverTrigger() {

  }

  /**
   * @brief Polls until the scene is clear and then triggers once.
   * @param _elapsed
   */
  void Update(float _elapsed) {
    if (callback && this->GetOwner() && scene) {
      if (this->scene->IsCleared()) {
        // Cast back to type T
        callback(*this->scene, *this->owner);
        callback = nullptr;
      }
    }
  }

  /**
   * @brief Stores the scene object ptr
   * @param scene BattleScene ref
   * @warning This component is not owned by the battle scene and must be removed manually
   */
  void Inject(BattleScene& scene) {
    this->scene = &scene;
  }
};
