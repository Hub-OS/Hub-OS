#pragma once
#include <SFML/Graphics.hpp>

namespace Overworld {
  /*! \brief Lightweight object that stores light properties */
  class Light {
    sf::Vector2f pos; /*!< position of the light */
    sf::Color diffuse; /*!< diffuse color of the light */
    double radius; /*!< Side of the light */

    public:
    /**
     * @brief constructor. Default radius is 60
     * @param _pos initial position of light
     * @param diffuse color of light
     * @param radius size of light
     */
    Light(sf::Vector2f pos, sf::Color diffuse, double radius = 20.f);

    /**
     * @brief Update the light's position
     * @param _pos
     */
    void SetPosition(sf::Vector2f _pos);

    /**
     * @brief Get current position of light
     * @return position
     */
    const sf::Vector2f GetPosition() const;
    
    /**
     * @brief Get color of light
     * @return color
     */
    const sf::Color GetDiffuse() const;
    
    /**
     * @brief Get size of light
     * @return radius
     */
    const double GetRadius() const;
  };
}
