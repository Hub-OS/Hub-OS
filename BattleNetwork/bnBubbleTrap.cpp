#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : Artifact(), Component(owner)
{
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
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_BUBBLE_TRAP));
  this->setScale(2.f, 2.f);
  bubble = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animate::Mode::Loop;

  animation.Update(0, *this);

  duration = 4; // seconds
}

void BubbleTrap::Inject(BattleScene& bs) {

}

void BubbleTrap::Update(float _elapsed) {
  if (duration <= 0 &&  animation.GetAnimationString() != "POP" ) {
    this->Pop();
  }

  duration -= _elapsed;

  this->setPosition(this->tile->getPosition() - sf::Vector2f(0, 25.0f));

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
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