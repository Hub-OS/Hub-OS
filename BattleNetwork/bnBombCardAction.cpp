#include "bnBombCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnMiniBomb.h"

#include <Swoosh/Game.h>

#define PATH "resources/spells/spell_bomb.png"

BombCardAction::BombCardAction(Character& user, int damage) : CardAction(user, "PLAYER_THROW") {
  BombCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().GetTexture(TextureType::SPELL_MINI_BOMB));
  attachment->SetLayer(-1);
  swoosh::game::setOrigin(attachment->getSprite(), 0.5, 0.5);

  AddAttachment(anim->GetAnimationObject(), "hand", *attachment);

}

BombCardAction::~BombCardAction()
{
}

void BombCardAction::OnExecute() {
  // On throw frame, spawn projectile
  auto onThrow = [this]() -> void {
    auto& owner = GetUser();

    attachment->Hide(); // the "bomb" is now airborn - hide the animation overlay

    auto duration = 0.5f; // seconds
    MiniBomb* b = new MiniBomb(user.GetField(), user.GetTeam(), owner.getPosition() + attachment->getPosition(), duration, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = &GetUser();
    b->SetHitboxProperties(props);

    user.GetField()->AddEntity(*b, user.GetTile()->GetX() + 3, user.GetTile()->GetY());
  };


  AddAction(3, onThrow);
}

void BombCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
