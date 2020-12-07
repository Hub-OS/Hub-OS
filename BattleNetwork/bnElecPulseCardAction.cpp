#include "bnElecPulseCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/elecpulse.png"
#define ANIM "resources/spells/elecpulse.animation"

#define WAIT   { 1, 0.25 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }

#define FRAMES  WAIT,  FRAME2,  FRAME1, FRAME2, FRAME1, FRAME2, \
                FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, \
                FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                FRAME1, FRAME2, FRAME1, FRAME2

ElecPulseCardAction::ElecPulseCardAction(Character * owner, int damage) : 
  CardAction(*owner, "PLAYER_SHOOTING"), 
  attachmentAnim(ANIM) {
    ElecPulseCardAction::damage = damage;

    attachment = new SpriteProxyNode();
    attachment->setTexture(Textures().GetTexture(TextureType::SPELL_ELEC_PULSE));
    attachment->SetLayer(-1);

    attachmentAnim = Animation(ANIM);
    attachmentAnim.SetAnimation("BUSTER");

    AddAttachment(anim->GetAnimationObject(), "buster", *attachment).PrepareAnimation(attachmentAnim);

    // add override anims
    OverrideAnimationFrames({ FRAMES });

    elecpulse = nullptr;

    AddAttachment(*owner, "buster", *attachment).UseAnimation(attachmentAnim);
}

ElecPulseCardAction::~ElecPulseCardAction()
{
}

void ElecPulseCardAction::OnExecute() {
    // On shoot frame, drop projectile
    auto onFire = [this]() -> void {
      auto& user = GetUser();

    attachment->EnableParentShader(false);

    // On shoot frame, drop projectile`
    auto onFire = [this, owner]() -> void {
      Team team = GetOwner()->GetTeam();
      elecpulse = new Elecpulse(GetOwner()->GetField(), team, damage);
      Audio().Play(AudioType::ELECPULSE);

      auto props = elecpulse->GetHitboxProperties();
      props.aggressor = &user;
      elecpulse->SetHitboxProperties(props);

      auto status = user.GetField()->AddEntity(*elecpulse, user.GetTile()->GetX() + 1, user.GetTile()->GetY());

      if (status != Field::AddEntityStatus::deleted) {
        Entity::RemoveCallback& deleteHandler = elecpulse->CreateRemoveCallback();
        deleteHandler.Slot([this]() {
          EndAction();
        });
        int step = 1;
        if (team != Team::red) {
          step = -1;
        }

        GetOwner()->GetField()->AddEntity(*elecpulse, GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());

    };


    AddAnimAction(2, onFire);
}

void ElecPulseCardAction::OnEndAction()
{
    CardAction::OnUpdate(_elapsed);
}

void ElecPulseCardAction::OnAnimationEnd()
{
}

void ElecPulseCardAction::EndAction()
{
  Eject();
}
