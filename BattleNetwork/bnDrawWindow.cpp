#include "bnDrawWindow.h"
#include <time.h>
#include <SFML/Window/ContextSettings.hpp>

#include "mmbn.ico.c"
#include "bnShaderType.h"
#include "bnShaderResourceManager.h"

void DrawWindow::Initialize(DrawWindow::WindowMode mode) {
  // center, size
  view = sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320));
  cam = std::make_shared<Camera>(view);

  if (window) delete window;
  window = nullptr;

#ifdef __ANDROID__
  // TODO: does the engine need to find the smallest or does this ratio work
  // auto videoMode = VideoMode::getFullscreenModes().front();
  videoMode.width = unsigned int(480.0f);
  videoMode.height = unsigned int(320.0f);
#else
  auto videoMode = VideoMode(480, 320);
#endif
  auto style = sf::Style::Default;

  if (mode == WindowMode::fullscreen) {
      style = sf::Style::Fullscreen;
  }

  window = new RenderWindow(videoMode, "Battle Network: Progs Edition", style);

  Resize((int)view.getSize().x, (int)view.getSize().y);

  window->setFramerateLimit(60);
  window->setMouseCursorVisible(false); // Hide cursor

  window->setIcon(sfml_icon.width, sfml_icon.height, sfml_icon.pixel_data);

  // See the random generator with current time
  srand((unsigned int)time(0));
}

void DrawWindow::Draw(Drawable& _drawable, bool applyShaders) {
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

void DrawWindow::Draw(SpriteProxyNode& _drawable) {
  if (!HasRenderSurface()) return;

  // For now, support at most one shader.
  // Grab the shader and image, apply to a new render target, pass this render target into Draw()

  SpriteProxyNode* context = &_drawable;
  SmartShader* shader = &context->GetShader();

  if (shader && shader->Get()) {
    shader->ApplyUniforms();

    sf::RenderStates newState = state;
    newState.shader = shader->Get();

    context->draw(*surface, newState);

    shader->ResetUniforms();
  } else {
    context->draw(*surface, state);
  }
}
void DrawWindow::Draw(vector<SpriteProxyNode*> _drawable) {
  if (!HasRenderSurface()) return;

  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    /*
    NOTE: Could add support for multiple shaders:
    NOTE 12/7/2020: Use G buffers
    
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

    SpriteProxyNode* context = *it;
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

void DrawWindow::Draw(vector<Drawable*> _drawable, bool applyShaders) {
  if (!HasRenderSurface()) return;

  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    Draw(*(*it), applyShaders);
  }
}

bool DrawWindow::Running() {
  return window->isOpen();
}

void DrawWindow::Clear() {
  if (HasRenderSurface()) {
    surface->clear();
  }

  window->clear();
}

RenderWindow* DrawWindow::GetRenderWindow() const {
  return window;
}

void DrawWindow::SetCommandLineValues(const cxxopts::ParseResult& values)
{
  commandline = values.arguments();

  if (commandline.empty()) return;

  Logger::Log("Command line args provided");
  for (auto&& kv : commandline) {
    Logger::Logf("%s : %s", kv.key().c_str(), kv.value().c_str());
  }
}

DrawWindow::DrawWindow()
{
  window = nullptr;
}

DrawWindow::~DrawWindow() {
  delete window;
}

const sf::Vector2f DrawWindow::GetViewOffset() {
  return GetView().getCenter() - cam->GetView().getCenter();
}

void DrawWindow::SetShader(sf::Shader* shader) {
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

void DrawWindow::RevokeShader() {
  SetShader(nullptr);
}

const bool DrawWindow::IsMouseHovering(sf::Sprite & sprite) const
{
  sf::Vector2i mousei = sf::Mouse::getPosition(*window);
  sf::Vector2f mouse = window->mapPixelToCoords(mousei);
  sf::FloatRect bounds = sprite.getGlobalBounds();

  return (mouse.x >= bounds.left && mouse.x <= bounds.left + bounds.width && mouse.y >= bounds.top && mouse.y <= bounds.top + bounds.height);
}

void DrawWindow::RegainFocus()
{

}

void DrawWindow::Resize(int newWidth, int newHeight)
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

const sf::View DrawWindow::GetView() {
  return view;
}

Camera& DrawWindow::GetCamera()
{
  return *cam;
}

void DrawWindow::SetCamera(const std::shared_ptr<Camera>& camera) {
  cam = camera;
}
