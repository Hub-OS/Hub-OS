#pragma once
#include <SFML/Graphics.hpp>
using sf::Font;
using sf::Text;
class Character;

class MobHealthUI : public Text {
public:
  MobHealthUI(void) = delete;
  MobHealthUI(Character* _mob);
  ~MobHealthUI(void);

  void Update(float elapsed);

private:
  Character * mob;
  Font* font;
  int healthCounter;
  double cooldown;
  bool loaded;
};