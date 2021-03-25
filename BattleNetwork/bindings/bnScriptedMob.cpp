#ifdef BN_MOD_SUPPORT
#include "bnScriptedMob.h"
#include "bnScriptedCharacter.h"
#include "../bnFadeInState.h"

ScriptedMob::ScriptedMob(Field* field, sol::state& script) : 
  MobFactory(field), 
  script(script)
{
}


ScriptedMob::~ScriptedMob()
{
}

Mob* ScriptedMob::Build() {
  // Mav note: this will all probably be handled by a script...
  
  // Build a mob around the field input
  Mob* mob = new Mob(field);

  for (int i = 0; i <= 3; i++) {
    field->GetAt(5, i)->SetState(TileState::hidden);
  }

  for (int i = 0; i <= 3; i++) {
    field->GetAt(6, i)->SetState(TileState::hidden);
  }

  auto spawner = mob->CreateSpawner<ScriptedCharacter>(std::ref(script));
  spawner.SpawnAt<FadeInState>(5, 2);

  return mob;
}
#endif