#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <algorithm>
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

/**
 * @class CustEmblem
 * @author mav
 * @date 05/05/19
 * @file bnCustEmblem.h
 * @brief Replaces symbol in chip select menu. Creates electricity that moves through wires for effect
 */
class CustEmblem : public sf::Drawable, public sf::Transformable {
private:
  sf::Sprite emblem; /*!< The emblem drawn in place */
  sf::Sprite emblemWireMask; /*!< The pixel color mask for electricity paths */

  mutable sf::Shader* wireShader; /*!< The shader that uses the mask and progress values */

  /**
   * @class WireEffect
   * @author mav
   * @date 05/05/19
   * @file bnCustEmblem.h
   * @brief Tracks wire states for immersion
   * 
   * If a wire moves forward and a chip is cancelled, the same wire is reversed
   */
  struct WireEffect {
    double progress; /*!< How far along the electricity is */
    int index; /*!< The index for the wires to animate */
    sf::Color color; /*!< The color of the electricity */
  };

  int numWires; /*!< How many wires the emblem has */

  std::deque<WireEffect> coming; /*!< List of wires moving forward */
  std::deque<WireEffect> leaving; /*!< List of wires reversed */

public:
  /**
   * @brief Loads graphics and sets relative position of the emblem
   */
  CustEmblem();

  /**
   * @brief Creates a new WireEffect state and adds it to coming list
   *
   */
  void CreateWireEffect();

  /**
   * @brief Places the last wire effect from coming into leaving 
   */
  void UndoWireEffect();
  
  /**
   * @brief When the wire is leaving and reaches total progress, remove the effect
   * @param elapsed in seconds
   */
  void Update(double elapsed);
  
  /**
   * @brief merge coming and leaving queues and draw the merged state data
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};