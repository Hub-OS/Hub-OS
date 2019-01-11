#include "bnChipDescriptionTextbox.h"

ChipDescriptionTextbox::ChipDescriptionTextbox(sf::Vector2f pos, sf::IntRect textArea) :
   AnimatedTextBox(pos, textArea)
{

}

void ChipDescriptionTextbox::DescribeChip(Chip* chip)
{
  if (chip == nullptr) return;

  this->DequeMessage();
  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", chip->GetDescription());
  this->Open();
}
