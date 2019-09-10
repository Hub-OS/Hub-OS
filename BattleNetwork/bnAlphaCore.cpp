#include "bnAlphaCore.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnExplodeState.h"
#include "bnDefenseVirusBody.h"
#include "bnHitBox.h"

#define RESOURCE_PATH "resources/mobs/alpha/alpha.animation"

AlphaCore::AlphaCore(Rank _rank)
  : AI<AlphaCore>(this), Character(_rank) {
  Entity::team = Team::BLUE;
  totalElapsed = 0;
  coreHP = prevCoreHP = 10;
  coreRegen = 0;
  setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  setScale(2.f, 2.f);

  this->SetHealth(4000);

  //Components setup and load
  this->animationComponent = (AnimationComponent*)RegisterComponent(new AnimationComponent(this));
  this->animationComponent->Setup(RESOURCE_PATH);
  this->animationComponent->Load();
  this->animationComponent->SetAnimation("CORE_FULL");
  this->animationComponent->SetPlaybackMode(Animator::Mode::Loop);
  this->animationComponent->OnUpdate(0);

  animation = Animation(animationComponent->GetFilePath());
  animation.Load();

  acid = new SpriteSceneNode();
  acid->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  animation.SetAnimation("ACID");
  animation.Update(0, *acid);
  
  head = new SpriteSceneNode();
  head->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  head->SetLayer(-2);
  head->setPosition(-10, -50);
  animation.SetAnimation("HEAD");
  animation.Update(0, *head);

  side = new SpriteSceneNode();
  side->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  side->SetLayer(-1);
  side->setPosition(6 , -50);
  animation.SetAnimation("SIDE");
  animation.Update(0, *side);

  this->AddNode(acid);
  this->AddNode(head);
  this->AddNode(side);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);

  defense = new AlphaCoreDefenseRule(coreHP);
  this->AddDefenseRule(defense);
}

AlphaCore::~AlphaCore() {
  delete head;
  delete acid, side;
}

void AlphaCore::OnUpdate(float _elapsed) {
  totalElapsed += _elapsed;
  coreRegen += _elapsed;

  if (coreHP < 10) {
    if (coreRegen > 1) {
      coreRegen = 0;
      coreHP += 5;
    }

    if (prevCoreHP != coreHP) {
      if (coreHP <= 0) {
        this->animationComponent->SetAnimation("CORE_EXPOSED");
        this->animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }
      else if (coreHP <= 5) {
        this->animationComponent->SetAnimation("CORE_HALF");
        this->animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }
      else {
        this->animationComponent->SetAnimation("CORE_FULL");
        this->animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }

      this->animationComponent->OnUpdate(0);
    }
  }

  prevCoreHP = coreHP;

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  hitHeight = getLocalBounds().height;

  animation.SetAnimation("ACID");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, *acid);

  animation.SetAnimation("HEAD");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, *head);

  animation.SetAnimation("SIDE");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, *side);

  this->AI<AlphaCore>::Update(_elapsed);
}

const bool AlphaCore::OnHit(const Hit::Properties props) {
    return true;
}

const float AlphaCore::GetHitHeight() const {
  return (float)hitHeight;
}

void AlphaCore::OnDelete() {
  if (virusBody) {
    this->RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;

  }

  // Explode if health depleted
  this->ChangeState<ExplodeState<AlphaCore>>(30, 1.5);
}

AlphaCore::AlphaCoreDefenseRule::AlphaCoreDefenseRule(int& alphaCoreHP) : DefenseRule(Priority(0)), alphaCoreHP(alphaCoreHP) {}
AlphaCore::AlphaCoreDefenseRule::~AlphaCoreDefenseRule() { }
const bool AlphaCore::AlphaCoreDefenseRule::Check(Spell* in, Character* owner) {
  // Drop a 0 damage hitbox to block/trigger attack hits
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());
  owner->Hit(Hit::Properties());

  if (alphaCoreHP <= 0) return false;
  alphaCoreHP -= in->GetHitboxProperties().damage;
  alphaCoreHP = std::max(-10, alphaCoreHP);

  return true;
}