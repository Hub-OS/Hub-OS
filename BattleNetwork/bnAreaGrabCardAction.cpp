#include "bnAreaGrabCardAction.h"
#include "bnCharacter.h"
#include "bnPanelGrab.h"
#include "bnField.h"

AreaGrabCardAction::AreaGrabCardAction(Character& owner, int damage) : 
  damage(damage),
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void AreaGrabCardAction::OnExecute() {
  auto& owner = GetCharacter();
  Field* f = owner.GetField();

  // Strategy: 1. Scanning will happen tile-by-tile from the direction the user is facing
  //           2. Scan the field for the first fully owned column, starting the furthest column behind the user
  //           3. When found, start the second scan from that column
  //           4. From the new start, scan for the first column with enemy team tiles
  //           5. If found, spawn panel grab attacks on all tiles in that column

  // find our team's fully owned column
  int column = owner.GetFacing() == Direction::left ? owner.GetField()->GetWidth() : 0;
  int lastColumn = owner.GetField()->GetWidth() - column;
  int inc = owner.GetFacing() == Direction::left ? -1 : 1;
  int teamColumnStart = column;

  for (int i = column; i != lastColumn; i += inc) {
    bool allSameTeam = true;
    for (int j = 1; j <= 3; j++) {
      allSameTeam = allSameTeam && owner.Teammate(f->GetAt(i, j)->GetTeam());
    }

    if (allSameTeam) {
      teamColumnStart = i;
      break;
    }
  }

  // find a column with at least one of our enemies tiles
  for (int i = teamColumnStart; i != lastColumn; i += inc) {
    bool allSameTeam = true;
    for (int j = 1; j <= 3; j++) {
      allSameTeam = allSameTeam && owner.Teammate(f->GetAt(i, j)->GetTeam());
    }

    // steal this column
    if (!allSameTeam) {
      for (int j = 1; j <= 3; j++) {
        auto panelGrab = new PanelGrab(owner.GetTeam(), owner.GetFacing(), 0.25);
        auto result = f->AddEntity(*panelGrab, i, j);

        if (result != Field::AddEntityStatus::deleted && !panelPtr) {
          panelPtr = panelGrab;
        }
      }

      break;
    }
  }

  CardAction::Step step;
  step.updateFunc = [this](double elapsed, Step& self) {
    if (panelPtr == nullptr || panelPtr->WillRemoveLater()) {
      self.markDone();
    }
  };

  AddStep(step);
}

AreaGrabCardAction::~AreaGrabCardAction()
{
}

void AreaGrabCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void AreaGrabCardAction::OnAnimationEnd()
{
}

void AreaGrabCardAction::OnEndAction()
{
  GetCharacter().Reveal();
}
