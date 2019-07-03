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

NinjaAntiDamage::NinjaAntiDamage(Entity* owner) : Component(owner) {
  // Construct a callback when anti damage is triggered
  DefenseAntiDamage::Callback onHit = [this](Spell* in, Character* owner) {
      
    // Get the aggressor of the attack, if any
    Entity* user = in->GetHitboxProperties().aggressor;
    Battle::Tile* tile = nullptr;

    if (user) {
      // If there's an aggressor, grab their tile and target it
      tile = user->GetTile();
      
      // Add a HideTimer for the owner of anti damage
      owner->RegisterComponent(new HideTimer(owner, 1.0));
      
      // Add a poof particle to denote owner dissapearing
      owner->GetField()->AddEntity(*new ParticlePoof(owner->GetField()), owner->GetTile()->GetX(), owner->GetTile()->GetY());
    }

    // If there's a tile
    if (tile) {
      // Add ninja star spell targetting the aggressor's tile
      owner->GetField()->AddEntity(*new NinjaStar(owner->GetField(), owner->GetTeam(), 0.2f), tile->GetX(), tile->GetY());
    }

    // Remove the anti damage rule from the owner
    owner->RemoveDefenseRule(defense);
    
    // Remove this component from the owner
    owner->FreeComponentByID(this->GetID());
    
    // Self cleanup
    delete this;
  }; // END callback 


  // Construct a anti damage defense rule check with callback onHit
  this->defense = new DefenseAntiDamage(onHit);
  
  // Add the defense rule to the owner of this component
  this->GetOwnerAs<Character>()->AddDefenseRule(defense);
}

NinjaAntiDamage::~NinjaAntiDamage() {
  // Delete the defense rule pointer we allocated
  delete defense;
}

void NinjaAntiDamage::OnUpdate(float _elapsed) {
  // Does nothing
}

void NinjaAntiDamage::Inject(BattleScene&) {
  // Does not effect battle scene
}