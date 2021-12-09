#ifdef BN_MOD_SUPPORT
#include "bnUserTypeEntity.h"

void DefineEntityUserType(sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<Entity>>("Entity");
  DefineEntityFunctionsOn(table);
}
#endif
