#pragma once
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

#include "bnChipUsePublisher.h"

class Character;
class Player;
class Chip;

class EnemyChipsUI : public ChipUsePublisher, public Component {
public:
  EnemyChipsUI(Character* owner);
  virtual ~EnemyChipsUI(void);

  bool GetNextComponent(Drawable*& out);
  void Update(float _elapsed);
  void LoadChips(std::vector<Chip> incoming);
  void UseNextChip();
private:
  std::vector<Chip> selectedChips;
  int chipCount;
  int curr;
  Character* owner;
  sf::Sprite icon;
  vector<Drawable*> components;
};
