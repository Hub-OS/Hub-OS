#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using std::vector;

#include "cxxopts/cxxopts.hpp"
#include "bnCamera.h"
#include "bnLayered.h"

/**
 * @class Engine
 * @author mav
 * @date 04/05/19
 * @brief Wrapper around SFML window and draw call API
 * 
 * Creates the window for the game
 * Uses Draw call across entire game to inject custom render states
 * and other effects into SFML draw calls
 */
class Engine {
public:
  enum class WindowMode : int {
    window,
    fullscreen
  };

  friend class ActivityManager;

  /**
   * @brief If this is the first call, creates the Engine singleton resource
   * @return Engine&
   */
  static Engine& GetInstance();
  
  /**
   * @brief Creates an SFML window and sets the icon
   * @param fullscreen. If false, the game launches in windowed mode.
   * @warning on mobile the activity is always the full window regardless of mode.
   */
  void Initialize(WindowMode);
  
  /**
   * @brief Draw an sf::Drawable through the engine pipeline
   * @param _drawable
   * @param applyShaders if true, applies a shader
   */
  void Draw(Drawable& _drawable, bool applyShaders = true);
  void Draw(Drawable* _drawable, bool applyShaders = true);
  
  /**
   * @brief Draws a batch of sf::Drawable through the engine pipeline
   * @param _drawable vector
   * @param applyShaders if true, applies a shader
   */
  void Draw(vector<Drawable*> _drawable, bool applyShaders = true);
  
  /**
   * @brief Draws a SpriteSceneNode through the engine pipeline
   * @param _drawable SpriteSceneNode*
   */
  void Draw(SpriteProxyNode * _drawable);
  
  /**
   * @brief Draws a batch of SpriteSceneNodes through the engine pipeline
   * @param _drawable vector of SpriteSceneNode*
   */
  void Draw(vector<SpriteProxyNode*> _drawable);
  
  /**
   * @brief Returns true if the window is open
   * @return true if window is open, false otherwise
   */
  bool Running();
  
  /**
   * @brief Dump the contents of the render window for a fresh redraw next frame
   */
  void Clear();
  
  /**
   * @brief Get the RenderWindow used by the app
   * @return RenderWindow*
   */
  RenderWindow* GetWindow() const;

  /**
   * @brief Store parsed command line values into the engine for easy access
   * @param values ParseResult object from cxxopts lib
   */
  void SetCommandLineValues(const cxxopts::ParseResult& values);

  /**
   * @brief Returns a value from the command line as type T
   * @param key String key for the command line arg
   * @return value for key as type T. If none are found T{} is returned.
   */
  template<typename T>
  const T CommandLineValue(const std::string& key) {
    for (auto&& keyval : commandline) {
      if (keyval.key() == key) {
        return keyval.as<T>();
      }
    }

    return T{};
  }

  /**
   * @brief Sets a post processing effect to be used on the screen
   * @param _shader
   */
  void SetShader(sf::Shader* _shader);
  
  /**
   * @brief Removes post processing effect 
   */
  void RevokeShader();

  /**
   * @brief Query if mouse is hovering over a sprite
   * @param sprite
   * @return true if the mouse is within the local bounds of the sprite
   */
  const bool IsMouseHovering(sf::Sprite& sprite) const;

  /**
  * @brief Describes how the engine should regain focus on diff hardware
  * 
  * Callback assigned to the InputManager */
  void RegainFocus();

  /**
  * @brief Describes how the engine should resize screen on diff hardware
  *
  * Callback assigned to the InputManager */
  void Resize(int newWidth, int newHeight);

  /**
   * @brief Creates a camera for the scene
   * @param camera
   * 
   * Camera's view offsets the screen drawing 
   */
  void SetCamera(Camera& camera);

  /**
   * @brief Make a copy of the current window view
   * 
   * useful for cameras to be exact size of window
   * @return const sf::View
   */
  const sf::View GetView();
  
  /**
   * @brief Get the camera object
   * @return Camera*
   */
  Camera* GetCamera();

  /**
   * @brief Sets the external render texture buffer to draw to
   * @param _surface
   */
  void SetRenderSurface(sf::RenderTexture& _surface) {
    surface = &_surface;
  }

  void SetRenderSurface(sf::RenderTexture* _surface) {
    surface = _surface;
  }

  /**
   * @brief Query if the engine has a buffer to draw to
   * @return true if surface is non null, false otherwise
   */
  const bool HasRenderSurface() {
    return (surface != nullptr);
  }

  /**
   * @brief Fetch the buffer the engine uses to draw to
   * @return sf::RenderTexture&
   */
  sf::RenderTexture& GetRenderSurface() {
    return *surface;
  }

  // TODO: make this private again
  const sf::Vector2f GetViewOffset(); // for drawing 
private:
  /**
   * @brief sets camera to nullptr
   */
  Engine();
  
  /**
    * @brief deletes the window */
  ~Engine();

  RenderWindow* window; /*!< Window created when app launches */
  sf::View view; /*!< Default view created when window launches */
  sf::View original; /*!< Default view created when window launches */
  sf::RenderStates state; /*!< Global GL context information used when drawing*/
  sf::RenderTexture* surface; /*!< The external buffer to draw to */
  std::vector<cxxopts::KeyValue> commandline; /*!< Values parsed from the command line*/
  Camera* cam; /*!< Camera object */
};

/**
 * @brief Shorter to type. Fetches instance of singleton.
 */
#define ENGINE Engine::GetInstance()

 // frames thunk transforms frames to milliseconds
static sf::Int32 frames(int frames)  {
  return (sf::Int32)(1000 * (float(frames) / 60.f));
};