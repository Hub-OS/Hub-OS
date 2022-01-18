#ifdef BN_MOD_SUPPORT
#include "bnScriptedMob.h"
#include "bnScriptedCharacter.h"
#include "../bnFadeInState.h"
#include "../bnScriptResourceManager.h"
#include "../bnTextureResourceManager.h"
#include "../bnCustomBackground.h"
#include "../bnSolHelpers.h"

//
// class ScriptedMob::Spawner : public Mob::Spawner<ScriptedCharacter>
//
ScriptedMob::ScriptedSpawner::ScriptedSpawner(sol::state& script, const std::filesystem::path& path, Character::Rank rank)
{
  scriptedSpawner = std::make_unique<Mob::Spawner<ScriptedCharacter>>(rank);
  std::function<std::shared_ptr<ScriptedCharacter>()> lambda = scriptedSpawner->constructor;

  scriptedSpawner->constructor = [lambda, path, scriptPtr=&script] () -> std::shared_ptr<ScriptedCharacter> {
    auto& script = *scriptPtr;
    script["_modpath"] = path.generic_u8string() + "/";
    script["_folderpath"] = path.generic_u8string() + "/";

    auto character = lambda();
    character->InitFromScript(script);
    return character;
  };
}

std::shared_ptr<Mob::Mutator> ScriptedMob::ScriptedSpawner::SpawnAt(int x, int y)
{
  if (scriptedSpawner) {
    // todo: swap out with ScriptedIntroState
    return scriptedSpawner->SpawnAt<FadeInState>(x, y);
  }

  // ensure we're in range or return `nil` in Lua
  if (x < 1 || x > mob->GetField()->GetWidth() || y < 1 || y > mob->GetField()->GetHeight())
    return nullptr;

  // Create a new enemy spawn data object
  auto data = std::make_unique<Mob::SpawnData>();
  std::shared_ptr<Character> character = constructor();

  auto ui = character->CreateComponent<MobHealthUI>(character);
  ui->Hide(); // let default state invocation reveal health

  // Assign the enemy to the spawn data object
  data->character = character;
  data->tileX = x;
  data->tileY = y;
  data->index = (unsigned)mob->spawn.size();

  mob->GetField()->GetAt(x, y)->ReserveEntityByID(character->GetID());

  mob->pixelStateInvokers.push_back(this->pixelStateInvoker);

  auto next = this->defaultStateInvoker;
  auto defaultStateWrapper = [ui, next](std::shared_ptr<Character> in) {
    ui->Reveal();
    next(in);
  };

  mob->defaultStateInvokers.push_back(defaultStateWrapper);

  // Set name special font based on rank
  switch (data->character->GetRank()) {
  case Character::Rank::_2:
    data->character->SetName(data->character->GetName() + "2");
    break;
  case Character::Rank::_3:
    data->character->SetName(data->character->GetName() + "3");
    break;
  case Character::Rank::Rare1:
    data->character->SetName(data->character->GetName() + "R1");
    break;
  case Character::Rank::Rare2:
    data->character->SetName(data->character->GetName() + "R2");
    break;
  case Character::Rank::SP:
    data->character->SetName(data->character->GetName() + "\ue000");
    break;
  case Character::Rank::EX:
    data->character->SetName(data->character->GetName() + "\ue001");
    break;
  case Character::Rank::NM:
    data->character->SetName(data->character->GetName() + "\ue002");
    break;
  case Character::Rank::RV:
    data->character->SetName(data->character->GetName() + "\ue003");
    break;
  case Character::Rank::DS:
    data->character->SetName(data->character->GetName() + "\ue004");
    break;
  case Character::Rank::Alpha:
    data->character->SetName(data->character->GetName() + "α");
    break;
  case Character::Rank::Beta:
    data->character->SetName(data->character->GetName() + "β");
    break;
  case Character::Rank::Omega:
    data->character->SetName(data->character->GetName() + "Ω");
    break;
  }

  // Add the mob spawn data to our list of enemies to spawn
  mob->tracked.push_back(data->character);

  // Return a mutator to change some spawn info
  auto mutator = std::make_shared<Mob::Mutator>(std::move(data));
  mob->spawn.push_back(mutator);

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
ScriptedMob::ScriptedMob(sol::state& script) :
  MobFactory(),
  script(script)
{
}

Mob* ScriptedMob::Build(std::shared_ptr<Field> field, const std::string& dataString) {
  // Build a mob around the field input
  this->field = field;
  this->mob = new Mob(field);

  Logger::Logf(LogLevel::debug, "Data passed to package_build -> %s", dataString.c_str());

  auto dataResult = EvalLua(script, dataString);

  if (dataResult.is_error()) {
    Logger::Log(LogLevel::critical, dataResult.error_cstr());
  }

  auto initResult = CallLuaFunction(script, "package_build", this, dataResult.ok());

  if (initResult.is_error()) {
    Logger::Log(LogLevel::critical, initResult.error_cstr());
  }

  return mob;
}

std::shared_ptr<Field> ScriptedMob::GetField()
{
  return field;
}

void ScriptedMob::EnableFreedomMission(uint8_t turnCount, bool playerCanFlip)
{
  if (!mob) return;

  mob->EnableFreedomMission(true);
  mob->LimitTurnCount(turnCount);
  mob->EnablePlayerCanFlip(playerCanFlip);
}

ScriptedMob::ScriptedSpawner ScriptedMob::CreateSpawner(const std::string& namespaceId, const std::string& fqn, Character::Rank rank)
{
  ScriptPackage* package = Scripts().FetchScriptPackage(namespaceId, fqn, ScriptPackageType::character);

  if (!package) {
    throw std::runtime_error("Character does not exist");
  }

  auto obj = ScriptedMob::ScriptedSpawner(*package->state, package->path, rank);
  obj.SetMob(this->mob);
  return obj;
}

void ScriptedMob::SetBackground(const std::filesystem::path& bgTexturePath, const std::filesystem::path& animPath, float velx, float vely)
{
  auto texture = Textures().LoadFromFile(bgTexturePath);
  auto anim = Animation(animPath);
  auto vel = sf::Vector2f{ velx, vely };
  std::shared_ptr<Background> background = std::make_shared<CustomBackground>(texture, anim, vel);
  mob->SetBackground(background);
}

void ScriptedMob::StreamMusic(const std::filesystem::path& path, long long startMs, long long endMs)
{
  mob->StreamCustomMusic(path, startMs, endMs);
}
void ScriptedMob::SpawnPlayer(unsigned playerNum, int tileX, int tileY)
{
  mob->SpawnPlayer(playerNum, tileX, tileY);
}
#endif
