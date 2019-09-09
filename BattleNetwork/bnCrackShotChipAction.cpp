#include "bnCrackShotChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCrackShot.h"

CrackShotChipAction::CrackShotChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SWORD", nullptr, "Buster") {

  // On throw frame, spawn projectile
  auto onThrow = [this, damage, owner]() -> void {

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

    if (tile && tile->IsWalkable() && tile->FindEntities([](Entity*e) { return true; }).size() == 0) {
      CrackShot* b = new CrackShot(GetOwner()->GetField(), GetOwner()->GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = GetOwnerAs<Character>();
      b->SetHitboxProperties(props);

      auto direction = (owner->GetTeam() == Team::RED) ? Direction::RIGHT : Direction::LEFT;
      b->SetDirection(direction);


      GetOwner()->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      AUDIO.Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::BROKEN);
    }
  };

  this->AddAction(2, onThrow); // TODO: should be frame 3, why does it not fire?
}

CrackShotChipAction::~CrackShotChipAction()
{
}

void CrackShotChipAction::OnUpdate(float _elapsed)
{
  ChipAction::OnUpdate(_elapsed);
}

void CrackShotChipAction::EndAction()
{
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
