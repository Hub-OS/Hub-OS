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
    this->damage = damage;

    overlay.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_ELEC_PULSE));

    this->attachment = new SpriteProxyNode(overlay);
    this->attachment->SetLayer(-1);
    attachmentAnim.Reload();
    attachmentAnim.SetAnimation("BUSTER");

    // add override anims
    this->OverrideAnimationFrames({ FRAMES });

    elecpulse = nullptr;
}

ElecPulseCardAction::~ElecPulseCardAction()
{
}

void ElecPulseCardAction::Execute() {
    auto owner = GetOwner();

    owner->AddNode(this->attachment);
    this->attachment->EnableParentShader(false);
    attachmentAnim.Update(0, this->attachment->getSprite());

    // On shoot frame, drop projectile
    auto onFire = [this, owner]() -> void {
        elecpulse = new Elecpulse(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
        AUDIO.Play(AudioType::ELECPULSE);

        auto props = elecpulse->GetHitboxProperties();
        props.aggressor = GetOwnerAs<Character>();
        elecpulse->SetHitboxProperties(props);

        Entity::DeleteCallback& deleteHandler = elecpulse->CreateDeleteCallback();
        
        deleteHandler.Slot([this]() {
            Logger::Log("elecpulse OnDelete() triggered.");
            this->elecpulse = nullptr;
        });

        GetOwner()->GetField()->AddEntity(*elecpulse, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
    };


    this->AddAction(2, onFire);
}

void ElecPulseCardAction::OnUpdate(float _elapsed)
{
    attachmentAnim.Update(_elapsed, this->attachment->getSprite());
    CardAction::OnUpdate(_elapsed);
}

void ElecPulseCardAction::EndAction()
{
    elecpulse? elecpulse->Delete() : 0;

    this->GetOwner()->RemoveNode(attachment);
    GetOwner()->FreeComponentByID(this->GetID());
    delete this;
}
