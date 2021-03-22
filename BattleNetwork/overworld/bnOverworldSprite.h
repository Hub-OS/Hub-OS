#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include "../bnSpriteProxyNode.h"

namespace Overworld {
  class WorldSprite : public SpriteProxyNode {
  public:
    sf::Transform GetPreTransform() const { return preTransform; }
    void SetPreTransform(sf::Transform transform) { preTransform = transform; }

    void SetDepth(float depth);
    float GetDepth() const;

    void Set3DPosition(sf::Vector3f position);
    sf::Vector3f Get3DPosition() const;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
  private:
    sf::Transform preTransform;
    float depth{};
  };
}
