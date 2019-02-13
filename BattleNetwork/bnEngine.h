#pragma once
#include <SFML/Graphics.hpp>
using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
#include <vector>
using std::vector;

#include "bnCamera.h"
#include "bnLayered.h"

class Engine {
public:
  friend class ActivityManager;

  static Engine& GetInstance();
  void Initialize();
  void Draw(Drawable& _drawable, bool applyShaders = true);
  void Draw(Drawable* _drawable, bool applyShaders = true);
  void Draw(vector<Drawable*> _drawable, bool applyShaders = true);
  void Draw(LayeredDrawable * _drawable);
  void Draw(vector<LayeredDrawable*> _drawable);
  bool Running();
  void Clear();
  RenderWindow* GetWindow() const;

  void Push(LayeredDrawable* _drawable);
  void Lay(LayeredDrawable* _drawable);
  void Lay(vector<sf::Drawable*> _drawable);
  void LayUnder(sf::Drawable* _drawable);
  void DrawLayers();
  void DrawOverlay();
  void DrawUnderlay();

  void SetShader(sf::Shader* _shader);
  void RevokeShader();

  const bool IsMouseHovering(sf::Sprite& sprite) const;

  //void SetView(sf::View camera);
  void SetCamera(Camera& camera);

  const sf::View GetDefaultView();
  Camera* GetCamera();

  void SetRenderSurface(sf::RenderTexture& _surface) {
    surface = &_surface;
  }

  void SetRenderSurface(sf::RenderTexture* _surface) {
    surface = _surface;
  }

  const bool HasRenderSurface() {
    return (surface != nullptr);
  }

  sf::RenderTexture& GetRenderSurface() {
    return *surface;
  }

  // TODO: make this private again
  const sf::Vector2f GetViewOffset(); // for drawing 
private:
  Engine(void);
  ~Engine(void);

  RenderWindow* window;
  sf::View view;
  sf::View original;
  Underlay underlay;
  Layers layers;
  Overlay overlay;
  sf::RenderStates state;
  sf::RenderTexture* surface;
  Camera* cam;

};

#define ENGINE Engine::GetInstance()