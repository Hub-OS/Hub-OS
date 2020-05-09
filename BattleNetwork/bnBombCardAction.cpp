#include "bnBombCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnMiniBomb.h"

#include <Swoosh/Game.h>

#define PATH "resources/spells/spell_bomb.png"

BombCardAction::BombCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_THROW", &attachment, "Hand") {
  BombCardAction::damage = damage;

  overlay.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_MINI_BOMB));
  swoosh::game::setOrigin(overlay, 0.5, 0.5);

  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
}

BombCardAction::~BombCardAction()
{
}

void BombCardAction::Execute() {
  auto owner = GetUser();
  owner->AddNode(attachment);

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {
    attachment->Hide(); // the "bomb" is now airborn - hide the animation overlay

    auto duration = 0.5f; // seconds
    MiniBomb* b = new MiniBomb(GetUser()->GetField(), GetUser()->GetTeam(), owner->getPosition() + attachment->getPosition(), duration, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = GetUser();
    b->SetHitboxProperties(props);

    GetUser()->GetField()->AddEntity(*b, GetUser()->GetTile()->GetX() + 3, GetUser()->GetTile()->GetY());
  };


  AddAction(3, onThrow);
}

void BombCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void BombCardAction::EndAction()
{
  GetUser()->RemoveNode(attachment);
  GetUser()->EndCurrentAction();
}
