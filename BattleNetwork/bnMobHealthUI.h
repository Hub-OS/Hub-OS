#pragma once
#include <SFML/Graphics.hpp>
#include "bnUIComponent.h"
#include "bnSpriteProxyNode.h"
#include "bnInputHandle.h"

// forward delcare
class Character;

/*! \brief Similar to PlayerHealthUI but draws under the mob */
class MobHealthUI : public UIComponent, InputHandle {
public:
  MobHealthUI() = delete;
  
  /**
   * @brief constructor character owns the component 
   */
  MobHealthUI(Character* _mob);
  
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

private:
  Character* mob{ nullptr }; /*!< Owner of health */
  sf::Color color; /*!< Color of the glyphs */
  mutable SpriteProxyNode glyphs; /*!< Drawable texture */
  int healthCounter; /*!< mob's current health */
  double cooldown; /*!< Time after dial to uncolorize */
};