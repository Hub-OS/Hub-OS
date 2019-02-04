#pragma once
#include <SFML\Graphics.hpp>
#include <vector>
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

class CustEmblem : public sf::Drawable, public sf::Transformable {
private:
  sf::Sprite emblem;
  sf::Sprite emblemWireMask;

  mutable sf::Shader* wireShader;

  struct WireEffect {
    int direction = 1; // 0 means reverse
    double progress;
    int index;
    sf::Color color;
  };

  int numWires;

  std::vector<WireEffect> effects;

public:
  CustEmblem() {
    numWires = 9;
    wireShader = &LOAD_SHADER(BADGE_WIRE);
    wireShader->setUniform("texture", sf::Shader::CurrentTexture);
    wireShader->setUniform("numOfWires", numWires);

    emblem.setTexture(LOAD_TEXTURE(CUST_BADGE));
    emblemWireMask.setTexture(LOAD_TEXTURE(CUST_BADGE_MASK));

    emblemWireMask.setPosition(-9.0f, -7.0f);
  }

  void CreateWireEffect() {

    sf::Color mix = sf::Color::White;

    WireEffect w;
    w.color = sf::Color::White; // todo make random
    w.direction = 1;
    w.index = rand() % numWires;
    w.progress = 0;

    effects.push_back(w);
  }

  void UndoWireEffect() {
    bool found = false;

    sf::Color red = sf::Color::White;

    if (effects.size() > 0) {
      // find a wire going forward and reverse it
      auto iter = effects.begin();
      while (iter != effects.end()) {
        if (iter->direction == 1) {
          iter->color = red;
          iter->direction = 0;
          found = true;
          break;
        }
        iter++;
      }
    }
    
    if(!found){
      WireEffect w;
      w.color = red; // todo make random
      w.direction = 0;
      w.index = rand() % numWires;
      w.progress = 1.0;

      effects.push_back(w);
    }
  }

  void Update(double elapsed) {
    for (auto iter = effects.begin(); iter != effects.end();) {
      if (iter->progress > 1.0) {
        iter = effects.erase(iter);
        continue;
      } else if (iter->progress < 0.0) {
        iter = effects.erase(iter);
        continue;
      }

      if (iter->direction) {
        iter->progress += elapsed*2.0;
      }
      else {
        iter->progress -= elapsed*2.0;
      }

      iter++;
    }
  }
  
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(emblem, states);

    states.shader = wireShader;

    for (auto& e : effects) {
      wireShader->setUniform("progress", (float)e.progress);
      wireShader->setUniform("index", e.index);
      wireShader->setUniform("inColor", sf::Glsl::Vec4(e.color));
      target.draw(emblemWireMask, states);
    }
  }
};