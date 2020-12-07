#include "bnBombCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnMiniBomb.h"

#include <Swoosh/Game.h>

#define PATH "resources/spells/spell_bomb.png"

BombCardAction::BombCardAction(Character * owner, int damage) : CardAction(*owner, "PLAYER_THROW") {
  BombCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().GetTexture(TextureType::SPELL_MINI_BOMB));
  attachment->SetLayer(-1);

  AddAttachment(*owner, "hand", *attachment);
  swoosh::game::setOrigin(attachment->getSprite(), 0.5, 0.5);
}

BombCardAction::~BombCardAction()
{
}

void BombCardAction::OnExecute() {
  // On throw frame, spawn projectile
  auto onThrow = [this]() -> void {
    auto& owner = GetUser();

    attachment->Hide(); // the "bomb" is now airborn - hide the animation overlay

    auto team = owner.GetTeam();
    auto duration = 0.5f; // seconds
    MiniBomb* b = new MiniBomb(owner.GetField(), team, owner.getPosition() + attachment->getPosition(), duration, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = &GetUser();
    b->SetHitboxProperties(props);

    int step = 3;

    if (team == Team::blue) {
      step = -3;
    }

    owner.GetField()->AddEntity(*b, owner.GetTile()->GetX() + step, owner.GetTile()->GetY());
  };


  AddAnimAction(3, onThrow);
}

void BombCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void BombCardAction::OnAnimationEnd()
{
}

void BombCardAction::OnEndAction()
{
  GetOwner()->RemoveNode(attachment);
  Eject();
}
