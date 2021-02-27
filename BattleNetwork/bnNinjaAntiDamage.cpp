#include "bnNinjaAntiDamage.h"
#include "bnDefenseAntiDamage.h"
#include "bnNinjaStar.h"
#include "bnCardAction.h"
#include "bnHideTimer.h"
#include "bnAudioResourceManager.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnCharacter.h"
#include "bnParticlePoof.h"
#include "bnSpell.h"

NinjaAntiDamage::NinjaAntiDamage(Entity* owner) : Component(owner) {
  // Construct a callback when anti damage is triggered
  DefenseAntiDamage::Callback onHit = [this](Spell& in, Character& owner) {
     
    class AntiDamageTriggerAction : public CardAction {
    private:
      Character* aggressor{ nullptr };
      NinjaAntiDamage& anti;

    public:
      AntiDamageTriggerAction(Character& owner, Character* aggressor, NinjaAntiDamage& anti) :
        CardAction(owner, "PLAYER_IDLE"),
        aggressor(aggressor),
        anti(anti) {}

      ~AntiDamageTriggerAction() { }

      void Update(double elapsed) override {}
      void OnExecute() override {
        auto& owner = GetCharacter();
        Battle::Tile* tile = nullptr;

        // Add a HideTimer for the owner of anti damage
        owner.RegisterComponent(new HideTimer(&owner, 1.0));

        // Add a poof particle to denote owner dissapearing
        owner.GetField()->AddEntity(*new ParticlePoof(), *owner.GetTile());

        if (aggressor) {
          // If there's an aggressor, grab their tile and target it
          if (auto tile = aggressor->GetTile()) {
            // Add ninja star spell targetting the aggressor's tile
            owner.GetField()->AddEntity(*new NinjaStar(owner.GetTeam(), 0.2f), *tile);
          }
        }

        // Remove the anti damage rule from the owner
        owner.RemoveDefenseRule(anti.defense);
        anti.Eject();
      }
      void OnAnimationEnd() override {}
      void OnEndAction() override {};
    };

    owner.QueueAction({
      ActionPriority::trigger,
      CurrentTime::AsMilli(),
      new AntiDamageTriggerAction(owner, in.GetHitboxProperties().aggressor, *this)
    });

    this->Eject();
  }; // END callback 


  // Construct a anti damage defense rule check with callback onHit
  defense = new DefenseAntiDamage(onHit);
  GetOwnerAs<Character>()->AddDefenseRule(defense);
}

NinjaAntiDamage::~NinjaAntiDamage() {
  // Delete the defense rule pointer we allocated
  delete defense;
}

void NinjaAntiDamage::OnUpdate(double _elapsed) {
}

void NinjaAntiDamage::Inject(BattleSceneBase&) {
  // Does not effect battle scene
}