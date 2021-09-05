#include "bnHubBatchCardAction.h"
#include "bnReflectCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseSuperArmor.h"
#include "battlescene/bnBattleSceneBase.h"
#include "bnPlayer.h"

#define FRAME1 { 1, 0.1f }

#define FRAMES FRAME1


HubBatchCardAction::HubBatchCardAction(std::shared_ptr<Character> actor) :
  CardAction(actor, "PLAYER_IDLE") {

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

HubBatchCardAction::~HubBatchCardAction()
{
}

void HubBatchCardAction::OnExecute(std::shared_ptr<Character> user) {
  // Play sound
  Audio().Play(AudioType::RECOVER);

  // Add hubbatchprogram compontent
  user->CreateComponent<HubBatchProgram>(user);
}

void HubBatchCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void HubBatchCardAction::OnAnimationEnd()
{
}

void HubBatchCardAction::OnActionEnd()
{
}

// class HubBatchProgram : public Component

HubBatchProgram::HubBatchProgram(std::weak_ptr<Character> _owner) :
  Component(_owner, Component::lifetimes::battlestep)
{
  auto owner = _owner.lock();

  anim = Animation("resources/spells/hub_batch.animation");
  effect.setTexture(owner->Textures().LoadTextureFromFile("resources/spells/hub_batch.png"));

  anim << "DEFAULT" << [this] {
    effect.Hide();
  };

  anim.Refresh(effect.getSprite());

  owner->AddNode(&effect);
  effect.SetLayer(-5);
  effect.setPosition(0, -owner->GetHeight() / 2.f / 2.f);

  superarmor = std::make_shared<DefenseSuperArmor>();
  owner->AddDefenseRule(superarmor);
}

HubBatchProgram::~HubBatchProgram()
{
  auto owner = GetOwner();

  if (owner) {
    owner->RemoveNode(&effect);
  }
}

void HubBatchProgram::OnUpdate(double elapsed)
{
  if (auto owner = GetOwner()) {
    owner->SetFloatShoe(true);
  }
  
  if (auto player = GetOwnerAs<Player>()) {
    player->SetAttackLevel(PlayerStats::MAX_ATTACK_LEVEL); // max
    player->OverrideSpecialAbility([player]{ return std::make_shared<ReflectCardAction>(player, 10, ReflectShield::Type::yellow); });
    
    if (Injected()) {
      // Scene()->GetCardSelectWidget().SetMaxCardDraw(10);
    }
  }

  anim.Update(elapsed, effect.getSprite());
}

void HubBatchProgram::Inject(BattleSceneBase& scene)
{
  scene.Inject(this);
}
