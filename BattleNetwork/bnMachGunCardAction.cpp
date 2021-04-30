#include "bnMachGunCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnSuperVulcan.h"
#include "bnCharacter.h"
#include "bnObstacle.h"

#define WAIT   { 1, 0.0166 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.033 }

#define FRAMES WAIT, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, 

MachGunCardAction::MachGunCardAction(Character& actor, int damage) :
  CardAction(actor, "PLAYER_SHOOTING"),
  damage(damage)
{
  machgun.setTexture(Textures().LoadTextureFromFile("resources/spells/machgun_buster.png"));
  machgun.SetLayer(-1);

  machgunAnim = Animation("resources/spells/machgun_buster.animation") << "FIRE";

  AddAttachment(actor, "BUSTER", machgun).UseAnimation(machgunAnim);

  OverrideAnimationFrames({ FRAMES });
}

MachGunCardAction::~MachGunCardAction()
{
}

void MachGunCardAction::OnExecute(Character* user)
{
  auto shoot = [this]() {
    auto& actor = GetActor();
    auto* field = actor.GetField();

    if (target == nullptr || target->WillRemoveLater()) {
      // find the closest
      auto ents = field->FindEntities([actor = &actor](Entity* e) {
        Team team = e->GetTeam();
        Character* character = dynamic_cast<Character*>(e);
        Obstacle* obst = dynamic_cast<Obstacle*>(e);
        return character && !obst && !e->WillRemoveLater()
               && team != actor->GetTeam() && team != Team::unknown;
      });

      if (!ents.empty()) {
        auto filter = [actor = &actor](Entity* A, Entity* B) { return A->GetTile()->GetX() < B->GetTile()->GetX(); };
        std::sort(ents.begin(), ents.end(), filter);

        for (auto e : ents) {
          if (e->GetTile()->GetX() > actor.GetTile()->GetX()) {
            target = e;
            break;
          }
        }

        if (target) {
          auto* removeCallback = target->CreateRemoveCallback();
          removeCallback->Slot([this](Entity* in) {
            this->target = nullptr;
          });

          removeCallbacks.push_back(removeCallback);
        }
      }
    }

    if (!targetTile && target) {
      targetTile = field->GetAt(target->GetTile()->GetX(), 3);
    }
    else if (!targetTile && !target) {
      // pick back col
      targetTile = field->GetAt(6, 3);
    }

    // We initially spawn the rectical where we want to start
    // Do note move it around
    if (!firstSpawn) {
      targetTile = this->MoveRectical(field, false);
    }
    else {
      firstSpawn = false;
    }

    // Spawn rectical where the targetTile is positioned which will attack for us
    if (targetTile) {
      field->AddEntity(*new Target(this->damage), *targetTile);
    }
  };

  // shoots 9 times total
  for (int i = 0; i < 9; i++) {
    AddAnimAction(2+(i*5), shoot);
  }
}

void MachGunCardAction::OnActionEnd()
{
  for (auto* callback : removeCallbacks) {
    delete callback;
  }

  removeCallbacks.clear();
}

void MachGunCardAction::OnAnimationEnd()
{
}

void MachGunCardAction::FreeTarget()
{
  target = nullptr;
}

Battle::Tile* MachGunCardAction::MoveRectical(Field* field, bool colMove)
{
  // Figure out where our last rectical was
  if (target && targetTile) {
    auto* charTile = target->GetTile();

    if (!charTile) return targetTile;

    Battle::Tile* nextTile = nullptr;

    if (colMove && charTile->GetX() != targetTile->GetX()) {
      if (charTile->GetX() < targetTile->GetX()) {
        nextTile = field->GetAt(targetTile->GetX() - 1, targetTile->GetY());

        if (nextTile->IsEdgeTile()) {
          nextTile = field->GetAt(targetTile->GetX() + 1, targetTile->GetY());
        }

        return nextTile;
      }
      else if (charTile->GetX() > targetTile->GetX()) {
        nextTile = field->GetAt(targetTile->GetX() + 1, targetTile->GetY());

        if (nextTile->IsEdgeTile()) {
          nextTile = field->GetAt(targetTile->GetX() - 1, targetTile->GetY());
        }

        return nextTile;
      }
    } 
 
    // If you cannot move left/right keep moving up/down
    int step = moveUp ? -1 : 1;

    nextTile = field->GetAt(targetTile->GetX(), targetTile->GetY() + step);

    if (nextTile->IsEdgeTile()) {
      moveUp = !moveUp;
      return MoveRectical(field, true);
    }
    else {
      return nextTile;
    }
  }

  return targetTile;
}


// class Target : public Artifact

Target::Target(int damage) :
  Artifact(),
  damage(damage),
  attack(seconds_cast<double>(frames(5)))
{
  setScale(2.f, 2.f);
  setTexture(Textures().LoadTextureFromFile("resources/spells/target.png"));
  anim = Animation("resources/spells/target.animation") << "DEFAULT";
  anim.Refresh(getSprite());
}

Target::~Target()
{
}

void Target::OnSpawn(Battle::Tile& start)
{
}

void Target::OnUpdate(double elapsed)
{
  attack -= elapsed;

  GetTile()->RequestHighlight(Battle::Tile::Highlight::flash);

  if (attack <= 0) {
    auto* vulcan = new SuperVulcan(GetTeam(), damage);
    GetField()->AddEntity(*vulcan, *GetTile());
    this->Remove();
  }
}

void Target::OnDelete()
{
}
