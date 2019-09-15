#include "bnAlphaCore.h"
#include "bnAlphaArm.h"
#include "bnObstacle.h"
#include "bnMobMoveEffect.h"
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
  coreHP = prevCoreHP = 40;
  coreRegen = 0;
  setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  setScale(2.f, 2.f);

  SetName("Alpha");
  this->SetHealth(1000);

  firstTime = true;

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
  acid->SetLayer(1);
  acid->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  animation.SetAnimation("ACID");
  animation.Update(0, *acid);

  head = new SpriteSceneNode();
  head->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  head->SetLayer(-2);
  animation.SetAnimation("HEAD");
  animation.Update(0, *head);

  side = new SpriteSceneNode();
  side->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  side->SetLayer(-1);
  animation.SetAnimation("SIDE");
  animation.Update(0, *side);

  leftShoulder = new SpriteSceneNode();
  leftShoulder->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  leftShoulder->SetLayer(0);
  animation.SetAnimation("LEFT_SHOULDER");
  animation.Update(0, *leftShoulder);

  rightShoulder = new SpriteSceneNode();
  rightShoulder->setTexture(*TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  rightShoulder->SetLayer(-3);
  animation.SetAnimation("RIGHT_SHOULDER");
  animation.Update(0, *rightShoulder);

  this->AddNode(acid);
  this->AddNode(head);
  this->AddNode(side);
  this->AddNode(rightShoulder);
  this->AddNode(leftShoulder);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);

  defense = new AlphaCoreDefenseRule(coreHP);
  this->AddDefenseRule(defense);

  // arms
  rightArm = new AlphaArm(nullptr, GetTeam(), AlphaArm::Type::RIGHT_IDLE);
  leftArm = new AlphaArm(nullptr, GetTeam(), AlphaArm::Type::LEFT_IDLE);
}

AlphaCore::~AlphaCore() {
  delete head;
  delete acid, side;
}

void AlphaCore::OnUpdate(float _elapsed) {
  // TODO: Add a OnEnterField() for these types of things...
  if (firstTime) {
    this->RevealLeftArm();
    this->RevealRightArm();

    // Block player from stealing rows
    Battle::Tile* block = GetField()->GetAt(4, 1);
    block->ReserveEntityByID(this->GetID());

    block = GetField()->GetAt(4, 2);
    block->ReserveEntityByID(this->GetID());

    block = GetField()->GetAt(4, 3);
    block->ReserveEntityByID(this->GetID());

    firstTime = false;
  }

  totalElapsed += _elapsed;

  leftArm->SyncElapsedTime(totalElapsed+1.0f);
  rightArm->SyncElapsedTime(totalElapsed+1.0f);

  float delta = std::sinf(10*totalElapsed)*0.5f;
  head->setPosition(-10, -44 - delta);
  side->setPosition(6, -54 - delta);

  // shoulders lag behind
  delta = std::sinf(10 * totalElapsed+1.5f)*0.5f;
  leftShoulder->setPosition(-21, -58 - delta);
  rightShoulder->setPosition(2, -53 - delta);

  if (coreHP < 40) {
    coreRegen += _elapsed;

    // regen after 4 seconds
    if (coreRegen >= 4) {
      coreRegen = 0;
      coreHP += 20;
    }

    coreHP = std::min(40, coreHP);

    if (prevCoreHP != coreHP) {
      if (prevCoreHP > coreHP) {
      //  coreRegen = 0; // restart the regen timer
      }

      if (coreHP <= 0) {
        this->animationComponent->SetAnimation("CORE_EXPOSED");
        this->animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }
      else if (coreHP <= 20) {
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

  leftArm->Delete();
  rightArm->Delete();

  AUDIO.StopStream();

  // Explode if health depleted
  this->ChangeState<ExplodeState<AlphaCore>>(30, 1.5);
}

void AlphaCore::OpenShoulderGuns()
{
  animation.SetAnimation("LEFT_SHOULDER");
  animation.SetFrame(2, *leftShoulder);

  animation.SetAnimation("RIGHT_SHOULDER");
  animation.SetFrame(2, *rightShoulder);
}

void AlphaCore::CloseShoulderGuns()
{
  animation.SetAnimation("LEFT_SHOULDER");
  animation.SetFrame(1, *leftShoulder);

  animation.SetAnimation("RIGHT_SHOULDER");
  animation.SetFrame(1, *rightShoulder);
}

void AlphaCore::HideLeftArm()
{
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, leftArm->GetTile()->GetX(), leftArm->GetTile()->GetY());
  leftArm->GetTile()->ReserveEntityByID(leftArm->GetID());
  leftArm->GetTile()->RemoveEntityByID(leftArm->GetID());

}

void AlphaCore::RevealLeftArm()
{
  GetField()->AddEntity((*leftArm), GetTile()->GetX(), GetTile()->GetY() + 1);
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, GetTile()->GetX(), GetTile()->GetY() + 1);
}

void AlphaCore::HideRightArm()
{
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, rightArm->GetTile()->GetX(), rightArm->GetTile()->GetY());
  rightArm->GetTile()->ReserveEntityByID(rightArm->GetID());
  rightArm->GetTile()->RemoveEntityByID(rightArm->GetID());
}

void AlphaCore::RevealRightArm()
{
  GetField()->AddEntity((*rightArm), GetTile()->GetX() - 1, GetTile()->GetY() - 1);
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, GetTile()->GetX() - 1, GetTile()->GetY() - 1);
}

AlphaCore::AlphaCoreDefenseRule::AlphaCoreDefenseRule(int& alphaCoreHP) : DefenseRule(Priority(0)), alphaCoreHP(alphaCoreHP) {}
AlphaCore::AlphaCoreDefenseRule::~AlphaCoreDefenseRule() { }
const bool AlphaCore::AlphaCoreDefenseRule::Check(Spell* in, Character* owner) {
  // Drop a 0 damage hitbox to block/trigger attack hits
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());

  if ((in->GetHitboxProperties().flags & Hit::breaking) == Hit::breaking) {
    alphaCoreHP = 0;
  }

  if (alphaCoreHP <= 0) return false;
  alphaCoreHP -= in->GetHitboxProperties().damage;
  alphaCoreHP = std::max(0, alphaCoreHP);

  return true;
}

Hit::Properties & AlphaCore::AlphaCoreDefenseRule::FilterStatuses(Hit::Properties & statuses)
{
  statuses.flags &= ~Hit::drag;
  return statuses;
}
