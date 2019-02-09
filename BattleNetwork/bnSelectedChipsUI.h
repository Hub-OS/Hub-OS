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
class BattleScene;

class SelectedChipsUI : public ChipUsePublisher, public Component {
public:
  SelectedChipsUI(Player* _player);
  ~SelectedChipsUI(void);

  bool GetNextComponent(Drawable*& out);
  void Update(float _elapsed);
  void LoadChips(Chip** incoming, int size);
  void UseNextChip();
  void Inject(BattleScene&);
private:
  Chip** selectedChips;
  int chipCount;
  int curr;
  double interpolTimeFlat;
  double interpolTimeDest;
  bool spread;
  sf::Time interpolDur;
  Player* player;
  Font* font;
  Text text;
  Text dmg;
  sf::Sprite icon, frame;
  vector<Drawable*> components;
};
