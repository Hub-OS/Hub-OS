#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>

#include "bnBattleOverTrigger.h"
#include "bnPlayer.h"
#include "bnSceneNode.h"

class Entity;
class Player;

using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
using std::vector;
using std::ostringstream;

class PlayerHealthUI : public SceneNode, public BattleOverTrigger<Player> {
public:
  PlayerHealthUI(Player* _player);
  ~PlayerHealthUI();

  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;

  void Update(float elapsed);
  void OffsetPosition(const sf::Vector2f offset); // Get rid of this eventually. See BattleScene.cpp line 241

private:
  int lastHP;
  int currHP;
  int startHP;
  Player* player;
  Font* font;
  Text text;
  Sprite sprite;
  Texture* texture;
  bool loaded;
  bool isBattleOver;
  double cooldown;
};
