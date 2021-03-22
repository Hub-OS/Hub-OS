#include "bnOverworldSprite.h"

namespace Overworld {
  void WorldSprite::SetDepth(float depth) {
    SetLayer((int)depth);
    this->depth = depth;
  }

  float WorldSprite::GetDepth() const {
    return depth;
  }

  void WorldSprite::Set3DPosition(sf::Vector3f position) {
    setPosition(position.x, position.y);
    SetDepth(position.z);
  }

  sf::Vector3f WorldSprite::Get3DPosition() const {
    auto layerPosition = getPosition();
    return { layerPosition.x, layerPosition.y, depth };
  }

  void WorldSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= preTransform;
    SpriteProxyNode::draw(target, states);
  }
}
