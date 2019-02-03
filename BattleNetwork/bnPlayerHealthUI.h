#pragma once
#include <SFML/Graphics.hpp>
using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
#include <sstream>
using std::ostringstream;
#include <vector>
using std::vector;

class Entity;
class Player;

class PlayerHealthUI : public sf::Drawable{
public:
  PlayerHealthUI(Player* _player);
  ~PlayerHealthUI(void);

  bool GetNextComponent(Drawable*& out);
  void Update(float elapsed);
  void OffsetPosition(const sf::Vector2f offset); // Get rid of this eventually. See BattleScene.cpp line 241
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

private:
  int lastHP;
  int currHP;
  Player* player;
  Font* font;
  Text text;
  Sprite sprite;
  Texture* texture;
  vector<Drawable*> components;
  bool loaded;
  double cooldown;
};
