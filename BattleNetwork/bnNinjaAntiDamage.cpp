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

NinjaAntiDamage::NinjaAntiDamage(std::weak_ptr<Entity> owner) : Component(owner) {
  // Construct a callback when anti damage is triggered
  DefenseAntiDamage::Callback onHit = [this](auto in, auto owner) {
     
    class AntiDamageTriggerAction : public CardAction {
    private:
      Team aggroTeam;
      NinjaAntiDamage& component;

    public:
      AntiDamageTriggerAction(std::shared_ptr<Character> actor, Team aggroTeam, NinjaAntiDamage& component) :
        CardAction(actor, "PLAYER_IDLE"),
        aggroTeam(aggroTeam),
        component(component) {}

      ~AntiDamageTriggerAction() { }

      void Update(double elapsed) override {}
      void OnExecute(std::shared_ptr<Character> user) override {
        auto owner = GetActor();

        Battle::Tile* tile = nullptr;
        Field& field = *user->GetField();

        // Add a HideTimer for the owner of anti damage
        user->CreateComponent<HideTimer>(user, 1.0);

        // Add a poof particle to denote owner dissapearing
        field.AddEntity(std::make_shared<ParticlePoof>(), *user->GetTile());

        // Find the nearest character on the other team to take a hit
        auto nearest = field.FindNearestCharacters(user, [user](std::shared_ptr<Entity> in) {
          return !user->Teammate(in->GetTeam()); // get the opposition
        });

        // If there's an aggressor, grab their tile and target it
        if (nearest.size()) {
          // Add ninja star spell targetting the tile
          field.AddEntity(std::make_shared<NinjaStar>(user->GetTeam(), 0.2f), *nearest[0]->GetTile());
        }

        // Remove the anti damage rule from the owner
        user->RemoveDefenseRule(component.defense);

        component.Eject();
      }
      void OnAnimationEnd() override {}
      void OnActionEnd() override {};
    };

    Team oppositeTeam = Team::blue;

    if (owner->GetTeam() == Team::blue) {
      oppositeTeam = Team::red;
    }

    auto action = std::make_shared<AntiDamageTriggerAction>(owner, oppositeTeam, *this);
    owner->AddAction({action}, ActionOrder::traps);
  }; // END callback 


  // Construct a anti damage defense rule check with callback onHit
  defense = std::make_shared<DefenseAntiDamage>(onHit);

  if (auto ownerAsCharacter = GetOwnerAs<Character>()) {
    ownerAsCharacter->AddDefenseRule(defense);
  }
}

NinjaAntiDamage::~NinjaAntiDamage() {
  auto owner = GetOwnerAs<Character>();
  
  if (owner) {
    owner->RemoveDefenseRule(defense);
  }
}

void NinjaAntiDamage::OnUpdate(double _elapsed) {
}

void NinjaAntiDamage::Inject(BattleSceneBase&) {
  // Does not effect battle scene
}