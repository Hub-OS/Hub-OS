#pragma once
#include <SFML/Graphics.hpp>
#include "bnUIComponent.h"
using sf::Font;
using sf::Text;
class Character;

class MobHealthUI : public UIComponent {
public:
  MobHealthUI(void) = delete;
  MobHealthUI(Character* _mob);
  ~MobHealthUI(void);

  virtual void Update(float elapsed);
  virtual void Inject(BattleScene& scene);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

private:
  Character * mob;
  sf::Color color;
  mutable sf::Sprite glyphs;
  int healthCounter;
  double cooldown;
  bool loaded;
};