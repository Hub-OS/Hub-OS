#include "bnCardDescriptionTextbox.h"
#include "bnMessage.h"

CardDescriptionTextbox::CardDescriptionTextbox(sf::Vector2f pos) :
   AnimatedTextBox(pos)
{

}

void CardDescriptionTextbox::DescribeCard(Card* card)
{
  if (card == nullptr) return;

  while (this->HasMessage()) {
    this->DequeMessage();
  }

  this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new Message(card->GetDescription()));


  // FOR FUN TESTING:
  /*auto yes = [this]() {
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", Message("Glad I could help!"));
  };

  auto no = [this]() {
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)), "resources/ui/navigator.animation", new Message("And now I'm here!"));
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new Message("Where did you come from???"));
    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_PROG)), "resources/ui/navigator.animation", new Message("I've always been here. You just never pay attention. See you around!"));
  };

    this->EnqueMessage(sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new Question("Was this helpful? Please tell me it was helpful. Yes or no?", yes, no));
  */

  this->Open();
}
