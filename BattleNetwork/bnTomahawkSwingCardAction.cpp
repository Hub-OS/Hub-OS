#include "bnTomahawkSwingCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAnimationComponent.h"
#include "bnArtifact.h"
#include "bnHitbox.h"

TomahawkSwingCardAction::TomahawkSwingCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_CHOP"),
  damage(damage)
{

}

TomahawkSwingCardAction::~TomahawkSwingCardAction()
{
}

void TomahawkSwingCardAction::OnExecute()
{
  auto spawn = [this] {
    auto* tile = GetOwner()->GetTile();
    auto* field = GetOwner()->GetField();
    field->AddEntity(*new TomahawkEffect(field), tile->GetX() + 1, tile->GetY());

    for (auto col : { 1, 2 }) {
      for (auto row : { 1, 0, -1 }) {
        auto* hitbox = new Hitbox(field, GetOwner()->GetTeam(), damage);
        auto props = hitbox->GetHitboxProperties();
        props.flags |= Hit::flinch;
        props.aggressor = GetOwner();
        hitbox->SetHitboxProperties(props);
        field->AddEntity(*hitbox, tile->GetX() + col, tile->GetY() + row);
      }
    }
  };

  AddAnimAction(4, spawn);
}

void TomahawkSwingCardAction::OnEndAction()
{
  Eject();
}

void TomahawkSwingCardAction::OnAnimationEnd()
{
}

// class TomahawkEffect

TomahawkEffect::TomahawkEffect(Field* field) : Artifact(field)
{
  SetLayer(-10); // be on top
  setTexture(Textures().LoadTextureFromFile("resources/navis/megaman/forms/tomahawk_swing.png"));
  setScale(2.f, 2.f);

  //Components setup and load
  auto animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/navis/megaman/forms/tomahawk_swing.animation");
  animation->Reload();

  // Create a callback
  // When animation ends
  // delete this effect
  auto onEnd = [this]() {
    Delete();
  };
  animation->SetAnimation("DEFAULT", onEnd);

  // Use the first rect in the frame list
  animation->SetFrame(0);
}

void TomahawkEffect::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition());
}

void TomahawkEffect::OnDelete()
{
  Remove();
}

TomahawkEffect::~TomahawkEffect()
{
}
