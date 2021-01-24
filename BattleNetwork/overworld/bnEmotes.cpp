#define _USE_MATH_DEFINES
#include <math.h>
#include <Swoosh/Ease.h>
#include "bnEmotes.h"
#include "../bnTextureResourceManager.h"
#include "../bnInputManager.h"
#include "../bnShaderResourceManager.h"

constexpr sf::Int32 EMOJI_DISPLAY_MILI = 5000;
constexpr float NOT_SELECTED_RADIUS = 0.75f;
constexpr float SELECTED_RADIUS = 1.0f;
constexpr float CIRCLE_RADIUS_PX = 44.0f; // in pixels

using namespace swoosh::ease;

Overworld::EmoteNode::EmoteNode() : ResourceHandle()
{
  setTexture(Textures().LoadTextureFromFile("resources/ow/emotes/emotes.png"));
  setOrigin(5.5f, 5.5f);
  Reset();
}

Overworld::EmoteNode::~EmoteNode()
{
}

void Overworld::EmoteNode::Reset()
{
  Emote(Overworld::Emotes::happy);
  timer.reverse(true);
  Hide();
}

void Overworld::EmoteNode::Emote(Overworld::Emotes type)
{
  size_t idx = std::min(static_cast<size_t>(type), static_cast<size_t>(Overworld::Emotes::size)-1);

  sf::IntRect rect = sf::IntRect(static_cast<int>(11 * idx), 0, 11, 11);
  setTextureRect(rect);
  timer.set(sf::milliseconds(EMOJI_DISPLAY_MILI));
  timer.start();
  Reveal();
}

void Overworld::EmoteNode::Update(double elapsed)
{
  timer.update(sf::seconds(static_cast<float>(elapsed)));

  if (timer.getElapsed().asMilliseconds() == 0) {
    Hide();
  }
}

Overworld::EmoteWidget::EmoteWidget() : 
  radius(CIRCLE_RADIUS_PX),
  sf::Drawable(), 
  sf::Transformable()
{
  size_t idx = 0;
  size_t max = static_cast<size_t>(Emotes::size);

  while (idx < max) {
    auto& e = emoteSprites[idx];
    e.setTexture(Textures().LoadTextureFromFile("resources/ow/emotes/emotes.png"));
    e.setOrigin(5.5f, 5.5f);

    sf::IntRect rect = sf::IntRect(static_cast<int>(idx * 11), 0, 11, 11);
    e.setTextureRect(rect);

    float alpha = radius * NOT_SELECTED_RADIUS;

    float theta = idx * ((2.f * static_cast<float>(M_PI)) / static_cast<float>(max));
    sf::Vector2f pos = sf::Vector2f(cos(theta) * alpha, sin(theta) * alpha);
    emoteSprites[idx].setPosition(pos.x, pos.y);
    idx++;
  }
}

Overworld::EmoteWidget::~EmoteWidget()
{
}

void Overworld::EmoteWidget::Update(double elapsed)
{
  size_t idx = 0;
  size_t max = static_cast<size_t>(Emotes::size);

  while (idx < max) {
    emoteSprites[idx].SetShader(Shaders().GetShader(ShaderType::GREYSCALE));
    float alpha = radius * NOT_SELECTED_RADIUS;

    if (static_cast<size_t>(currEmote) == idx && IsOpen()) {
      alpha = radius * SELECTED_RADIUS;
      emoteSprites[idx].SetShader(nullptr);
    }
    float theta = static_cast<float>(idx * ((2.f * M_PI) / static_cast<float>(max)));
    sf::Vector2f pos = sf::Vector2f(cos(theta) * alpha, sin(theta) * alpha);
    auto x = interpolate(0.5f, pos.x, emoteSprites[idx].getPosition().x);
    auto y = interpolate(0.5f, pos.y, emoteSprites[idx].getPosition().y);

    emoteSprites[idx].setPosition(x, y);
    idx++;
  }

  if (IsClosed()) return;

  Emotes end = static_cast<Emotes>(static_cast<size_t>(Emotes::size) - 1);

  if (Input().Has(InputEvents::pressed_scan_left)) {
    if (currEmote == Emotes{ 0 }) {
      currEmote = end;
    }
    else {
      currEmote = static_cast<Emotes>(static_cast<size_t>(currEmote) - 1);
    }
  }

  if (Input().Has(InputEvents::pressed_scan_right)) {
    if (currEmote == end) {
      currEmote = Emotes{ 0 };
    }
    else {
      currEmote = static_cast<Emotes>(static_cast<size_t>(currEmote) + 1);
    }
  }

  if (Input().Has(InputEvents::pressed_confirm)) {
    callback ? callback(currEmote) : void(0);
  }
}

void Overworld::EmoteWidget::draw(sf::RenderTarget& surface, sf::RenderStates states) const
{
  if (state == State::closed) return;

  states.transform *= getTransform();

  sf::CircleShape circle = sf::CircleShape(this->radius, static_cast<size_t>(Emotes::size));
  circle.setOutlineColor(sf::Color(155, 155, 155, 155));
  circle.setFillColor(sf::Color(80, 80, 80, 80));
  circle.setOrigin(this->radius, this->radius);

  surface.draw(circle, states);
  
  for (auto& e : emoteSprites) {
    surface.draw(e, states);
  }
}

void Overworld::EmoteWidget::Open()
{
  state = State::open;
}

void Overworld::EmoteWidget::Close()
{
  state = State::closed;
}

void Overworld::EmoteWidget::OnSelect(const std::function<void(Overworld::Emotes)>& callback)
{
  this->callback = callback;
}

const bool Overworld::EmoteWidget::IsOpen()
{
  return state == State::open;
}

const bool Overworld::EmoteWidget::IsClosed()
{
  return state == State::closed;
}
