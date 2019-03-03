#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <algorithm>
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

class CustEmblem : public sf::Drawable, public sf::Transformable {
private:
  sf::Sprite emblem;
  sf::Sprite emblemWireMask;

  mutable sf::Shader* wireShader;

  struct WireEffect {
    double progress;
    int index;
    sf::Color color;
  };

  int numWires;

  std::deque<WireEffect> coming;
  std::deque<WireEffect> leaving;

public:
  CustEmblem();

  void CreateWireEffect();

  void UndoWireEffect();

  void Update(double elapsed);
  
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};