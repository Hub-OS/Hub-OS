#include "bnEngine.h"
#include <time.h>       /* time */
#include <SFML/Window/ContextSettings.hpp>

#include "mmbn.ico.c"
#include "bnShaderType.h"
#include "bnShaderResourceManager.h"

Engine& Engine::GetInstance() {
  static Engine instance;
  return instance;
}

void Engine::Initialize() {
  // center, size
  view = sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320));
  cam = new Camera(view);
  window = nullptr;

#ifdef __ANDROID__
  // TODO: does the engine need to find the smallest or does this ratio work
  // auto videoMode = VideoMode::getFullscreenModes().front();
  videoMode.width = unsigned int(480.0f);
  videoMode.height = unsigned int(320.0f);
#else
  auto videoMode = VideoMode(480, 320);
#endif
  window = new RenderWindow(videoMode, "Battle Network: Progs Edition");

  this->Resize(view.getSize().x, view.getSize().y);

  window->setFramerateLimit(60);
  window->setMouseCursorVisible(false); // Hide cursor

  window->setIcon(sfml_icon.width, sfml_icon.height, sfml_icon.pixel_data);

  // See the random generator with current time
  srand((unsigned int)time(0));
}

void Engine::Draw(Drawable& _drawable, bool applyShaders) {
  if (!HasRenderSurface()) return;

  if (applyShaders) {
    auto stateCopy = state;

#ifdef __ANDROID__
    if(!stateCopy.shader) {
      stateCopy.shader = SHADERS.GetShader(ShaderType::DEFAULT);
    }
#endif 

    surface->draw(_drawable, stateCopy);
  } else {
    surface->draw(_drawable);
  }
}

void Engine::Draw(Drawable* _drawable, bool applyShaders) {
  if (!HasRenderSurface()) return;

  if (!_drawable) {
    return;
  }

  if (applyShaders) {
    auto stateCopy = state;

#ifdef __ANDROID__
    if(!stateCopy.shader) {
      stateCopy.shader = SHADERS.GetShader(ShaderType::DEFAULT);
    }
#endif

    surface->draw(*_drawable, stateCopy);
  } else {
    surface->draw(*_drawable);
  }
}

void Engine::Draw(SpriteSceneNode* _drawable) {
  if (!HasRenderSurface()) return;

  // For now, support at most one shader.
  // Grab the shader and image, apply to a new render target, pass this render target into Draw()

  SpriteSceneNode* context = _drawable;
  SmartShader* shader = &context->GetShader();

  if (shader && shader->Get()) {
    const sf::Texture* original = context->getTexture();
    shader->ApplyUniforms();

    sf::RenderStates newState = state;
    newState.shader = shader->Get();

    context->draw(*surface, newState);
    // surface->draw(*context, newState); // bake
    shader->ResetUniforms();
  } else {
    context->draw(*surface, state);
  }
}
void Engine::Draw(vector<SpriteSceneNode*> _drawable) {
  if (!HasRenderSurface()) return;

  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    /*
    NOTE: Could add support for multiple shaders:
    sf::RenderTexture image1;
    sf::RenderTexture image2;

    sf::RenderTexture* front = &image1;
    sf::RenderTexture* back = &image2;

    // draw the initial scene into "back"
    ...

    for (std::vector<sf::Shader>::iterator it = shaders.begin(); it != shaders.end(); ++it)
    {
    // draw "back" into "front"
    front->clear();
    front->draw(sf::Sprite(back->getTexture()), &*it);
    front->display();

    // swap front and back buffers
    std::swap(back, front)
    }

    */

    // For now, support at most one shader.
    // Grab the shader and image, apply to a new render target, pass this render target into Draw()

    SpriteSceneNode* context = *it;
    SmartShader& shader = context->GetShader();
    if (shader.Get() != nullptr) {
      shader.ApplyUniforms();

      sf::RenderStates newState = state;
      newState.shader = shader.Get();

      context->draw(*surface, newState);

      //surface->draw(*context, newState); // bake

      shader.ResetUniforms();
    } else {
      context->draw(*surface, state);
    }
  }
}

void Engine::Draw(vector<Drawable*> _drawable, bool applyShaders) {
  if (!HasRenderSurface()) return;

  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    Draw(*it, applyShaders);
  }
}

bool Engine::Running() {
  return window->isOpen();
}

void Engine::Clear() {
  if (HasRenderSurface()) {
    surface->clear();
  }

  window->clear();
}

RenderWindow* Engine::GetWindow() const {
  return window;
}

Engine::Engine()
{

  cam = new Camera(view);
}

Engine::~Engine() {
  delete window;
}

const sf::Vector2f Engine::GetViewOffset() {
  return GetDefaultView().getCenter() - cam->GetView().getCenter();
}

void Engine::SetShader(sf::Shader* shader) {
#ifdef __ANDROID__
  if (shader == nullptr) {
    state.shader = SHADERS.GetShader(ShaderType::DEFAULT);

    if(HasRenderSurface()) {
      surface->setDefaultShader(SHADERS.GetShader(ShaderType::DEFAULT));
    }
  } else {
    state.shader = shader;
  }
#else 
  state.shader = shader;
#endif
}

void Engine::RevokeShader() {
  SetShader(nullptr);
}

const bool Engine::IsMouseHovering(sf::Sprite & sprite) const
{
  sf::Vector2i mousei = sf::Mouse::getPosition(*window);
  sf::Vector2f mouse = window->mapPixelToCoords(mousei);
  sf::FloatRect bounds = sprite.getGlobalBounds();

  return (mouse.x >= bounds.left && mouse.x <= bounds.left + bounds.width && mouse.y >= bounds.top && mouse.y <= bounds.top + bounds.height);
}

void Engine::RegainFocus()
{

}

void Engine::Resize(int newWidth, int newHeight)
{
  float windowRatio = (float)newWidth / (float)newHeight;

  float viewRatio = view.getSize().x / view.getSize().y;
  float sizeX = 1;
  float sizeY = 1;
  float posX = 0;
  float posY = 0;

  bool horizontalSpacing = true;
  if (windowRatio < viewRatio) {
    horizontalSpacing = false;
  }

  // If horizontalSpacing is true, the black bars will appear on the left and right side.
  // Otherwise, the black bars will appear on the top and bottom.

  if (horizontalSpacing) {
    sizeX = viewRatio / windowRatio;
    posX = (1 - sizeX) / 2.f;

  }
  else {
    sizeY = windowRatio / viewRatio;
    posY = (1 - sizeY) / 2.f;
  }

  view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));

  window->setView(view);
}

const sf::View Engine::GetDefaultView() {
  return original;
}

Camera* Engine::GetCamera()
{
  return cam;
}

void Engine::SetCamera(Camera& camera) {
  // if (cam) delete cam;

  this->cam = &camera;
}

/*void Engine::SetView(sf::View v) {
  this->view = v;
}
*/
