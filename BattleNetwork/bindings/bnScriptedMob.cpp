#ifdef BN_MOD_SUPPORT
#include "bnScriptedMob.h"
#include "bnScriptedCharacter.h"
#include "../bnFadeInState.h"
#include "../bnScriptResourceManager.h"
#include "../bnCustomBackground.h"

// Builtins
#include "../bnMettaur.h"
#include "../bnCanodumb.h"

//
// class ScriptedMob::Spawner : public Mob::Spawner<ScriptedCharacter>
//
ScriptedMob::ScriptedSpawner::ScriptedSpawner(sol::state& script, const std::string& path)
{ 
  scriptedSpawner = new Mob::Spawner <ScriptedCharacter>(std::ref(script));
  auto lambda = scriptedSpawner->constructor;

  scriptedSpawner->constructor = [lambda, path, scriptPtr=&script] () -> ScriptedCharacter* {
    (*scriptPtr)["_modpath"] = path+"/";

    return lambda();
  };
}

ScriptedMob::ScriptedSpawner::ScriptedSpawner(const std::function<Character*()>& new_constructor)
{
  this->builtInSpawner = new_constructor;
}

ScriptedMob::ScriptedSpawner::~ScriptedSpawner()
{}

void ScriptedMob::ScriptedSpawner::SpawnAt(int x, int y)
{
  // todo: swap out with ScriptedIntroState
 scriptedSpawner->SpawnAt<FadeInState>(x, y);
}

void ScriptedMob::ScriptedSpawner::SetMob(Mob* mob)
{
  scriptedSpawner->SetMob(mob);
}

//
// class ScriptedMob : public Mob
// 
ScriptedMob::ScriptedMob(Field* field, sol::state& script) : 
  MobFactory(field), 
  script(script)
{
}


ScriptedMob::~ScriptedMob()
{
}

Mob* ScriptedMob::Build() {
  // Build a mob around the field input
  this->mob = new Mob(field);
  script["build"](this);
  return mob;
}

Field* ScriptedMob::GetField()
{
  return this->field;
}

ScriptedMob::ScriptedSpawner ScriptedMob::CreateSpawner(const std::string& fqn)
{
  size_t builtin = fqn.find_first_of("BuiltIns.");

  if (builtin == std::string::npos) {
    auto obj = ScriptedSpawner(*Scripts().FetchCharacter(fqn), Scripts().CharacterToModpath(fqn));
    obj.SetMob(this->mob);
    return obj;
  }

  // else we are built in

  if (fqn == "BuiltIns.Canodumb") {
    auto makeCano = []() -> Character* {
      return new Canodumb();
    };
    auto obj = ScriptedSpawner(makeCano);
    obj.SetMob(this->mob);
    return obj;
  }

  // else, none of the above? TODO: throw?
  // for now spawn metts if nothing matches...
  
  // else if (fqn == "BuiltIns.Mettaur")
  auto makeMet = []() -> Character* {
    return new Mettaur();
  };
  auto obj = ScriptedSpawner(makeMet);
  obj.SetMob(this->mob);
  return obj;
}

void ScriptedMob::SetBackground(const std::string& bgTexturePath, const std::string& animPath, float velx, float vely)
{
  auto texture = Textures().LoadTextureFromFile(bgTexturePath);
  auto anim = Animation(animPath);
  auto vel = sf::Vector2f{ velx, vely };
  std::shared_ptr<Background> background = std::make_shared<CustomBackground>(texture, anim, vel);
  mob->SetBackground(background);
}

void ScriptedMob::StreamMusic(const std::string& path)
{
  mob->StreamCustomMusic(path);
}
#endif
