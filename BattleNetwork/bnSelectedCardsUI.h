/*! \brief Shows the cards over the player and the name and damage at the bottom-left
 * 
 * Hold START to spread the cards out in FIFO order
 */
#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>
#include "bnUIComponent.h"
#include "bnCardUsePublisher.h"

using std::ostringstream;
using std::vector;
using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;

class Entity;
class Player;
class Card;
class BattleScene;

class SelectedCardsUI : public CardUsePublisher, public UIComponent {
public:
  /**
   * \brief Loads the graphics and sets spread duration to .2 seconds
   * \param _player the player to attach to
   */
  SelectedCardsUI(Player* _player);
  
  /**
   * @brief destructor
   */
  ~SelectedCardsUI();

  void draw(sf::RenderTarget & target, sf::RenderStates states) const override;

  /**
   * @brief Hold START to spread the cards out
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;
  
  /**
   * @brief Set the cards array and size. Updates card cursor to 0.
   * @param incoming List of Card pointers
   * @param size Size of List
   */
  void LoadCards(Battle::Card** incoming, int size);
  
  /**
   * @brief Broadcasts the card at the cursor curr. Increases curr.
   */
  void UseNextCard();
  
  /**
   * @brief nothing
   */
  void Inject(BattleScene&) override;

private:
  float elapsed; /*!< Used by draw function, delta time since last update frame */
  Battle::Card** selectedCards; /*!< Current list of cards. */
  int cardCount; /*!< Size of list */
  int curr; /*!< Card cursor index */
  mutable double interpolTimeFlat; /*!< Interpolation time for spread cards */
  mutable double interpolTimeDest; /*!< Interpolation time for default card stack */
  bool spread; /*!< If true, spread the cards, otherwise stack like the game */
  mutable bool firstFrame; /*!< If true, this UI graphic is being drawn for the first time*/
  sf::Time interpolDur; /*!< Max duration for interpolation 0.2 seconds */
  Player* player; /*!< Player this component is attached to */
  std::shared_ptr<Font> font; /*!< Card name font */
  mutable Text text; /*!< Text displays card name */
  mutable Text dmg; /*!< Text displays card damage */
  mutable SpriteProxyNode icon, frame; /*!< Sprite for the card icon and the black border */
};
