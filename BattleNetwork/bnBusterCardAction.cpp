#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"
#include "bnPlayer.h"
#include "bnCharacter.h"
#include "bnField.h"

#define NODE_PATH "resources/scenes/battle/spells/buster_shoot.png"
#define NODE_ANIM "resources/scenes/battle/spells/buster_shoot.animation"

#define FRAME_DATA { {1, frames(1)}, {2, frames(2)}, {3, frames(2)}, {4, frames(1)} }
#define CHARGED_FRAME_DATA { {6, frames(1)}, {2, frames(2)}, {3, frames(2)}, {4, frames(1)} }

frame_time_t BusterCardAction::CalculateCooldown(unsigned speedLevel, unsigned tileDist)
{
  speedLevel = std::min(std::max(speedLevel, 1u), 5u);
  tileDist = std::min(std::max(tileDist, 1u), 6u);

  static constexpr frame_time_t table[5][6] = {
    { frames(5), frames(9), frames(13), frames(17), frames(21), frames(25) },
    { frames(4), frames(8), frames(11), frames(15), frames(18), frames(21) },
    { frames(4), frames(7), frames(10), frames(13), frames(16), frames(18) },
    { frames(3), frames(5), frames(7),  frames(9),  frames(11), frames(13) },
    { frames(3), frames(4), frames(5),  frames(6),  frames(7) , frames(8)  }
  };

  return table[speedLevel - 1][tileDist - 1];
}

BusterCardAction::BusterCardAction(std::weak_ptr<Character> actorWeak, bool charged, int damage) : CardAction(actorWeak, "PLAYER_SHOOTING")
{
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  ToggleShowName(false);

  std::shared_ptr<Character> actor = actorWeak.lock();

  busterAttachment = &AddAttachment("buster");

  Animation& busterAnim = busterAttachment->GetAnimationObject();
  busterAnim.CopyFrom(actor->GetFirstComponent<AnimationComponent>()->GetAnimationObject());
  busterAnim.SetAnimation("BUSTER");

  buster = busterAttachment->GetSpriteNode();
  buster->setTexture(actor->getTexture());
  buster->SetLayer(-1);
  buster->EnableParentShader(true);
  buster->AddTags({Player::BASE_NODE_TAG});

  busterAnim.Refresh(buster->getSprite());

  if (charged) {
    OverrideAnimationFrames(CHARGED_FRAME_DATA);
  }
  else {
    OverrideAnimationFrames(FRAME_DATA);
  }

  SetLockout({ CardAction::LockoutType::animation });
}

void BusterCardAction::OnExecute(std::shared_ptr<Character> user) {
  buster->setColor(user->getColor());

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    Team team = user->GetTeam();

    Direction facing = user->GetFacing();
    Battle::Tile* tilePtr = user->GetTile() + facing;
    unsigned tileDist = 6u;
    unsigned speedLevel = 1u;

    if (tilePtr) {
      std::shared_ptr<Buster> b = std::make_shared<Buster>(team, charged, damage);
      std::shared_ptr<Field> field = user->GetField();

      b->SetMoveDirection(facing);

      field->AddEntity(b, *tilePtr);

      tileDist = 0u;

      do {
        tileDist++;

        auto list = tilePtr->FindHittableEntities([user](std::shared_ptr<Entity>& e) {
          return !user->Teammate(e->GetTeam());
        });

        if (list.empty()) {
          tilePtr = tilePtr + facing;
        }
        else break;
      } while (tilePtr);

      Player* asPlayer = dynamic_cast<Player*>(user.get());
      if (asPlayer) {
        speedLevel = asPlayer->GetSpeedLevel();
      }
    }

    int64_t frameCount = CalculateCooldown(speedLevel, tileDist).count();

    int poiseIndex = 4; // player frame arrangement must have 4 frames...
    Frame copyFrame = GetFrame(poiseIndex);
    copyFrame.duration = frames(1);

    for (; frameCount > 0u; frameCount--) {
      // copy the last frame for as many frames as we need
      InsertFrame(poiseIndex, copyFrame);
    }

    Audio().Play(AudioType::BUSTER_PEA);
  };

  AddAnimAction(2, onFire);

  auto onVisual = [this, user]() -> void {
    canMove = true;
    CardAction::Attachment& attachment = busterAttachment->AddAttachment("endpoint");

    std::shared_ptr<SpriteProxyNode> flare = attachment.GetSpriteNode();
    flare->setTexture(Textures().LoadFromFile(NODE_PATH));
    flare->SetLayer(-2);

    Animation& flareAnim = attachment.GetAnimationObject();
    flareAnim = Animation(NODE_ANIM);
    flareAnim.SetAnimation("DEFAULT");
  };

  AddAnimAction(3, onVisual);
}

void BusterCardAction::OnActionEnd()
{
}

void BusterCardAction::OnAnimationEnd()
{
  EndAction();
}

bool BusterCardAction::CanMoveInterrupt()
{
  return canMove;
}
