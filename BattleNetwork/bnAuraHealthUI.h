#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>

#include "bnUIComponent.h"

class Entity;
class Player;

using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
using std::vector;
using std::ostringstream;

class AuraHealthUI :  public UIComponent {
public:
  AuraHealthUI(std::weak_ptr<Character> owner);
  ~AuraHealthUI();

  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;

  virtual void Update(double elapsed);
  virtual void Inject(BattleSceneBase& scene) { }

private:
  int lastHP;
  int currHP;
  int startHP;
  mutable Sprite font;
};