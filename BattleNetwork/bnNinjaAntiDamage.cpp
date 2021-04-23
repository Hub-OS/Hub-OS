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
      Entity::ID_t aggroId;
      NinjaAntiDamage& component;

    public:
      AntiDamageTriggerAction(Character& actor, Entity::ID_t aggroId, NinjaAntiDamage& component) :
        CardAction(actor, "PLAYER_IDLE"),
        aggroId(aggroId),
        component(component) {}

      ~AntiDamageTriggerAction() { }

      void Update(double elapsed) override {}
      void OnExecute(Character* user) override {
        auto& owner = GetActor();
        auto* aggressor = owner.GetField()->GetCharacter(aggroId);

        Battle::Tile* tile = nullptr;

        // Add a HideTimer for the owner of anti damage
        user->CreateComponent<HideTimer>(user, 1.0);

        // Add a poof particle to denote owner dissapearing
        user->GetField()->AddEntity(*new ParticlePoof(), *user->GetTile());

        // If there's an aggressor, grab their tile and target it
        if (aggressor) {
          if (auto tile = aggressor->GetTile()) {
            // Add ninja star spell targetting the aggressor's tile
            user->GetField()->AddEntity(*new NinjaStar(user->GetTeam(), 0.2f), *tile);
          }
        }

        // Remove the anti damage rule from the owner
        user->RemoveDefenseRule(component.defense);
        component.Eject();
      }
      void OnAnimationEnd() override {}
      void OnActionEnd() override {};
    };

    auto* action = new AntiDamageTriggerAction(owner, in.GetHitboxProperties().aggressor, *this);
    owner.AddAction({ action }, ActionOrder::traps);
  }; // END callback 


  // Construct a anti damage defense rule check with callback onHit
  defense = new DefenseAntiDamage(onHit);
  GetOwnerAs<Character>()->AddDefenseRule(defense);
}

NinjaAntiDamage::~NinjaAntiDamage() {
  delete defense;
}

void NinjaAntiDamage::OnUpdate(double _elapsed) {
}

void NinjaAntiDamage::Inject(BattleSceneBase&) {
  // Does not effect battle scene
}