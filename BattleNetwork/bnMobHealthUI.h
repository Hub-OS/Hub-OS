#pragma once
#include <SFML/Graphics.hpp>
using sf::Font;
using sf::Text;
class Character;

class MobHealthUI : public Text {
public:
  MobHealthUI(void);
  MobHealthUI(Character* _mob);
  ~MobHealthUI(void);

  void Update();

private:
  Character * mob;
  Font* font;
  int healthCounter;
  bool loaded;
};