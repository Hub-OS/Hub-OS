#ifdef BN_MOD_SUPPORT
#include "bnUserTypeEntity.h"

void DefineEntityUserType(sol::state& state, sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<Entity>>("Entity");
  DefineEntityFunctionsOn(table);

  state.new_enum("Shadow",
    "None", Entity::Shadow::none,
    "Small", Entity::Shadow::small,
    "Big", Entity::Shadow::big
  );

  state.new_enum("Team",
    "Red", Team::red,
    "Blue", Team::blue,
    "Other", Team::unknown
  );

  state.new_enum("ActionOrder",
    "Involuntary", ActionOrder::involuntary,
    "Voluntary", ActionOrder::voluntary,
    "Immediate", ActionOrder::immediate
  );
}
#endif
