#include "bnAlphaArm.h"
#include "bnMobMoveEffect.h"
#include "bnDefenseIndestructable.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"
#include <Swoosh/Ease.h>
#include <cmath>

#define RESOURCE_PATH "resources/mobs/alpha/alpha.animation"

AlphaArm::AlphaArm(Field* _field, Team _team, AlphaArm::Type type)
  : Obstacle(_field, _team), isMoving(false), canChangeTileState(false), type(type) {
  setScale(2.f, 2.f);
  SetFloatShoe(true);
  SetTeam(_team);
  SetDirection(Direction::left);
  SetHealth(999);
  ShareTileSpace(true);
  SetSlideTime(sf::seconds(0.1333f)); // 8 frames
  SetName("AlphaArm");
  Hit::Properties props = Hit::DefaultProperties;
  props.flags |= Hit::recoil | Hit::breaking;
  props.damage = 120;
  SetHitboxProperties(props);

  AddDefenseRule(new DefenseIndestructable(true));

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);

  totalElapsed = 0;

  setTexture(LOAD_TEXTURE(MOB_ALPHA_ATLAS));
  auto animComponent = CreateComponent<AnimationComponent>(this);
  animComponent->SetPath(RESOURCE_PATH);
  animComponent->Load();

  blueShadow = new SpriteProxyNode();
  blueShadow->setTexture(LOAD_TEXTURE(MOB_ALPHA_ATLAS));
  blueShadow->SetLayer(1);

  Animation blueShadowAnim(animComponent->GetFilePath());
  blueShadowAnim.Load();
  blueShadowAnim.SetAnimation("RIGHT_CLAW_SWIPE_SHADOW");
  blueShadowAnim.Update(0, blueShadow->getSprite());

  switch (type) {
  case Type::LEFT_IDLE:
    animComponent->SetAnimation("LEFT_CLAW_DEFAULT");
    AddNode(shadow);
    break;
  case Type::RIGHT_IDLE:
    animComponent->SetAnimation("RIGHT_CLAW_DEFAULT");
    AddNode(shadow);
    break;
  case Type::RIGHT_SWIPE:
    animComponent->SetAnimation("RIGHT_CLAW_SWIPE");
    SetSlideTime(sf::seconds(0.13f)); // 8 frames in 60 seconds
    SetDirection(Direction::down);
    AddNode(shadow);

    blueArmShadowPos = {
      sf::Vector2f(0, -80.0f), sf::Vector2f(0, -10.0f),
      sf::Vector2f(0, -80.0f), sf::Vector2f(0, -10.0f),
      sf::Vector2f(0, -80.0f), sf::Vector2f(0, -10.0f),  sf::Vector2f(0, -40.0f)
    };

    blueArmShadowPosIdx = 0;
    blueShadowTimer = 0.0f;

    blueShadow->setPosition(0, -10.0f);
    blueShadow->Hide();
    AddNode(blueShadow);
    break;
  case Type::LEFT_SWIPE:
    animComponent->SetAnimation("LEFT_CLAW_SWIPE");
    SetSlideTime(sf::seconds(0.13f)); // 8 frames in 60 seconds
    SetDirection(Direction::left);
    changeState = (rand() % 10 < 5) ? TileState::poison : TileState::ice;
    SetLayer(-1);
    break;
  }

  isSwiping = isFinished = false;
  animComponent->OnUpdate(0);
}

AlphaArm::~AlphaArm() {
  // RemoveNode(shadow);
  delete shadow;
  delete blueShadow;
}

bool AlphaArm::CanMoveTo(Battle::Tile * next)
{
  return true;
}

void AlphaArm::OnUpdate(float _elapsed) {
  if (isFinished) { Delete(); }

  totalElapsed += _elapsed;
  float delta = std::sin(10.0f*totalElapsed+1.0f);

  if (canChangeTileState) {
    // golden claw
    setColor(sf::Color::Yellow);
  }

  if (type == Type::RIGHT_SWIPE) {
    blueShadowTimer += _elapsed;
    
    if (blueShadowTimer > 1.0f / 60.0f) {
      blueShadowTimer = 0;
      blueArmShadowPosIdx++;
    }

    blueArmShadowPosIdx = blueArmShadowPosIdx % blueArmShadowPos.size();

    blueShadow->setPosition(blueArmShadowPos[blueArmShadowPosIdx]);

    delta = GetTile()->GetHeight();

    if (totalElapsed > 1.0f) {
      if (!isSwiping) {
        isSwiping = true;
        AUDIO.Play(AudioType::SWORD_SWING);
      }

      blueShadow->Reveal();

      delta = (1.0f - swoosh::ease::linear(totalElapsed - 1.0f, 0.12f, 1.0f)) * GetTile()->GetHeight();

      if (totalElapsed - 1.0f > 0.12f) {
        // May have just finished moving
        if (!Teammate(GetTile()->GetTeam())) {
          if (canChangeTileState) {
            GetTile()->SetTeam(GetTeam());
          }
        }

        // Keep moving
        if (!IsSliding()) {
          SlideToTile(true);
          Move(GetDirection());

          if (!GetNextTile()) {
            isFinished = true;
          }
        }
      }
    }
  }
  else if (type == Type::LEFT_SWIPE) {
    delta = 0; // do not bob

      // May have just finished moving
    if (totalElapsed > 1.2f) {
      if (!isSwiping) {
        isSwiping = true;
        AUDIO.Play(AudioType::TOSS_ITEM_LITE);
      }

      if (!Teammate(GetTile()->GetTeam()) && GetTile()->IsWalkable()) {
        if (canChangeTileState) {
          GetTile()->SetState(changeState);
        }
      }

      // Keep moving
      if (!IsSliding()) {
        SlideToTile(true);
        Move(GetDirection());

        if (!GetNextTile()) {
          isFinished = true;
        }
      }
    }
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - GetHeight() - delta);

  shadow->setPosition(-13, -4 + delta / 2.0f); // counter offset the shadow node

  if (isSwiping) {
      auto res = GetTile()->FindEntities([this](Entity* in) -> bool {
        if (dynamic_cast<Obstacle*>(in) && in != this) {
          Delete();
          return true;
        }

        return false;
      });

      // Only if we are not blocked by an obstacle, can we proceed and attack
      if (res.size() == 0) {
        GetTile()->AffectEntities(this);
      }
  }
}

void AlphaArm::OnDelete() {
  if (GetTile()) {
    auto fx = new MobMoveEffect(GetField());
    GetField()->AddEntity(*fx, *GetTile());
  }

  Remove();
}

void AlphaArm::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    Delete(); // cannot pass through obstacles like Cube
    return;
  }

  other->Hit(GetHitboxProperties());
}

const float AlphaArm::GetHeight() const
{
  switch (type) {
  case Type::RIGHT_IDLE:
    return 10; break;
  case Type::LEFT_IDLE:
    return 10; break;
  case Type::RIGHT_SWIPE:
    return 30; break;
  case Type::LEFT_SWIPE:
    return 10; break;
  }

  return 0;
}

const bool AlphaArm::IsSwiping() const
{
  return isSwiping;
}

void AlphaArm::SyncElapsedTime(const float elapsedTime)
{
  // the claws get out of sync, we must sync them up
  totalElapsed = elapsedTime;
}

void AlphaArm::LeftArmChangesTileState()
{
  canChangeTileState = true;
}

void AlphaArm::RightArmChangesTileTeam()
{
  canChangeTileState = true;
}

const bool AlphaArm::GetIsFinished() const
{
  return isFinished;
}
