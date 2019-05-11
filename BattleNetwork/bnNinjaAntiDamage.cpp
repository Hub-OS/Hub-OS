#include "bnNinjaAntiDamage.h"
#include "bnDefenseAntiDamage.h"
#include "bnNinjaStar.h"
#include "bnHideTimer.h"
#include "bnAudioResourceManager.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnCharacter.h"
#include "bnParticlePoof.h"
#include "bnSpell.h"

// Take out
#include "bnPlayer.h

NinjaAntiDamage::NinjaAntiDamage(Entity* owner) : Component(owner) {
  DefenseAntiDamage::Callback onHit = [this](Spell* in, Character* owner) {
    Entity* user = in->GetHitboxProperties().aggressor;
    Battle::Tile* tile = nullptr;

    if (user) {
      tile = user->GetTile();

      // TODO: owner->GetComponent<Animation>()->SetState("IDLE");
      Player* p = GetOwnerAs<Player>();

      if(p) {
          p->SetAnimation("PLAYER_IDLE");
      }
      owner->RegisterComponent(new HideTimer(owner, 1.0));
      owner->GetField()->AddEntity(*new ParticlePoof(owner->GetField()), owner->GetTile()->GetX(), owner->GetTile()->GetY());
    }

    if (tile) {
      owner->GetField()->AddEntity(*new NinjaStar(owner->GetField(), owner->GetTeam(), 0.2f), tile->GetX(), tile->GetY());
    }

    owner->RemoveDefenseRule(this->defense);
    owner->FreeComponentByID(this->GetID());
    delete this;
  };

  this->defense = new DefenseAntiDamage(onHit);
  this->GetOwnerAs<Character>()->AddDefenseRule(defense);
}

NinjaAntiDamage::~NinjaAntiDamage() {
  delete defense;
}

void NinjaAntiDamage::Update(float _elapsed) {

}

void NinjaAntiDamage::Inject(BattleScene&) {
  // Does not effect battle scene
}