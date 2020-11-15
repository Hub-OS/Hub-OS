#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnDefenseBubbleWrap.h"
#include "bnBubbleTrap.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/bubble_trap.animation"

BubbleTrap::BubbleTrap(Character* owner) : willDelete(false), defense(nullptr), duration(3), SpriteProxyNode(), Component(owner)
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

void BubbleTrap::Inject(BattleSceneBase& bs) {

}

void BubbleTrap::OnUpdate(float _elapsed) {
  auto keyTestThunk = [this](const InputEvent& key) {
    bool pass = false;

    if (INPUTx.Has(key)) {
      auto iter = std::find(lastFrameStates.begin(), lastFrameStates.end(), key);

      if (iter == lastFrameStates.end()) {
        lastFrameStates.clear();
        pass = true;
      } 

      lastFrameStates.push_back(key);
    }

    return pass;
  };

  bool anyKey = keyTestThunk(EventTypes::PRESSED_USE_CHIP);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_CUST_MENU);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_MOVE_DOWN);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_MOVE_UP);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_MOVE_LEFT);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_MOVE_RIGHT);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_SHOOT);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_PAUSE);
  anyKey = anyKey || keyTestThunk(EventTypes::PRESSED_SPECIAL);

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