#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : SpriteProxyNode(), Component(owner)
{
  // Bubbles have to pop when hit
  defense = new DefenseBubbleWrap();

  if (owner->IsDeleted()) {
    GetOwner()->FreeComponentByID(Component::GetID());
    defense = nullptr;
  }
  else {
    owner->AddDefenseRule(defense);
  }

  SetLayer(1);
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BUBBLE_TRAP));
  setScale(2.f, 2.f);
  bubble = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animator::Mode::Loop;

  animation.Update(0, getSprite());

  duration = 4; // seconds
}

void BubbleTrap::Inject(BattleScene& bs) {

}

void BubbleTrap::OnUpdate(float _elapsed) {

  if (duration <= 0 &&  animation.GetAnimationString() != "POP" ) {
    Pop();
  }

  duration -= _elapsed;

  auto x = GetOwner()->getPosition().x;
  auto y = GetOwner()->getPosition().y - (GetOwnerAs<Character>()->GetHeight()/2.0f / 2.0f);
  setPosition(x, y);

  animation.Update(_elapsed, getSprite());
}

void BubbleTrap::Pop()
{
  auto onFinish = [this]() {
    if (GetOwner()) {
      GetOwnerAs<Character>()->RemoveDefenseRule(defense);
      GetOwner()->FreeComponentByID(Component::GetID());
    }

    GetOwner()->RemoveNode(this);
    delete this;
  };

  animation << "POP" << onFinish;
}

BubbleTrap::~BubbleTrap()
{
  delete defense;
}