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

ScriptedMob::ScriptedSpawner::~ScriptedSpawner()
{}

Mob::Mutator* ScriptedMob::ScriptedSpawner::SpawnAt(int x, int y)
{
  if (scriptedSpawner) {
    // todo: swap out with ScriptedIntroState
    return scriptedSpawner->SpawnAt<FadeInState>(x, y);
  }

  // ensure we're in range or return `nil` in Lua
  if (x < 1 || x > mob->GetField()->GetWidth() || y < 1 || y > mob->GetField()->GetHeight())
    return nullptr;

  // Create a new enemy spawn data object
  Mob::MobData* data = new Mob::MobData();
  Character* character = constructor();

  auto ui = character->CreateComponent<MobHealthUI>(character);
  ui->Hide(); // let default state invocation reveal health

  // Assign the enemy to the spawn data object
  data->character = character;
  data->tileX = x;
  data->tileY = y;
  data->index = (unsigned)mob->spawn.size();

  mob->GetField()->GetAt(x, y)->ReserveEntityByID(character->GetID());

  auto mutator = new Mob::Mutator(data);

  mob->pixelStateInvokers.push_back(this->pixelStateInvoker);

  auto next = this->defaultStateInvoker;
  auto defaultStateWrapper = [ui, next](Character* in) {
    ui->Reveal();
    next(in);
  };

  mob->defaultStateInvokers.push_back(defaultStateWrapper);

  // Add the mob spawn data to our list of enemies to spawn
  mob->spawn.push_back(mutator);
  mob->tracked.push_back(data->character);

  // Return a mutator to change some spawn info
  return mutator;
}

void ScriptedMob::ScriptedSpawner::SetMob(Mob* mob)
{
  this->mob = mob;

  if (scriptedSpawner) {
    scriptedSpawner->SetMob(mob);
  }
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
  std::string_view prefix = "BuiltIns.";
  size_t builtin = fqn.find(prefix);

  if (builtin == std::string::npos) {
    auto obj = ScriptedMob::ScriptedSpawner(*Scripts().FetchCharacter(fqn), Scripts().CharacterToModpath(fqn));
    obj.SetMob(this->mob);
    return obj;
  }

  std::string name = fqn.substr(builtin+prefix.size());

  //
  // else we are built in
  //

  auto obj = ScriptedMob::ScriptedSpawner();
  obj.SetMob(this->mob);

  if (name == "Canodumb") {
    obj.UseBuiltInType<Canodumb>();
  }
  else /* if (name == "BuiltIns.Mettaur") */ {
    // else, none of the above? TODO: throw?
    // for now spawn metts if nothing matches...
    obj.UseBuiltInType<Mettaur>();
  }

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

void ScriptedMob::StreamMusic(const std::string& path, long long startMs, long long endMs)
{
  mob->StreamCustomMusic(path, startMs, endMs);
}
#endif
