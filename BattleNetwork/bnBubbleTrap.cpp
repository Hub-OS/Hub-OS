#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnInputManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : 
  willDelete(false), defense(nullptr), duration(3), 
  ResourceHandle(), InputHandle(),
  SpriteProxyNode(), Component(owner)
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
  setTexture(Textures().GetTexture(TextureType::SPELL_BUBBLE_TRAP));
  bubble = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation << "DEFAULT" << Animator::Mode::Loop;

  animation.Update(0, getSprite());
}

void BubbleTrap::Inject(BattleSceneBase& bs) {

}

void BubbleTrap::OnUpdate(float _elapsed) {
  auto keyTestThunk = [this](const InputEvent& key) {
    bool pass = false;

    if (Input().Has(key)) {
      auto iter = std::find(lastFrameStates.begin(), lastFrameStates.end(), key);

      if (iter == lastFrameStates.end()) {
        lastFrameStates.clear();
        pass = true;
      } 

      lastFrameStates.push_back(key);
    }

    return pass;
  };

  bool anyKey = keyTestThunk(InputEvents::pressed_use_chip);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_cust_menu);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_down);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_up);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_left);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_right);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_shoot);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_pause);
  anyKey = anyKey || keyTestThunk(InputEvents::pressed_special);

  if (anyKey) {
    duration -= frames(1).asSeconds();
  }

  duration -= _elapsed;

  // either the timer runs out or the defense was popped by an attack
  bool shouldpop = duration <= 0 && animation.GetAnimationString() != "POP";
  shouldpop = shouldpop || (defense->IsPopped() && animation.GetAnimationString() != "POP");

  if (shouldpop) {
    Pop();
  }

  auto y = -GetOwnerAs<Character>()->GetHeight() / 4.0f;
  setPosition(0.f, y);

  animation.Update(_elapsed, getSprite());
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

const double BubbleTrap::GetDuration() const
{
  return duration;
}

BubbleTrap::~BubbleTrap()
{
  delete defense;
}