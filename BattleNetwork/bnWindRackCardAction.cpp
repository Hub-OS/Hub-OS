#include "bnWindRackCardAction.h"
#include "bnWind.h"
#include "bnField.h"
#include "bnSwordEffect.h"
#include "bnBasicSword.h"

#define FRAME1 { 1, 0.1666 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME4

WindRackCardAction::WindRackCardAction(Character& owner, int damage) :
  damage(damage),
  CardAction(owner, "PLAYER_SWORD") {
  hilt = new SpriteProxyNode();
  hilt->setTexture(owner.getTexture());
  hilt->SetLayer(-1);
  hilt->EnableParentShader(true);

  auto userAnim = GetCharacter().GetFirstComponent<AnimationComponent>();
  hiltAnim = Animation(userAnim->GetFilePath());
  hiltAnim.Reload();
  hiltAnim.SetAnimation("HILT");

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile("resources/spells/WindRack/attachment.png"));

  attachment->SetLayer(-1);
  attachment->EnableParentShader(false);

  attachmentAnim = Animation("resources/spells/WindRack/attachment.animation");
  attachmentAnim.SetAnimation("DEFAULT");

  OverrideAnimationFrames({ FRAMES });
}

WindRackCardAction::~WindRackCardAction()
{
  delete attachment;
  delete hilt;
}

void WindRackCardAction::ReplaceRack(SpriteProxyNode* node, const Animation& newAnim)
{
  if (attachment) {
    delete attachment;
    attachment = nullptr;
  }

  if (hilt) {
    delete hilt;
  }

  hilt = node;
  hiltAnim = newAnim;
}

void WindRackCardAction::OnExecute()
{
  auto owner = &GetCharacter();
  auto team = owner->GetTeam();
  auto field = owner->GetField();

  // On throw frame, spawn projectile
  auto onThrow = [this, team, owner, field]() -> void {
    auto userAnim = GetCharacter().GetFirstComponent<AnimationComponent>();
    auto& hiltAttachment = AddAttachment(userAnim->GetAnimationObject(), "HILT", *hilt).UseAnimation(hiltAnim);

    if (attachment) {
      hiltAttachment.AddAttachment(hiltAnim, "ENDPOINT", *attachment).UseAnimation(attachmentAnim);
    }

    Direction direction{ Direction::right };

    if (owner->GetTeam() == Team::blue) {
      direction = Direction::left;
    }

    // tiles
    auto tiles = std::vector{
      owner->GetTile()->Offset(1, 0),
      owner->GetTile()->Offset(1, 1),
      owner->GetTile()->Offset(1,-1)
    };

    // big push effect
    auto wind = new Wind(team);
    wind->Hide();
    wind->SetDirection(direction);
    field->AddEntity(*wind, *tiles[0]);

    wind = new Wind(team);
    wind->Hide();
    wind->SetDirection(direction);
    field->AddEntity(*wind, *tiles[1]);

    wind = new Wind(team);
    wind->Hide();
    wind->SetDirection(direction);
    field->AddEntity(*wind, *tiles[2]);

    // hitboxes

    SwordEffect* e = new SwordEffect;
    e->SetAnimation("WIDE");
    field->AddEntity(*e, *tiles[0]);

    BasicSword* b = new BasicSword(owner->GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = owner;
    b->SetHitboxProperties(props);

    Audio().Play(AudioType::SWORD_SWING);
    field->AddEntity(*b, *tiles[0]);

    b = new BasicSword(owner->GetTeam(), damage);
    b->SetHitboxProperties(props);
    field->AddEntity(*b, *tiles[1]);

    b = new BasicSword(owner->GetTeam(), damage);
    b->SetHitboxProperties(props);
    field->AddEntity(*b, *tiles[2]);
  };

  AddAnimAction(2, onThrow);
}

void WindRackCardAction::OnEndAction()
{
}

void WindRackCardAction::OnAnimationEnd()
{
}