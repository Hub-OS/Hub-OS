#pragma once

#include "../bnInputManager.h"
#include "../bnSceneNode.h"
#include <SFML/Graphics.hpp>

namespace Overworld {
  class Menu : public sf::Drawable {
  protected:
    bool open = false;
  public:
    /**
    * @brief Query if the menu is should lock input when open
    * @return true if the menu locks input
    */
    virtual bool LocksInput() { return true; }

    /**
    * @brief Query if the menu is fullscreen when open
    * @return true if the menu is fullscreen when open
    */
    virtual bool IsFullscreen() { return false; }

    /**
    * @brief Query if the menu is open
    * @return true if the menu is open, false if any other state
    */
    bool IsOpen() const { return open; };

    /**
    * @brief Query if the menu is fully closed
    * @return true if the menu is fully closed, false if any other state
    */
    bool IsClosed() const { return !open; }

    /**
    * @brief Open the menu and begin the open animations
    */
    virtual void Open() { open = true; };

    /**
    * @brief Close the menu and begin the close animations
    */
    virtual void Close() { open = false; };

    virtual void Update(double elapsed) { };

    virtual void HandleInput(InputManager& input, sf::Vector2f mousePos) = 0;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const = 0;
  };
}
