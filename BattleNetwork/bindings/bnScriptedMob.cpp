#ifdef BN_MOD_SUPPORT
#include "bnScriptedMob.h"
#include "bnScriptedCharacter.h"
#include "../bnFadeInState.h"
#include "../bnScriptResourceManager.h"
#include "../bnCustomBackground.h"

//
// class ScriptedMob::Spawner : public Mob::Spawner<ScriptedCharacter>
//
ScriptedMob::ScriptedSpawner::ScriptedSpawner(sol::state& script, const std::string& fqn) :
  Mob::Spawner <ScriptedCharacter>(std::ref(script)) 
{ 
  auto lambda = this->constructor;

  this->constructor = [lambda, fqn, scriptPtr=&script] () -> ScriptedCharacter* {
    (*scriptPtr)["_modpath"] = fqn;

    return lambda();
  };
}

ScriptedMob::ScriptedSpawner::~ScriptedSpawner()
{}

void ScriptedMob::ScriptedSpawner::SpawnAt(int x, int y)
{
  // todo: swap out with ScriptedIntroState
  Mob::Spawner <ScriptedCharacter>::SpawnAt<FadeInState>(x, y);
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
  script["build"](mob);
  return mob;
}

ScriptedMob::ScriptedSpawner ScriptedMob::CreateSpawner(const std::string& fqn)
{
  return ScriptedSpawner(*this->Scripts().FetchCharacter(fqn), fqn);
}
void ScriptedMob::SetBackground(const std::string& bgTexturePath, const std::string& animPath, float velx, float vely)
{
  auto texture = Texture().loadFromFile(bgTexturePath);
  std::shared_ptr<Background> background = std::make_shared<CustomBackground>(bgTexturePath, animPath, sf::Vector2f{ velx, vely });
  mob->SetBackground(background);
}

void ScriptedMob::StreamMusic(const std::string& path)
{
  mob->StreamCustomMusic(path);
}
#endif
