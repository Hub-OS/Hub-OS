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
#include "bnSceneNode.h"

class Character;
class Player;
class Chip;

class EnemyChipsUI : public ChipUsePublisher, public Component, public SceneNode {
public:
  EnemyChipsUI(Character* owner);
  virtual ~EnemyChipsUI();

  bool GetNextComponent(Drawable*& out);
  void Update(float _elapsed);
  void OnDraw();
  void LoadChips(std::vector<Chip> incoming);
  void UseNextChip();
  void Inject(BattleScene&);
private:
  std::vector<Chip> selectedChips;
  int chipCount;
  int curr;
  Character* owner;
  sf::Sprite icon;
  vector<Drawable*> components;
};
