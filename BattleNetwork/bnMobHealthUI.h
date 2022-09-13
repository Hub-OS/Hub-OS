#pragma once
#include <SFML/Graphics.hpp>
#include "bnUIComponent.h"
#include "bnSpriteProxyNode.h"
#include "bnInputHandle.h"
#include "bnFrameTimeUtils.h"

// forward delcare
class Character;

/*! \brief Similar to PlayerHealthUI but draws under the mob */
class MobHealthUI : public UIComponent, InputHandle {
public:
  MobHealthUI() = delete;
  
  /**
   * @brief constructor character owns the component 
   */
  MobHealthUI(std::weak_ptr<Character> _mob);
  
  /**
   * @brief deconstructor
   */
  ~MobHealthUI();

  /**
   * @brief Dials health to the mob's current health and colorizes
   * @param elapsed
   */
  void OnUpdate(double elapsed) override;
  
  /**
   * @brief UI is drawn lest and must be injected into the battle scene
   * @param scene
   * 
   * Frees the owner of the component of resource management
   * but still maintains the pointer to the character
   */
  void Inject(BattleSceneBase& scene) override;
  
  /**
   * @brief Uses bitmap glyphs to draw game accurate health
   * @param target
   * @param states
   */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /**
  * @brief Sets the hp manually
  * @param hp manual hp value
  */
  void SetHP(int hp);

  /**
  * @brief Requires that the hp value is set manually
  * @param true to enable, false for default behavior (reads entity's hp)
  * @warning value will NOT reflect the underlining entity's REAL hp
  *
  * If not set to manual mode, the hp will update from the entity's real hp
  */
  void SetManualMode(bool enabled);

private:
  sf::Color color; /*!< Color of the glyphs */
  mutable SpriteProxyNode glyphs; /*!< Drawable texture */
  int healthCounter{}; /*!< UI current health */
  int targetHealth{}; /*!< UI target health (mob real hp) */
  double cooldown{}; /*!< Time after dial to uncolorize */
  bool manualMode{}; /*!< Default is automatic from the entity's hp*/
  frame_time_t hideCountdown{ frames(0) };
};