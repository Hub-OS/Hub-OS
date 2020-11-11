#include "bnMachGunCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSuperVulcan.h"

#define WAIT   { 1, 0.0166 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.033 }

#define FRAMES WAIT, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                      FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, 

MachGunCardAction::MachGunCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_SHOOTING"),
  damage(damage)
{
  machgun.setTexture(TEXTURES.LoadTextureFromFile("resources/spells/machgun_buster.png"));
  machgunAnim = Animation("resources/spells/machgun_buster.animation") << "FIRE";

  AddAttachment(owner, "BUSTER", machgun).UseAnimation(machgunAnim);

  OverrideAnimationFrames({ FRAMES });
}

MachGunCardAction::~MachGunCardAction()
{
}

void MachGunCardAction::Execute()
{
  auto shoot = [this]() {
    AUDIO.Play(AudioType::GUN);
  };

  AddAnimAction(6, shoot);
  AddAnimAction(10, shoot);
  AddAnimAction(15, shoot);
}

void MachGunCardAction::EndAction()
{
  Eject();
}

void MachGunCardAction::OnAnimationEnd()
{
}
