#include "bnRecoverCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleHeal.h"

RecoverCardAction::RecoverCardAction(std::shared_ptr<Character> actor, int heal) : 
  CardAction(actor, "PLAYER_IDLE") {
  RecoverCardAction::heal = heal;

  this->SetLockout({ CardAction::LockoutType::sequence });
}

RecoverCardAction::~RecoverCardAction()
{
}

void RecoverCardAction::OnExecute(std::shared_ptr<Character> user) {
  // Increase player health
  user->SetHealth(user->GetHealth() + heal);

  // Play sound
  Audio().Play(AudioType::RECOVER);

  // Add artifact on the same layer as player
  auto healfx = std::make_shared<ParticleHeal>();
  user->GetField()->AddEntity(healfx, *user->GetTile());

  CardAction::Step step;
  step.updateFunc = [=](Step& self, double elapsed) {
    if (healfx == nullptr || healfx->WillRemoveLater()) {
      self.markDone();
    }
  };

  AddStep(step);
}

void RecoverCardAction::OnAnimationEnd()
{
}

void RecoverCardAction::OnActionEnd()
{
}
