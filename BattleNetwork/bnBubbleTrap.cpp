#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : willDelete(false), defense(nullptr), duration(4), SpriteProxyNode(), Component(owner)
{
  if (owner->IsDeleted()) {
    GetOwner()->FreeComponentByID(Component::GetID());
  }
  else {
    // Bubbles have to pop when hit
    defense = new DefenseBubbleWrap();
    owner->AddDefenseRule(defense);
    owner->AddNode(this);
  }

  SetLayer(1);
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BUBBLE_TRAP));
  bubble = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animator::Mode::Loop;

  animation.Update(0, getSprite());
}

void BubbleTrap::Inject(BattleScene& bs) {

}

void BubbleTrap::OnUpdate(float _elapsed) {

  // either the timer runs out or the defense was popped by an attack
  bool shouldpop = duration <= 0 && animation.GetAnimationString() != "POP";
  shouldpop = shouldpop || (defense->IsPopped() && animation.GetAnimationString() != "POP");

  if (shouldpop) {
    Pop();
  }

  duration -= _elapsed;

  auto y = -GetOwnerAs<Character>()->GetHeight() / 4.0f;
  setPosition(0.f, y);

  animation.Update(_elapsed, getSprite());

  if (willDelete)
    delete this;
}

void BubbleTrap::Pop()
{
  auto onFinish = [this]() {
    if (auto character = GetOwnerAs<Character>()) {
      character->RemoveNode(this);
      character->RemoveDefenseRule(defense);
      character->FreeComponentByID(Component::GetID());
    }
    willDelete = true;
  };

  animation << "POP" << onFinish;
}

BubbleTrap::~BubbleTrap()
{
  delete defense;
}