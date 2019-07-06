/*! \brief Shows the chips over the player and the name and damage at the bottom-left
 * 
 * Hold START to spread the chips out in FIFO order
 */
#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>
#include "bnUIComponent.h"
#include "bnChipUsePublisher.h"

using std::ostringstream;
using std::vector;
using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;

class Entity;
class Player;
class Chip;
class BattleScene;

class SelectedChipsUI : public ChipUsePublisher, public UIComponent {
public:
  /**
   * \brief Loads the graphics and sets spread duration to .2 seconds
   * \param _player the player to attach to
   */
  SelectedChipsUI(Player* _player);
  
  /**
   * @brief destructor
   */
  ~SelectedChipsUI();

  void draw(sf::RenderTarget & target, sf::RenderStates states) const;

  /**
   * @brief Hold START to spread the chips out
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);
  
  /**
   * @brief Set the chips array and size. Updates chip cursor to 0.
   * @param incoming List of Chip pointers
   * @param size Size of List
   */
  void LoadChips(Chip** incoming, int size);
  
  /**
   * @brief Broadcasts the chip at the cursor curr. Increases curr.
   */
  void UseNextChip();
  
  /**
   * @brief nothing
   */
  void Inject(BattleScene&);
private:
  float elapsed; /*!< Used by draw function, delta time since last update frame */
  Chip** selectedChips; /*!< Current list of chips. */
  int chipCount; /*!< Size of list */
  int curr; /*!< Chip cursor index */
  mutable double interpolTimeFlat; /*!< Interpolation time for spread chips */
  mutable double interpolTimeDest; /*!< Interpolation time for default chip stack */
  bool spread; /*!< If true, spread the chips, otherwise stack like the game */
  sf::Time interpolDur; /*!< Max duration for interpolation 0.2 seconds */
  Player* player; /*!< Player this component is attached to */
  Font* font; /*!< Chip name font */
  mutable Text text; /*!< Text displays chip name */
  mutable Text dmg; /*!< Text displays chip damage */
  mutable sf::Sprite icon, frame; /*!< Sprite for the chip icon and the black border */
};
