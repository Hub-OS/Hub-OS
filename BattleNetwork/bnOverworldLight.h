#pragma once
#include <SFML/Graphics.hpp>

namespace Overworld {
  class Light {
    sf::Vector2f pos;
    sf::Color diffuse;
    double radius;

  public:
    Light(sf::Vector2f pos, sf::Color diffuse, double radius = 60.f);

    void SetPosition(sf::Vector2f _pos);

    const sf::Vector2f GetPosition() const;
    const sf::Color GetDiffuse() const;
    const double GetRadius() const;
  };
}
