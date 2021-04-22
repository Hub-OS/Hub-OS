#include "bnBombCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnMiniBomb.h"
#include "bnField.h"

#include <Swoosh/Game.h>

#define PATH "resources/spells/spell_bomb.png"

BombCardAction::BombCardAction(Character& actor, int damage) : 
  CardAction(actor, "PLAYER_THROW") {
  BombCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().GetTexture(TextureType::SPELL_MINI_BOMB));
  attachment->SetLayer(-1);

  AddAttachment(actor, "hand", *attachment);
  swoosh::game::setOrigin(attachment->getSprite(), 0.5, 0.5);
}

BombCardAction::~BombCardAction()
{
}

void BombCardAction::OnExecute(Character* user) {
  // On throw frame, spawn projectile
  auto onThrow = [=]() -> void {
    attachment->Hide(); // the "bomb" is now airborn - hide the animation overlay

    auto team = user->GetTeam();
    auto duration = 0.5f; // seconds
    MiniBomb* b = new MiniBomb(team, user->getPosition() + attachment->getPosition(), duration, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = user->GetID();
    b->SetHitboxProperties(props);

    int step = 3;

    if (team == Team::blue) {
      step = -3;
    }

    user->GetField()->AddEntity(*b, user->GetTile()->GetX() + step, user->GetTile()->GetY());
  };


  AddAnimAction(3, onThrow);
}

void BombCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void BombCardAction::OnAnimationEnd()
{
}

void BombCardAction::OnEndAction()
{
  GetActor().RemoveNode(attachment);
}
