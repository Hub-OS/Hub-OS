#include "bnOverworldSprite.h"

namespace Overworld {
  void WorldSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= preTransform;
    SpriteProxyNode::draw(target, states);
  }
}
