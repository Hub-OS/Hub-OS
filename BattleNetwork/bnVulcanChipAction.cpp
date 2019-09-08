#include "bnVulcanChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnVulcan.h"

#define PATH "resources/spells/spell_vulcan.png"
#define ANIM "resources/spells/spell_vulcan.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.05 }
#define FRAME5 { 5, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1


VulcanChipAction::VulcanChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim.Update(0, *this->attachment);

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

  // On shoot frame, drop projectile
  auto onFire = [this, damage, owner]() -> void {
    Vulcan* b = new Vulcan(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    b->SetHitboxProperties(props);
    b->SetDirection(Direction::RIGHT);

    GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };


  this->AddAction(2, onFire);
  this->AddAction(4, onFire);
  this->AddAction(6, onFire);
}

VulcanChipAction::~VulcanChipAction()
{
}

void VulcanChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void VulcanChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
