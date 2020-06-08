#include "bnAppButton.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

const float AppButton::CalculateWidth() const
{
  return edge.getLocalBounds().width + text.GetLocalBounds().width;
}

AppButton::AppButton(const std::string & name) : text(name, AppButton::font)
{
  edge.setTexture(LOAD_TEXTURE(APP_BUTTON_EDGE));
  mid.setTexture(LOAD_TEXTURE(APP_BUTTON_MID));
  text.setScale(2.f, 2.f);
  text.setOrigin(sf::Vector2f(text.GetLocalBounds().width / 2.0f, text.GetLocalBounds().height / 2.0f));
}

AppButton::~AppButton()
{
}

void AppButton::SetColor(const sf::Color & color)
{
  edge.setColor(color);
  mid.setColor(color);
}

void AppButton::Slot(const Callback<void()> callback)
{
  AppButton::callback = callback;
}

void AppButton::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
  states.transform *= getTransform();
  states.shader = Shaders().GetShader(ShaderType::COLORIZE);

  edge.setScale(2.f, 2.f);
  edge.setPosition(0, 0);
  target.draw(edge, states);

  float width = CalculateWidth()*2.f;
  mid.setScale(width, 2.f);
  mid.setPosition(edge.getLocalBounds().width*2.f, 0.f);
  target.draw(mid, states);

  text.setPosition((edge.getLocalBounds().width*2.f)+(width/2.0f), mid.getLocalBounds().height-(font.GetLetterHeight()/2.f));
  target.draw(text, states);

  edge.setScale(-2.f, 2.f);
  edge.setPosition((edge.getLocalBounds().width*4.f)+width, 0.f);
  target.draw(edge, states);
}

Font AppButton::font = Font(Font::Style::thick);
