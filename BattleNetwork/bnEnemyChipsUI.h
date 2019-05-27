#pragma once
#pragma once
#include <SFML/Graphics.hpp>
<<<<<<< HEAD
=======
#include <sstream>
#include <vector>

#include "bnChipUsePublisher.h"
#include "bnSceneNode.h"

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
<<<<<<< HEAD
#include <sstream>
using std::ostringstream;
#include <vector>
using std::vector;

#include "bnChipUsePublisher.h"
#include "bnSceneNode.h"
=======

using std::vector;
using std::ostringstream;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

class Character;
class Player;
class Chip;

<<<<<<< HEAD
=======
/**
 * @class EnemyChipsUI
 * @author mav
 * @date 05/05/19
 * @brief Similar to PlayerChipsUI, display chips over head of enemy
 * 
 * Uses AI to randomly use chip
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class EnemyChipsUI : public ChipUsePublisher, public Component, public SceneNode {
public:
  EnemyChipsUI(Character* owner);
  virtual ~EnemyChipsUI();

<<<<<<< HEAD
  void Update(float _elapsed);
  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  void LoadChips(std::vector<Chip> incoming);
  void UseNextChip();
=======
  /**
   * @brief Randomly uses a chip if the scene is active
   * @param _elapsed
   */
  void Update(float _elapsed);
  
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Inject(BattleScene&);
private:
  std::vector<Chip> selectedChips;
  int chipCount;
  int curr;
  Character* character;
  mutable sf::Sprite icon;
};
