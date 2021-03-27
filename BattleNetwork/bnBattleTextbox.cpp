#include "bnBattleTextbox.h"
#include "bnMessage.h"
#include "bnMessageQuestion.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

Battle::Textbox::Textbox(const sf::Vector2f& pos) : AnimatedTextBox(pos)
{
  this->SetTextSpeed(2.0);
}

void Battle::Textbox::DescribeCard(Battle::Card* card)
{
  if (card == nullptr || requestedRetreat) return;

  while (HasMessage()) {
    DequeMessage();
  }

  EnqueMessage(mug, anim, new Message(card->GetDescription()));
  Open();
}

void Battle::Textbox::PromptRetreat()
{
  if (requestedRetreat) return;

  while (HasMessage()) {
    DequeMessage();
  }

  auto onYes = [this] {
    this->requestedRetreat = true;
  };

  this->question = new Question("Do you want to retreat?", onYes);
  EnqueMessage(mug, anim, question);
  Open();

  asking = true;
}

void Battle::Textbox::SetSpeaker(const sf::Sprite& mug, const Animation& anim)
{
  this->mug = mug;
  this->anim = anim;
}

const bool Battle::Textbox::RequestedRetreat() const
{
  return this->requestedRetreat;
}

const bool Battle::Textbox::HasQuestion() const
{
  return asking;
}

bool Battle::Textbox::SelectYes() const
{
  if (asking) {
    return question->SelectYes();
  }

  return false;
}

bool Battle::Textbox::SelectNo() const
{
  if (asking) {
    return question->SelectNo();
  }

  return false;
}

void Battle::Textbox::ConfirmSelection()
{
  if (asking) {
    question->ConfirmSelection();
    asking = false;
  }
}

void Battle::Textbox::Cancel()
{
  if (asking) {
    question->Cancel();
    asking = false;
  }
}
