#include "bnCardDescriptionTextbox.h"
#include "bnMessage.h"
#include "bnTextureResourceManager.h"

CardDescriptionTextbox::CardDescriptionTextbox(const sf::Vector2f& pos) :
   AnimatedTextBox(pos)
{
  this->SetTextSpeed(2.0);
}

void CardDescriptionTextbox::DescribeCard(Battle::Card* card)
{
  if (card == nullptr) return;

  while (HasMessage()) {
    DequeMessage();
  }

  EnqueMessage(new Message(card->GetDescription()));
  Open();
}
