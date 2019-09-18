#include "bnChipDescriptionTextbox.h"

ChipDescriptionTextbox::ChipDescriptionTextbox(sf::Vector2f pos) :
   AnimatedTextBox(pos)
{

}

void ChipDescriptionTextbox::DescribeChip(Chip* chip)
{
  if (chip == nullptr) return;

  while (this->HasMessage()) {
    this->DequeMessage();
  }

  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new AnimatedTextBox::Message(chip->GetDescription()));


  // FOR FUN TESTING:
  /*auto yes = [this]() {
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new AnimatedTextBox::Message("Glad I could help!"));
  };

  auto no = [this]() {
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)), "resources/ui/navigator.animation", new AnimatedTextBox::Message("And now I'm here!"));
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new AnimatedTextBox::Message("Where did you come from???"));
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)), "resources/ui/navigator.animation", new AnimatedTextBox::Message("I've always been here. You just never pay attention. See you around!"));
  };

  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new AnimatedTextBox::Question("Was this helpful? Please tell me it was helpful. Yes or no?", yes, no));
  */

  this->Open();
}
