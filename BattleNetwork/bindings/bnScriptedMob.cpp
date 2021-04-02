#ifdef BN_MOD_SUPPORT
#include "bnScriptedMob.h"
#include "bnScriptedCharacter.h"
#include "../bnFadeInState.h"
#include "../bnScriptResourceManager.h"
#include "../bnCustomBackground.h"

//
// class ScriptedMob::Spawner : public Mob::Spawner<ScriptedCharacter>
//
ScriptedMob::ScriptedSpawner::ScriptedSpawner(sol::state& script, const std::string& path) :
  Mob::Spawner <ScriptedCharacter>(std::ref(script)) 
{ 
  auto lambda = this->constructor;

  this->constructor = [lambda, path, scriptPtr=&script] () -> ScriptedCharacter* {
    (*scriptPtr)["_modpath"] = path+"/";

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
  script["build"](this);
  return mob;
}

Field* ScriptedMob::GetField()
{
  return this->field;
}

ScriptedMob::ScriptedSpawner ScriptedMob::CreateSpawner(const std::string& fqn)
{
  auto obj = ScriptedSpawner(*Scripts().FetchCharacter(fqn), Scripts().CharacterToModpath(fqn));
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
