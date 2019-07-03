#pragma once
#pragma once
#include <SFML/Graphics.hpp>

#include <sstream>
#include <vector>

#include "bnChipUsePublisher.h"
#include "bnSceneNode.h"

using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;

using std::vector;
using std::ostringstream;

class Character;
class Player;
class Chip;

/**
 * @class EnemyChipsUI
 * @author mav
 * @date 05/05/19
 * @brief Similar to PlayerChipsUI, display chips over head of enemy
 * 
 * Uses AI to randomly use chip
 */
class EnemyChipsUI : public ChipUsePublisher, public Component, public SceneNode {
public:
  EnemyChipsUI(Character* owner);
  virtual ~EnemyChipsUI();

  /**
   * @brief Randomly uses a chip if the scene is active
   * @param _elapsed
   */
  void OnUpdate(float _elapsed);
  
  /**
   * @brief Draws chips stacked
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  
  /**
   * @brief Loads the next chips
   * @param incoming
   */
  void LoadChips(std::vector<Chip> incoming);
  
  /**
   * @brief Broadcasts the used chip
   */
  void UseNextChip();
  
  /**
   * @brief Injects itself as a chip publisher into the scene
   */
  void Inject(BattleScene&);
private:
  std::vector<Chip> selectedChips;
  int chipCount;
  int curr;
  Character* character;
  mutable sf::Sprite icon;
};
