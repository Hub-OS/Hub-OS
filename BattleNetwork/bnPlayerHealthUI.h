#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>

#include "bnBattleOverTrigger.h"
#include "bnPlayer.h"
#include "bnUIComponent.h"

class Entity;
class Player;

using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
using std::vector;
using std::ostringstream;

class PlayerHealthUI : virtual public UIComponent, virtual public BattleOverTrigger<Player> {
public:
  PlayerHealthUI(Player* _player);
  ~PlayerHealthUI();

  virtual void Inject(BattleScene& scene) { ; }

  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  void Update(float elapsed);

private:
  int lastHP;
  int currHP;
  int startHP;
  Player* player;
  mutable Sprite glyphs;
  Sprite sprite;
  Texture* texture;

  enum class Color : int {
    NORMAL,
    ORANGE,
    GREEN
  } color;

  bool loaded;
  bool isBattleOver;
  double cooldown;
};
