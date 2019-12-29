#include "bnAlphaElectricState.h"
#include "bnAlphaCore.h"
#include "bnAnimationComponent.h"
#include "bnTile.h"
#include "bnField.h"

AlphaElectricState::AlphaElectricState() : AIState<AlphaCore>() { ; }
AlphaElectricState::~AlphaElectricState() { ; }

void AlphaElectricState::OnEnter(AlphaCore& a) {
  // skip unless at half health
  //if (a.GetHealth() > a.GetMaxHealth() / 2) {
  //  a.GoToNextState();
  //  return;
  //}

  ready = false;
  count = 0;
  current = nullptr;

  a.EnableImpervious();
  AnimationComponent* anim = a.GetFirstComponent<AnimationComponent>();

  auto onFinish = [alpha = &a, anim, this]() {
    ready = true;
  };

  anim->SetAnimation("CORE_ATTACK1", onFinish);
}

void AlphaElectricState::OnUpdate(float _elapsed, AlphaCore& a) {
  if (current) {
    if (current->IsDeleted()) {
      AnimationComponent* anim = a.GetFirstComponent<AnimationComponent>();
      auto onFinish = [alpha = &a, anim, this]() {
        // This will happen on the core's tick step but we want it to happen immediately after too.
        anim->SetAnimation("CORE_FULL");
        anim->SetPlaybackMode(Animator::Mode::Loop);

        alpha->GoToNextState();
      };

      anim->SetAnimation("CORE_ATTACK1", onFinish);
      anim->SetPlaybackMode(Animator::Mode::Reverse);
      current = nullptr;
      ready = false; // will stop electric currents from being spawned
    }
    return;
  }

  if (!ready) return;

  current = new AlphaElectricCurrent(a.GetField(), a.GetTeam(), 7);
  auto props = current->GetHitboxProperties();
  props.aggressor = &a;
  current->SetHitboxProperties(props);

  a.GetField()->AddEntity(*current, a.GetTile()->GetX() - 2, a.GetTile()->GetY());
}

void AlphaElectricState::OnLeave(AlphaCore& a) {
  a.EnableImpervious(false);
}

#define RESOURCE_PATH "resources/mobs/alpha/alpha.animation"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

AlphaElectricCurrent::AlphaElectricCurrent(Field* field, Team team, int count) : countMax(count), count(0), Spell(field, team)
{
  this->setTexture(LOAD_TEXTURE(MOB_ALPHA_ATLAS));
  anim = (AnimationComponent*)RegisterComponent(new AnimationComponent(this));
  anim->Setup(RESOURCE_PATH);
  anim->Load();
  this->setScale(2.f, 2.f);
  this->SetHeight(48);
}

AlphaElectricCurrent::~AlphaElectricCurrent()
{
}

void AlphaElectricCurrent::OnSpawn(Battle::Tile & start)
{
  auto endTrigger = [this]() {
    count++;
  };

  anim->SetAnimation("ELECTRIC", endTrigger);
  anim->SetPlaybackMode(Animator::Mode::Loop);

  auto attackTopAndBottomRowTrigger = [this]() {
    GetTile()->AffectEntities(this);
    AUDIO.Play(AudioType::THUNDER);
  };

  auto attackMiddleRowTrigger = [this]() {
    GetTile()->AffectEntities(this);
    AUDIO.Play(AudioType::THUNDER);
  };


  anim->AddCallback(1, attackTopAndBottomRowTrigger, Animator::NoCallback, false);
  anim->AddCallback(4, attackMiddleRowTrigger, Animator::NoCallback, false);
}


void AlphaElectricCurrent::OnUpdate(float _elapsed)
{
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - GetHeight());

  if (count >= countMax) {
    this->Delete();
  }
}

void AlphaElectricCurrent::Attack(Character * _entity)
{
  _entity->Hit(GetHitboxProperties());
}
