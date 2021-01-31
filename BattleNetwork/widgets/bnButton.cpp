#include "bnButton.h"

Button::Button(std::shared_ptr<Widget> parent, const std::string& labelStr) :
  label(Font::Style::thick), 
  Widget(parent)
{
  SetLabel(labelStr);
  AddNode(&label);
  AddNode(&img);
}

Button::~Button()
{
}

const sf::FloatRect Button::CalculateBounds() const
{
  float distx = std::fabs(label.getPosition().x - img.getPosition().x);
  float disty = std::fabs(label.getPosition().y - img.getPosition().y);
  float minLeft   = std::min(label.GetLocalBounds().left, img.getLocalBounds().left);
  float minTop    = std::min(label.GetLocalBounds().top, img.getLocalBounds().top);
  float maxWidth  = distx + std::max(label.GetLocalBounds().width, img.getLocalBounds().width);
  float maxHeight = disty + std::max(label.GetLocalBounds().height, img.getLocalBounds().height);

  return sf::FloatRect(minLeft, minTop, maxWidth, maxHeight);
}

void Button::SetLabel(const std::string& labelStr)
{
  this->label.SetString(labelStr);
  img.setPosition(this->label.GetLocalBounds().width*0.5f , this->label.GetFont().GetLetterHeight());
}

void Button::SetImage(const std::string& path)
{
  ResourceHandle handle;
  img.setTexture(handle.Textures().LoadTextureFromFile(path));
}

void Button::ClearImage()
{
  img.setTexture(std::make_shared<sf::Texture>(), true);
}
