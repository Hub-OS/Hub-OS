#include "bnMachGunCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSuperVulcan.h"
#include "bnCharacter.h"

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

MachGunCardAction::MachGunCardAction(Character* owner, int damage) :
  CardAction(*owner, "PLAYER_SHOOTING"),
  damage(damage)
{
  machgun.setTexture(TEXTURES.LoadTextureFromFile("resources/spells/machgun_buster.png"));
  machgun.SetLayer(-1);

  machgunAnim = Animation("resources/spells/machgun_buster.animation") << "FIRE";

  AddAttachment(*owner, "BUSTER", machgun).UseAnimation(machgunAnim);

  OverrideAnimationFrames({ FRAMES });
}

MachGunCardAction::~MachGunCardAction()
{
}

void MachGunCardAction::Execute()
{
  auto shoot = [this]() {
    auto* owner = GetOwner();
    auto* field = owner->GetField();

    if (!target) {
      // find one 
      auto ents = field->FindEntities([owner](Entity* e) {
        Team team = e->GetTeam();
        Character* character = dynamic_cast<Character*>(e);
        return character && team != owner->GetTeam() && team != Team::unknown;
      });

      if (ents.empty() == false) {
        std::sort(ents.begin(), ents.end(), [](Entity* A, Entity* B) {return  A->GetTile()->Distance(*B->GetTile()) < 0; });
        target = ents[0];
      }
    }

    if (!targetTile && target) {
      targetTile = field->GetAt(target->GetTile()->GetX(), 3);
    }

    this->MoveRectical(field);

    // Spawn rectical where the targetTile is positioned which will attack for us
    if (targetTile) {
      field->AddEntity(*new Target(field, this->damage), *targetTile);
    }
  };

  // shoots 9 times total
  for (int i = 0; i < 9; i++) {
    AddAnimAction(2+(i*5), shoot);
  }
}

void MachGunCardAction::EndAction()
{
  Eject();
}

void MachGunCardAction::OnAnimationEnd()
{
}

void MachGunCardAction::FreeTarget()
{
  target = nullptr;
}

void MachGunCardAction::MoveRectical(Field* field)
{
  // Figure out where our last rectical was
  if (target && targetTile) {
    auto* charTile = target->GetTile();
    Battle::Tile* nextTile = nullptr;

    if (moveOneCol && charTile->GetX() != targetTile->GetX()) {
      if (charTile->GetX() < targetTile->GetX()) {
        moveOneCol = false;
        nextTile = field->GetAt(targetTile->GetX() - 1, targetTile->GetY());

        if (nextTile->IsEdgeTile()) {
          nextTile = field->GetAt(targetTile->GetX() + 1, targetTile->GetY());
        }

        targetTile = nextTile;

      }
      else if (charTile->GetX() > targetTile->GetX()) {
        moveOneCol = false;
        nextTile = field->GetAt(targetTile->GetX() + 1, targetTile->GetY());

        if (nextTile->IsEdgeTile()) {
          nextTile = field->GetAt(targetTile->GetX() - 1, targetTile->GetY());
        }

        targetTile = nextTile;

      }
    } 
    else {
      int step = moveUp ? -1 : 1;

      nextTile = field->GetAt(targetTile->GetX(), targetTile->GetY() + step);

      if (nextTile->IsEdgeTile()) {
        moveUp = !moveUp;
        moveOneCol = true;
        MoveRectical(field);
      }
      else {
        targetTile = nextTile;
      }
    }
  }
}


// class Target : public Artifact

Target::Target(Field* field, int damage) :
  Artifact(field),
  damage(damage),
  attack(frames(10).asSeconds())
{
  setScale(2.f, 2.f);
  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/target.png"));
  anim = Animation("resources/spells/target.animation") << "DEFAULT";
  anim.Update(0, getSprite());
}

Target::~Target()
{
}

void Target::OnSpawn(Battle::Tile& start)
{
  start.RequestHighlight(Battle::Tile::Highlight::solid);
}

void Target::OnUpdate(float elapsed)
{
  attack -= elapsed;

  if (attack <= 0) {
    auto* vulcan = new SuperVulcan(GetField(), GetTeam(), damage);
    GetField()->AddEntity(*vulcan, *GetTile());
    this->Remove();
  }
}

void Target::OnDelete()
{
}
