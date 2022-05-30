#include "bnBattleTextBox.h"
#include "bnMessage.h"
#include "bnMessageQuestion.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

Battle::AnimatedTextBox::AnimatedTextBox(const sf::Vector2f& pos) : ::AnimatedTextBox(pos)
{
}

void Battle::AnimatedTextBox::DescribeCard(Battle::Card* card)
{
  if (card == nullptr || requestedRetreat) return;

  while (HasMessage()) {
    DequeMessage();
  }

  // use the long description unless it is not provided (empty) otherwise 
  // use the short card description instead
  const std::string& longDescr = card->GetVerboseDescription();
  const std::string& shortDescr = card->GetDescription();
  EnqueMessage(mug, anim, new Message(longDescr.empty()? shortDescr : longDescr));
  Open();
}

void Battle::AnimatedTextBox::PromptRetreat()
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

void Battle::AnimatedTextBox::SetSpeaker(const sf::Sprite& mug, const Animation& anim)
{
  this->mug = mug;
  this->anim = anim;
}

void Battle::AnimatedTextBox::Reset()
{
  asking = requestedRetreat = false;
}

const bool Battle::AnimatedTextBox::RequestedRetreat() const
{
  return this->requestedRetreat;
}

const bool Battle::AnimatedTextBox::HasQuestion() const
{
  return asking;
}

bool Battle::AnimatedTextBox::SelectYes() const
{
  if (asking) {
    return question->SelectYes();
  }

  return false;
}

bool Battle::AnimatedTextBox::SelectNo() const
{
  if (asking) {
    return question->SelectNo();
  }

  return false;
}

void Battle::AnimatedTextBox::ConfirmSelection()
{
  if (asking) {
    question->ConfirmSelection();
    asking = false;
  }
  this->Close();
}

void Battle::AnimatedTextBox::Cancel()
{
  if (asking) {
    question->Cancel();
    asking = false;
  }
  this->Close();
}
