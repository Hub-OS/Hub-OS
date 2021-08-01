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

    int step = 3;

    Direction dir = Direction::right;
    auto team = user->GetTeam();
    if (team == Team::blue) {
      step = -3;
      dir = Direction::left;
    }

    auto duration = 0.5f; // seconds
    
    if (auto target = user->GetTile(dir, step)) {
      auto offset = (user->GetTile()->getPosition() + attachment->getPosition()) - target->getPosition();
      MiniBomb* b = new MiniBomb(team, offset, duration, damage);
      auto props = b->GetHitboxProperties();
      props.aggressor = user->GetID();
      b->SetHitboxProperties(props);

      user->GetField()->AddEntity(*b, *target);
    }
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

void BombCardAction::OnActionEnd()
{
  GetActor().RemoveNode(attachment);
}
