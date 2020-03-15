#include "bnBombCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnMiniBomb.h"

#define PATH "resources/spells/spell_bomb.png"

BombCardAction::BombCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_THROW", &attachment, "Hand") {
  this->damage = damage;

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteProxyNode(overlay);
  this->attachment->SetLayer(-1);
}

BombCardAction::~BombCardAction()
{
}

void BombCardAction::Execute() {
  auto owner = GetOwner();
  owner->AddNode(this->attachment);

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {
    attachment->Hide(); // the "bomb" is now airborn - hide the animation overlay

    auto duration = 0.5f; // seconds
    MiniBomb* b = new MiniBomb(GetOwner()->GetField(), GetOwner()->GetTeam(), owner->getPosition() + attachment->getPosition(), duration, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    b->SetHitboxProperties(props);

    GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 3, GetOwner()->GetTile()->GetY());
  };


  this->AddAction(3, onThrow);
}

void BombCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void BombCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
