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


HubBatchCardAction::HubBatchCardAction(Character* owner) :
  CardAction(*owner, "PLAYER_IDLE") {

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

HubBatchCardAction::~HubBatchCardAction()
{
}

void HubBatchCardAction::Execute() {
  // Play sound
  AUDIO.Play(AudioType::RECOVER);

  // Add hubbatchprogram compontent
  GetOwner()->CreateComponent<HubBatchProgram>(GetOwner());
}

void HubBatchCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void HubBatchCardAction::OnAnimationEnd()
{
}

void HubBatchCardAction::EndAction()
{
  Eject();
}

// class HubBatchProgram : public Component

HubBatchProgram::HubBatchProgram(Character* owner) :
  Component(owner, Component::lifetimes::battlestep)
{
  anim = Animation("resources/spells/hub_batch.animation");
  effect.setTexture(TEXTURES.LoadTextureFromFile("resources/spells/hub_batch.png"));

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
  GetOwnerAs<Character>()->RemoveDefenseRule(superarmor);
  delete superarmor;
}

void HubBatchProgram::OnUpdate(float elapsed)
{
  GetOwner()->SetFloatShoe(true);
  Player* player = GetOwnerAs<Player>();

  
  if (player) {
    player->SetAttackLevel(PlayerStats::MAX_ATTACK_LEVEL); // max
    player->OverrideSpecialAbility([player]{ return new ReflectCardAction(player, 10, ReflectShield::Type::yellow); });
    
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
