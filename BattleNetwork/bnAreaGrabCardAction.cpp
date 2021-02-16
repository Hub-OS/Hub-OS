#include "bnAreaGrabCardAction.h"
#include "bnCharacter.h"
#include "bnPanelGrab.h"
#include "bnField.h"

AreaGrabCardAction::AreaGrabCardAction(Character& owner, int damage) : 
  damage(damage),
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ ActionLockoutType::sequence });
}

void AreaGrabCardAction::OnExecute() {
  auto owner = GetOwner();
  Field* f = owner->GetField();
  PanelGrab** grab = new PanelGrab * [3];

  for (int i = 0; i < 3; i++) {
    grab[i] = new PanelGrab(owner->GetTeam(), 0.25f);
  }

  Battle::Tile** tile = new Battle::Tile * [3];

  // Read team grab scans from left to right
  if (owner->GetTeam() == Team::red) {
    int minIndex = 6;

    for (int i = 0; i < f->GetHeight(); i++) {
      int index = 1;
      while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::red) {
        index++;
      }

      minIndex = std::min(minIndex, index);
    }

    for (int i = 0; i < f->GetHeight(); i++) {
      tile[i] = f->GetAt(minIndex, i + 1);

      if (tile[i]) {
        auto status = f->AddEntity(*grab[i], tile[i]->GetX(), tile[i]->GetY());

        if (status != Field::AddEntityStatus::deleted) {
          panelPtr = grab[i];
        }
      }
      else {
        delete grab[i];
      }
    }
  }
  else if (owner->GetTeam() == Team::blue) {
    // Blue team grab scans from right to left

    int maxIndex = 1;

    for (int i = 0; i < f->GetHeight(); i++) {
      int index = f->GetWidth();
      while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::blue) {
        index--;
      }

      maxIndex = std::max(maxIndex, index);
    }

    for (int i = 0; i < f->GetHeight(); i++) {
      tile[i] = f->GetAt(maxIndex, i + 1);

      if (tile[i]) {
        auto status = f->AddEntity(*grab[i], tile[i]->GetX(), tile[i]->GetY());

        if (status != Field::AddEntityStatus::deleted) {
          panelPtr = grab[i];
        }
      }
      else {
        delete grab[i];
      }
    }
  }

  // cleanup buckets
  delete[] tile;
  delete[] grab;

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

void AreaGrabCardAction::OnUpdate(double _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void AreaGrabCardAction::OnAnimationEnd()
{
}

void AreaGrabCardAction::OnEndAction()
{
  GetOwner()->Reveal();
  Eject();
}
