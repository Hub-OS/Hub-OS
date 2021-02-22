#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include "../bnSpriteProxyNode.h"

namespace Overworld {
  class WorldSprite : public SpriteProxyNode {
    public:
      int GetLayer() const { return layer; }
      void SetLayer(int layer) { this->layer = layer; };

      sf::Transform GetPreTransform() const { return preTransform; }
      void SetPreTransform(sf::Transform transform) { preTransform = transform; }

      virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    private:
      sf::Transform preTransform;
      int layer{ 0 };
  };
}
