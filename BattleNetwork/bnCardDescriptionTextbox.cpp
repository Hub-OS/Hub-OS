#include "bnCardDescriptionTextbox.h"
#include "bnMessage.h"

CardDescriptionTextbox::CardDescriptionTextbox(sf::Vector2f pos) :
   AnimatedTextBox(pos)
{
  this->SetTextSpeed(2.0); // TODO just make this default
}

void CardDescriptionTextbox::DescribeCard(Battle::Card* card)
{
  if (card == nullptr) return;

  while (HasMessage()) {
    DequeMessage();
  }

  EnqueMessage(sf::Sprite(*LOAD_TEXTURE(MUG_NAVIGATOR)), "resources/ui/navigator.animation", new Message(card->GetDescription()));
  Open();
}
