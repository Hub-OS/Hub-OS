#include "bnLightningCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnHitbox.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME3, FRAME3

#define LIGHTNING_IMG "resources/spells/spell_lightning.png"
#define LIGHTNING_ANI "resources/spells/spell_lightning.animation"

LightningCardAction::LightningCardAction(Character * owner, int damage) :
  CardAction(*owner, "PLAYER_SHOOTING")
{
  LightningCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(owner->getTexture());
  attachment->SetLayer(-1);

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");

  attack = new SpriteProxyNode();
  attack->setTexture(LOAD_TEXTURE_FILE(LIGHTNING_IMG));
  attack->SetLayer(-2);

  attackAnim = Animation(LIGHTNING_ANI);
  attackAnim.SetAnimation("DEFAULT");

  AddAttachment(*owner, "buster", *attachment).PrepareAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void LightningCardAction::Execute() {
  auto owner = GetOwner();

  attachment->EnableParentShader(true);

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    AUDIO.Play(AudioType::SPREADER);

    attachment->AddNode(attack);
    attack->setPosition(attachmentAnim.GetPoint("endpoint"));

    attackAnim.Update(0, attack->getSprite());

    auto field = GetOwner()->GetField();
    auto team = GetOwner()->GetTeam();
    int col = GetOwner()->GetTile()->GetX();
    int row = GetOwner()->GetTile()->GetY();

    for (int i = 1; i < 6; i++) {
      auto hitbox = new Hitbox(field, team, LightningCardAction::damage);
      hitbox->HighlightTile(Battle::Tile::Highlight::solid);
      auto props = hitbox->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      props.damage = LightningCardAction::damage;
      props.element = Element::elec;
      props.flags |= Hit::stun;
      hitbox->SetHitboxProperties(props);
      field->AddEntity(*hitbox, col + i, row);
    }

    AUDIO.Play(AudioType::THUNDER);
    this->fired = true;
  };

  AddAnimAction(2, onFire);
}

LightningCardAction::~LightningCardAction()
{
}

void LightningCardAction::OnUpdate(float _elapsed)
{
  if (fired) {
    attackAnim.Update(_elapsed, attack->getSprite());
  }

  CardAction::OnUpdate(_elapsed);
}

void LightningCardAction::OnAnimationEnd()
{
  attachment->RemoveNode(attack);
  delete attack;
  delete attachment;
}

void LightningCardAction::EndAction()
{
  OnAnimationEnd();
  Eject();
}
