#include "bnChipDescriptionTextbox.h"

ChipDescriptionTextbox::ChipDescriptionTextbox(sf::Vector2f pos) :
   AnimatedTextBox(pos)
{

}

void ChipDescriptionTextbox::DescribeChip(Chip* chip)
{
  if (chip == nullptr) return;

  this->DequeMessage();
  this->DequeMessage();
  this->DequeMessage();
  this->DequeMessage();
  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", chip->GetDescription());
  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)),      "resources/ui/navigator.animation", "And now I'm here!");
  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", "Where did you come from???");
  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)),      "resources/ui/navigator.animation", "I've always been here. You just never pay attention. See you around!");
  this->Open();
}
