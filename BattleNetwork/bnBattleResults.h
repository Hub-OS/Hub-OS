#pragma once

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

class Mob;
class BattleItem;

class BattleResults {
private:
  sf::Sprite resultsSprite;
  sf::Text time;
  sf::Text rank;
  sf::Text reward;
  sf::Text chipCode;
  sf::Sprite rewardCard;

  bool isHidden;
  bool isRevealed;
  bool rewardIsChip;

  BattleItem* item;
  int score; // 1-10 or 11+ as S rank

  // utilities 
  std::string FormatString(sf::Time time);

public:
  BattleResults(sf::Time battleLength, int moveCount, int hitCount, int counterCount, bool doubleDelete, bool tripleDelete, Mob* mob);
  ~BattleResults();

  // GUI ops
  bool CursorAction();

  bool IsOutOfView();
  bool IsInView();
  void Move(sf::Vector2f delta);
  void Draw();
  bool IsFinished();

  // Get the reward 
  BattleItem* GetReward();
};

