#ifdef BN_MOD_SUPPORT
#include "bnUserTypeDefenseRule.h"

#include "bnScopedWrapper.h"
#include "bnScriptedDefenseRule.h"
#include "../bnDefenseVirusBody.h"
#include "../bnDefenseNodrag.h"
#include "../bnSolHelpers.h"

void DefineDefenseRuleUserTypes(sol::state& state, sol::table& battle_namespace) {
  // using shared_ptr as it can be added + removed from entities, ownership is never given to the field
  battle_namespace.new_usertype<ScriptedDefenseRule>("DefenseRule",
    sol::factories(
        [](int priority, const DefenseOrder& order) -> std::shared_ptr<ScriptedDefenseRule>
        { return std::make_shared<ScriptedDefenseRule>(Priority(priority), order); }
    ),
    "is_replaced", &ScriptedDefenseRule::IsReplaced,
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    sol::base_classes, sol::bases<DefenseRule>(),
    "filter_statuses_func", sol::property(
      [](ScriptedDefenseRule& defenseRule) { return defenseRule.filter_statuses_func; },
      [](ScriptedDefenseRule& defenseRule, sol::stack_object value) {
        defenseRule.filter_statuses_func = VerifyLuaCallback(value);
      }
    ),
    "can_block_func", sol::property(
      [](ScriptedDefenseRule& defenseRule) { return defenseRule.can_block_func; },
      [](ScriptedDefenseRule& defenseRule, sol::stack_object value) {
        defenseRule.can_block_func = VerifyLuaCallback(value);
      }
    )
  );

  battle_namespace.new_usertype<DefenseNodrag>("DefenseNoDrag",
    sol::factories(
      [] () -> std::shared_ptr<DefenseRule> { return std::make_shared<DefenseNodrag>(); }
    ),
    sol::base_classes, sol::bases<DefenseRule>()
  );

  battle_namespace.new_usertype<DefenseVirusBody>("DefenseVirusBody",
    sol::factories(
      [] () -> std::shared_ptr<DefenseRule> { return std::make_shared<DefenseVirusBody>(); }
    )
  );

  battle_namespace.new_usertype<ScopedWrapper<DefenseFrameStateJudge>>("DefenseFrameStateJudge",
    "block_damage", [](ScopedWrapper<DefenseFrameStateJudge>& judge) {
      judge.Unwrap().BlockDamage();
    },
    "block_impact", [](ScopedWrapper<DefenseFrameStateJudge>& judge) {
      judge.Unwrap().BlockImpact();
    },
    "is_damage_blocked", [](ScopedWrapper<DefenseFrameStateJudge>& judge) -> bool {
      return judge.Unwrap().IsDamageBlocked();
    },
    "is_impact_blocked", [](ScopedWrapper<DefenseFrameStateJudge>& judge) -> bool {
      return judge.Unwrap().IsImpactBlocked();
    },
    /*"add_trigger", &DefenseFrameStateJudge::AddTrigger,*/
    "signal_defense_was_pierced", [](ScopedWrapper<DefenseFrameStateJudge>& judge) {
      judge.Unwrap().SignalDefenseWasPierced();
    }
  );

  state.new_enum("DefenseOrder",
    "Always", DefenseOrder::always,
    "CollisionOnly", DefenseOrder::collisionOnly
  );
}
#endif
