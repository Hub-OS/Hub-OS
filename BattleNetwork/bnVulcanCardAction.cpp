#include "bnVulcanCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnVulcan.h"

#define PATH "resources/spells/spell_vulcan.png"
#define ANIM "resources/spells/spell_vulcan.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1


VulcanCardAction::VulcanCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  this->damage = damage;
  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteProxyNode(overlay);
  this->attachment->SetLayer(-1);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

VulcanCardAction::~VulcanCardAction()
{
}

void VulcanCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
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

void VulcanCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  CardAction::OnUpdate(_elapsed);
}

void VulcanCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
