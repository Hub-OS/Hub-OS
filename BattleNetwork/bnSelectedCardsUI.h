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
#include "bnSpriteProxyNode.h"
#include "bnInputHandle.h"
#include "bnText.h"

using std::ostringstream;
using std::vector;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;

class Entity;
class Player;
class Card;
class BattleSceneBase;

class SelectedCardsUI : public CardUsePublisher, public UIComponent, public InputHandle {
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
   * @return True if there was a card to use
   */
  const bool UseNextCard() override;
  
  /**
   * @brief nothing
   */
  void Inject(BattleSceneBase&) override;

  void SetMultiplier(unsigned mult);

private:
  float elapsed{}; /*!< Used by draw function, delta time since last update frame */
  Battle::Card** selectedCards{ nullptr }; /*!< Current list of cards. */
  int cardCount{}; /*!< Size of list */
  int curr{}; /*!< Card cursor index */
  unsigned multiplierValue{ 1 };
  mutable double interpolTimeFlat{}; /*!< Interpolation time for spread cards */
  mutable double interpolTimeDest{}; /*!< Interpolation time for default card stack */
  bool spread{ false }; /*!< If true, spread the cards, otherwise stack like the game */
  mutable bool firstFrame{ true }; /*!< If true, this UI graphic is being drawn for the first time*/
  sf::Time interpolDur; /*!< Max duration for interpolation 0.2 seconds */
  Player* player{ nullptr }; /*!< Player this component is attached to */
  Font font; /*!< Card name font */
  mutable Text text; /*!< Text displays card name */
  mutable Text multiplier;
  mutable Text dmg; /*!< Text displays card damage */
  mutable SpriteProxyNode icon;
  mutable SpriteProxyNode frame; /*!< Sprite for the card icon and the black border */
};
