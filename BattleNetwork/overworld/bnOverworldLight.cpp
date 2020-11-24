#include "bnOverworldLight.h"

namespace Overworld {
  Light::Light(sf::Vector2f pos, sf::Color diffuse, double radius) : pos(pos), diffuse(diffuse), radius(radius) {
    ;
  }

  void Light::SetPosition(sf::Vector2f _pos) { pos = _pos; }

  const sf::Vector2f Light::GetPosition() const { return pos; }
  const sf::Color Light::GetDiffuse() const { return diffuse; }
  const double Light::GetRadius() const { return radius; }
}