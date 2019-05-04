#include "bnEngine.h"
#include <time.h>       /* time */

#include "mmbn.ico.c"
#include "bnShaderType.h"
#include "bnShaderResourceManager.h"

Engine& Engine::GetInstance() {
  static Engine instance;
  return instance;
}

void Engine::Initialize() {
  view = sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320));
  original = view; // never changes 
  cam = new Camera(view);

  window = new RenderWindow(VideoMode((unsigned int)view.getSize().x, (unsigned int)view.getSize().y), "Battle Network: Progs Edition");
  window->setFramerateLimit(120);

  window->setIcon(sfml_icon.width, sfml_icon.height, sfml_icon.pixel_data);

  postprocessing.create((unsigned int)view.getSize().x, (unsigned int)view.getSize().y); // Same as display

  // See the random generator with current time
  srand((unsigned int)time(0));
}

void Engine::Draw(Drawable& _drawable, bool applyShaders) {
    applyShaders = true;

  if (applyShaders) {
    auto stateCopy = state;
    if(!stateCopy.shader) {
      stateCopy.shader = SHADERS.GetShader(ShaderType::DEFAULT);
    }

    postprocessing.draw(_drawable, stateCopy);
  } else {
    postprocessing.draw(_drawable);
  }
}

void Engine::Draw(Drawable* _drawable, bool applyShaders) {
    applyShaders = true;
  if (!_drawable) {
    return;
  }

  if (applyShaders) {
    auto stateCopy = state;
    if(!stateCopy.shader) {
      stateCopy.shader = SHADERS.GetShader(ShaderType::DEFAULT);
    }

    postprocessing.draw(*_drawable, stateCopy);
  } else {
    postprocessing.draw(*_drawable);
  }
}

void Engine::Draw(LayeredDrawable* _drawable) {
  // For now, support at most one shader.
  // Grab the shader and image, apply to a new render target, pass this render target into Draw()

  LayeredDrawable* context = _drawable;
  SmartShader* shader = &context->GetShader();

  if (shader && shader->Get()) {
    const sf::Texture* original = context->getTexture();
    shader->ApplyUniforms();
    postprocessing.draw(*context, shader->Get()); // bake
    shader->ResetUniforms();
  } else {
    Draw(context, false);
  }
}
void Engine::Draw(vector<LayeredDrawable*> _drawable) {
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

    LayeredDrawable* context = *it;
    SmartShader& shader = context->GetShader();
    if (shader.Get() != nullptr) {
      shader.ApplyUniforms();
      postprocessing.draw(*context, shader.Get()); // bake
      shader.ResetUniforms();
    } else {
      Draw(context, false);
    }
  }
}

void Engine::Draw(vector<Drawable*> _drawable, bool applyShaders) {
    applyShaders = true;
  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    Draw(*it, true);
  }
}

void Engine::Display() {

  //view = cam->GetView();
  //window->setView(view);

  // flip and ready buffer
  postprocessing.display();
  // Capture buffer in a drawable context
  sf::Sprite postFX(postprocessing.getTexture());
  // drawbuffer on top of the scene
  window->draw(postFX, state);
  // show final result
  window->display();
  // Prepare buffer for next cycle
  postprocessing.clear(sf::Color::Transparent);
}

bool Engine::Running() {
  return window->isOpen();
}

void Engine::Clear() {
  underlay.Clear();
  layers.Clear();
  overlay.Clear();
  window->clear();
}

RenderWindow* Engine::GetWindow() const {
  return window;
}

Engine::Engine(void)
  : layers(Layers()),
  overlay(Overlay()),
  underlay(Underlay()) {

  cam = new Camera(view);
}

Engine::~Engine(void) {
  delete window;
}

const sf::Vector2f Engine::GetViewOffset() {
  return GetDefaultView().getCenter() - cam->GetView().getCenter();
}

void Engine::Push(LayeredDrawable* _drawable) {
  if (_drawable) {
    layers.Insert(_drawable);
  }
}

void Engine::Lay(LayeredDrawable* _drawable) {
  if (_drawable) {
    overlay.Push(_drawable);
  }
}

void Engine::Lay(vector<sf::Drawable*> _drawable) {
  auto it = _drawable.begin();
  for (it; it != _drawable.end(); ++it) {
    if (*it) {
      overlay.Push(*it);
    }
  }
}

void Engine::LayUnder(sf::Drawable* _drawable) {
  if (_drawable) {
    underlay.Push(_drawable);
  }
}

void Engine::DrawLayers() {
  for (int i = layers.min; i <= layers.max; i++) {
    Draw(layers.At(i));
  }
}

void Engine::DrawOverlay() {
  Draw(overlay, false);
}

void Engine::DrawUnderlay() {
  Draw(underlay);
}

void Engine::SetShader(sf::Shader* shader) {

  if (shader == nullptr) {
    state = sf::RenderStates::Default;
    state.shader = SHADERS.GetShader(ShaderType::DEFAULT);
    postprocessing.setDefaultShader(SHADERS.GetShader(ShaderType::DEFAULT));
  } else {
    state.shader = shader;
  }
}

void Engine::RevokeShader() {
  SetShader(nullptr);
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
