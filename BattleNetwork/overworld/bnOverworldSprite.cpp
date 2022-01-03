#include "bnOverworldSprite.h"

namespace Overworld {
  void WorldSprite::SetElevation(float elevation) {
    SetLayer((int)elevation);
    this->elevation = elevation;
  }

  float WorldSprite::GetElevation() const {
    return elevation;
  }

  void WorldSprite::Set3DPosition(sf::Vector3f position) {
    setPosition(position.x, position.y);
    SetElevation(position.z);
  }

  sf::Vector3f WorldSprite::Get3DPosition() const {
    auto layerPosition = getPosition();
    return { layerPosition.x, layerPosition.y, elevation };
  }

  void WorldSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= preTransform;
    SpriteProxyNode::draw(target, states);
  }
}
