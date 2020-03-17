#pragma once
#pragma once
#include <SFML/Graphics.hpp>

#include <sstream>
#include <vector>

#include "bnCardUsePublisher.h"
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
class Card;

/**
 * @class EnemyCardsUI
 * @author mav
 * @date 05/05/19
 * @brief Similar to PlayerCardsUI, display cards over head of enemy
 * 
 * Uses AI to randomly use card
 */
class EnemyCardsUI : public CardUsePublisher, public Component, public SceneNode {
public:
  EnemyCardsUI(Character* owner);
  virtual ~EnemyCardsUI();

  /**
   * @brief Randomly uses a card if the scene is active
   * @param _elapsed
   */
  void OnUpdate(float _elapsed);
  
  /**
   * @brief Draws cards stacked
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  
  /**
   * @brief Loads the next cards
   * @param incoming
   */
  void LoadCards(const std::vector<Battle::Card>& incoming);
  
  /**
   * @brief Broadcasts the used card
   */
  void UseNextCard();
  
  /**
   * @brief Injects itself as a card publisher into the scene
   */
  void Inject(BattleScene&);
private:
  std::vector<Battle::Card> selectedCards;
  int cardCount;
  int curr;
  Character* character;
  mutable sf::Sprite icon;
};
