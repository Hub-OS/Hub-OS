#include "bnDrawWindow.h"
#include <time.h>
#include <SFML/Window/ContextSettings.hpp>

#include "mmbn.ico.c"
#include "bnShaderType.h"
#include "bnShaderResourceManager.h"

void DrawWindow::Initialize(const std::string& title, DrawWindow::WindowMode mode)
{
  this->title = title;

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
  sf::VideoMode videoMode = VideoMode(480, 320);
#endif
  auto style = sf::Style::Default;

  if (mode == WindowMode::fullscreen) {
      style = sf::Style::Fullscreen;
  }

  window = new RenderWindow(videoMode, title, style);

  Resize((int)view.getSize().x, (int)view.getSize().y);

  window->setFramerateLimit(frame_time_t::frames_per_second);
  window->setIcon(sfml_icon.width, sfml_icon.height, sfml_icon.pixel_data);
}

void DrawWindow::Draw(Drawable& _drawable, bool applyShaders) {
  if (!HasRenderSurface()) return;

  if (applyShaders && supportShaders) {
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

  if (shader && shader->Get() && supportShaders) {
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
    if (shader.Get() != nullptr && supportShaders) {
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

void DrawWindow::Display()
{
  if (window) {
    window->display();
  }
}

RenderWindow* DrawWindow::GetRenderWindow() const {
  return window;
}

void DrawWindow::SupportShaders(bool support)
{
  this->supportShaders = support;
}

DrawWindow::DrawWindow()
{}

DrawWindow::~DrawWindow() {
  delete window;
}

const sf::Vector2f DrawWindow::GetViewOffset() {
  return GetView().getCenter() - cam->GetView().getCenter();
}

void DrawWindow::SetShader(sf::Shader* shader) {
  if (!supportShaders) return;

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

const std::string DrawWindow::GetTitle() const
{
  return title;
}

void DrawWindow::SetSubtitle(const std::string& subtitle)
{
  this->subtitle = subtitle;

  if (this->subtitle.empty()) {
    window->setTitle(this->title);
    return;
  }

  window->setTitle(this->title + ": " + this->subtitle);
}

Camera& DrawWindow::GetCamera()
{
  return *cam;
}

void DrawWindow::SetCamera(const std::shared_ptr<Camera>& camera) {
  cam = camera;
}
