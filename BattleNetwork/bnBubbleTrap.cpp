#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnInputManager.h"
#include "bnField.h"
#include "bnDefenseBubbleStatus.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/scenes/battle/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(std::weak_ptr<Entity> owner) : 
  willDelete(false), defense(nullptr), 
  ResourceHandle(),
  SpriteProxyNode(), Component(owner)
{
  SetLayer(1);
  setTexture(Textures().LoadFromFile(TexturePaths::SPELL_BUBBLE_TRAP));
  bubble = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animator::Mode::Loop;

  animation.Update(0, getSprite());
}

void BubbleTrap::Inject(BattleSceneBase& bs) {
}

void BubbleTrap::OnUpdate(double _elapsed) {
  auto asCharacter = GetOwnerAs<Character>();
  if (!asCharacter || asCharacter->IsDeleted()) {
    this->Eject();
  }
  
  frame_time_t duration = GetDuration();

  if (!init) {
    // Bubbles have to pop when hit
    defense = std::make_shared<DefenseBubbleStatus>();
    asCharacter->AddDefenseRule(defense);
    asCharacter->AddNode(shared_from_base<BubbleTrap>());

    init = true;
  }

  // either the timer runs out or the defense was popped by an attack
  bool shouldpop = duration <= frames(0) && animation.GetAnimationString() != "POP";
  shouldpop = shouldpop || (defense->IsPopped() && animation.GetAnimationString() != "POP");

  if (shouldpop) {
    Pop();
  }

  auto y = -asCharacter->GetHeight() / 2.0f;
  setPosition(0.f, y);

  animation.Update(_elapsed, getSprite());
}

void BubbleTrap::Pop()
{
  auto onFinish = [this]() {
    if (auto character = GetOwnerAs<Character>()) {
      character->RemoveNode(this);
      character->RemoveDefenseRule(defense);
      this->Eject();
    }
    willDelete = true;
  };

  animation << "POP" << onFinish;
}

frame_time_t BubbleTrap::GetDuration() const
{
    return GetOwner()->GetStatusDuration(Hit::bubble);
}
