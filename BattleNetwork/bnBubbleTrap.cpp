#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : Artifact(nullptr), Component(owner)
{
  // Bubbles have to pop when hit
  defense = new DefenseBubbleWrap();

  if (owner->IsDeleted()) {
    this->Delete();
    this->GetOwner()->FreeComponentByID(this->Component::GetID());
    this->FreeOwner();
    this->defense = nullptr;
  }
  else {
    owner->AddDefenseRule(defense);
  }

  SetLayer(1);
  this->setTexture(TEXTURES.GetTexture(TextureType::SPELL_BUBBLE_TRAP));
  this->setScale(2.f, 2.f);
  bubble = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animator::Mode::Loop;

  animation.Update(0, this->getSprite());

  duration = 4; // seconds
}

void BubbleTrap::Inject(BattleScene& bs) {

}

void BubbleTrap::OnUpdate(float _elapsed) {
  if (!this->tile) return;

  if (duration <= 0 &&  animation.GetAnimationString() != "POP" ) {
    this->Pop();
  }

  duration -= _elapsed;

  auto x = this->GetOwner()->getPosition().x;
  auto y = this->GetOwner()->getPosition().y - (this->GetOwnerAs<Character>()->GetHeight()/2.0f / 2.0f);
  this->setPosition(x, y);

  animation.Update(_elapsed, this->getSprite());
}

void BubbleTrap::OnDelete()
{
  Remove();
}

bool BubbleTrap::Move(Direction _direction)
{
  return false;
}

void BubbleTrap::Pop()
{
  auto onFinish = [this]() {
    if (this->GetOwner()) {
      this->GetOwnerAs<Character>()->RemoveDefenseRule(defense);
      this->GetOwner()->FreeComponentByID(this->Component::GetID());
    }

    this->Delete();
  };

  animation << "POP" << onFinish;
}

BubbleTrap::~BubbleTrap()
{
  delete defense;
}