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

class Entity;
class Player;
class Chip;

class SelectedChipsUI : public ChipUsePublisher {
public:
  SelectedChipsUI(Player* _player);
  ~SelectedChipsUI(void);

  bool GetNextComponent(Drawable*& out);
  void Update();
  void LoadChips(Chip** incoming, int size);
  void UseNextChip();
private:
  Chip** selectedChips;
  int chipCount;
  int curr;
  Player* player;
  Font* font;
  Text text;
  Text dmg;
  sf::Sprite icon;
  vector<Drawable*> components;
};
