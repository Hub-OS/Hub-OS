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

#define FRAMES WAIT, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1 \
               , FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2

ElecPulseCardAction::ElecPulseCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
    ElecPulseCardAction::damage = damage;

    overlay.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_ELEC_PULSE));

    attachment = new SpriteProxyNode(overlay);
    attachment->SetLayer(-1);
    attachmentAnim.Reload();
    attachmentAnim.SetAnimation("BUSTER");

    // add override anims
    OverrideAnimationFrames({ FRAMES });

    elecpulse = nullptr;
}

ElecPulseCardAction::~ElecPulseCardAction()
{
}

void ElecPulseCardAction::Execute() {
    auto owner = GetOwner();

    owner->AddNode(attachment);
    attachment->EnableParentShader(false);
    attachmentAnim.Update(0, attachment->getSprite());

    // On shoot frame, drop projectile`
    auto onFire = [this, owner]() -> void {
        Team team = GetOwner()->GetTeam();
        elecpulse = new Elecpulse(GetOwner()->GetField(), team, damage);
        //AUDIO.Play(AudioType::ELECPULSE);

        auto props = elecpulse->GetHitboxProperties();
        props.aggressor = GetOwnerAs<Character>();
        elecpulse->SetHitboxProperties(props);

        Entity::RemoveCallback& deleteHandler = elecpulse->CreateRemoveCallback();

        deleteHandler.Slot([this]() {
            Logger::Log("elecpulse OnDelete() triggered.");
            elecpulse = nullptr;
        });

        int step = 1;
        if (team != Team::red) {
          step = -1;
        }

        GetOwner()->GetField()->AddEntity(*elecpulse, GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());
    };


    AddAction(2, onFire);
}

void ElecPulseCardAction::OnUpdate(float _elapsed)
{
    attachmentAnim.Update(_elapsed, attachment->getSprite());
    CardAction::OnUpdate(_elapsed);
}

void ElecPulseCardAction::EndAction()
{
    elecpulse? elecpulse->Delete() : (void(0));

    GetOwner()->RemoveNode(attachment);
    GetOwner()->FreeComponentByID(GetID());
    delete this;
}
