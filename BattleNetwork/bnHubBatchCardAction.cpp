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


HubBatchCardAction::HubBatchCardAction(Character& actor) :
  CardAction(actor, "PLAYER_IDLE") {

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

HubBatchCardAction::~HubBatchCardAction()
{
}

void HubBatchCardAction::OnExecute(Character* user) {
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

HubBatchProgram::HubBatchProgram(Character* owner) :
  Component(owner, Component::lifetimes::battlestep)
{
  anim = Animation("resources/spells/hub_batch.animation");
  effect.setTexture(owner->Textures().LoadTextureFromFile("resources/spells/hub_batch.png"));

  anim << "DEFAULT" << [this] {
    effect.Hide();
  };

  anim.Refresh(effect.getSprite());

  owner->AddNode(&effect);
  effect.SetLayer(-5);
  effect.setPosition(0, -owner->GetHeight() / 2.f / 2.f);

  superarmor = new DefenseSuperArmor();
  owner->AddDefenseRule(superarmor);
}

HubBatchProgram::~HubBatchProgram()
{
  GetOwner()->RemoveNode(&effect);
  delete superarmor;
}

void HubBatchProgram::OnUpdate(double elapsed)
{
  GetOwner()->SetFloatShoe(true);
  Player* player = GetOwnerAs<Player>();

  
  if (player) {
    player->SetAttackLevel(PlayerStats::MAX_ATTACK_LEVEL); // max
    player->OverrideSpecialAbility([player]{ return new ReflectCardAction(*player, 10, ReflectShield::Type::yellow); });
    
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
