#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedComponent.h"

#include "bnWeakWrapper.h"
#include "bnScriptedComponent.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"

static WeakWrapper<ScriptedComponent> construct(std::shared_ptr<Character> character, Component::lifetimes lifetime) {
  auto component = std::make_shared<ScriptedComponent>(character, lifetime);
  component->Init();

  auto wrappedComponent = WeakWrapper(component);
  wrappedComponent.Own();
  return wrappedComponent;
}

void DefineScriptedComponentUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<ScriptedComponent>>("Component",
    sol::factories([](WeakWrapper<Character> owner, Component::lifetimes lifetime) -> WeakWrapper<ScriptedComponent> {
      return construct(owner.Unwrap(), lifetime);
    }),
    sol::factories([](WeakWrapper<ScriptedCharacter> owner, Component::lifetimes lifetime) -> WeakWrapper<ScriptedComponent> {
      return construct(owner.Unwrap(), lifetime);
    }),
    sol::factories([](WeakWrapper<ScriptedPlayer> owner, Component::lifetimes lifetime) -> WeakWrapper<ScriptedComponent> {
      return construct(owner.Unwrap(), lifetime);
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedComponent>& component, std::string key) {
      return component.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedComponent>& component, std::string key, sol::stack_object value) {
      component.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedComponent>& component) {
      return component.Unwrap()->entries.size();
    },
    "eject", [](WeakWrapper<ScriptedComponent>& component) {
      component.Unwrap()->Eject();
    },
    "get_id", [](WeakWrapper<ScriptedComponent>& component) -> Component::ID_t {
      return component.Unwrap()->GetID();
    },
    "is_injected", [](WeakWrapper<ScriptedComponent>& component) -> bool{
      return component.Unwrap()->Injected();
    },
    "get_owner", [](WeakWrapper<ScriptedComponent>& component) -> WeakWrapper<Character> {
      return WeakWrapper(component.Unwrap()->GetOwnerAsCharacter());
    }
  );
}
#endif
