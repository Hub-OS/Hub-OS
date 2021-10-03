#ifdef BN_MOD_SUPPORT
#include <memory>
#include <vector>
#include <functional>
#include "bnScriptResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
#include "bnCardPackageManager.h"
#include "bnPlayerPackageManager.h"
#include "bnMobPackageManager.h"

#include "bindings/bnScriptEnvironmentManager.h"

namespace {
  int exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    if (maybe_exception) {
      const std::exception& ex = *maybe_exception;
      Logger::Log(ex.what());
    }
    else {
      std::string message(description.data(), description.size());
      Logger::Log(message.c_str());
    }

    return sol::stack::push(L, description);
  }
}

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {

  sol::table battle_namespace =     state.get<sol::table>("Battle");
  sol::table overworld_namespace =  state.get<sol::table>("Overworld");
  sol::table engine_namespace =     state.get<sol::table>("Engine");

  engine_namespace.set_function("get_rand_seed", [this]() -> unsigned int { return randSeed; });

  engine_namespace.set_function("define_character",
    [this](const std::string& fqn, const std::string& path) {
        this->DefineCharacter(fqn, path);
    }
  );

  engine_namespace.set_function("requires_character",
    [this](const std::string& fqn)
    {
      // Handle built-ins...
      auto builtins = { "com.builtins.char.canodumb", "com.builtins.char.mettaur" };
      for (auto&& match : builtins) {
          if (fqn == match) return;
      }

      if (this->FetchCharacter(fqn) == nullptr) {
          std::string msg = "Failed to Require character with FQN " + fqn;
          Logger::Log(msg);
          throw std::runtime_error(msg);
      }
    }
  );

  engine_namespace.set_function("action_from_card",
    [this](const std::string& fqn)
    {

    }
  );

  engine_namespace.set_function("load_texture",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Textures().LoadTextureFromFile(path);
    }
  );

  engine_namespace.set_function("load_shader",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Shaders().LoadShaderFromFile(path);
    }
  );

  engine_namespace.set_function("load_audio",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Audio().LoadFromFile(path);
    }
  );

  engine_namespace.set_function("play_audio",
    sol::factories(
      [](std::shared_ptr<sf::SoundBuffer> buffer, AudioPriority priority) {
      static ResourceHandle handle;
      return handle.Audio().Play(buffer, priority);
    },
    [](AudioType type, AudioPriority priority) {
      static ResourceHandle handle;
      return handle.Audio().Play(type, priority);
    })
  );

  engine_namespace.set_function("input_has",
    [](const InputEvent& event) {
      static InputHandle handle;
      return handle.Input().Has(event);
    }
  );
}

ScriptResourceManager& ScriptResourceManager::GetInstance()
{
  static ScriptResourceManager instance;
  return instance;
}

ScriptResourceManager::~ScriptResourceManager()
{
  scriptTableHash.clear();
  for (auto ptr : states) {
    delete ptr;
  }
  states.clear();
}

ScriptResourceManager::LoadScriptResult& ScriptResourceManager::LoadScript(const std::filesystem::path& modDirectory)
{
  auto entryPath = modDirectory / "entry.lua";
  auto modpath = modDirectory;

  auto iter = scriptTableHash.find(entryPath.filename().generic_string());

  if (iter != scriptTableHash.end()) {
    return iter->second;
  }

  sol::state* lua = new sol::state;
  
  ScriptEnvironmentManager::SetModPathVariable( lua, modDirectory );
  ScriptEnvironmentManager::SetSystemFunctions( lua );
  ScriptEnvironmentManager::ConfigureEnvironment( *lua );
  ConfigureEnvironment(*lua);

  lua->set_exception_handler(&::exception_handler);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(entryPath, sol::script_pass_on_error);
  auto pair = scriptTableHash.emplace(entryPath, LoadScriptResult{std::move(load_result), lua} );
  return pair.first->second;
}

void ScriptResourceManager::DefineCharacter(const std::string& fqn, const std::string& path)
{
  auto iter = characterFQN.find(fqn);

  if (iter == characterFQN.end()) {
    auto& res = LoadScript(path);

    if (res.result.valid()) {
      characterFQN[fqn] = path;
    }
    else {
      sol::error error = res.result;
      Logger::Logf("Failed to Require character with FQN %s. Reason: %s", fqn.c_str(), error.what());

      // bubble up to end this scene from loading that needs this character...
      throw std::exception(error);
    }
  }
}

sol::state* ScriptResourceManager::FetchCharacter(const std::string& fqn)
{
  auto iter = characterFQN.find(fqn);

  if (iter != characterFQN.end()) {
    return LoadScript(iter->second).state;
  }

  // else miss
  return nullptr;
}

const std::string& ScriptResourceManager::CharacterToModpath(const std::string& fqn) {
  return characterFQN[fqn];
}
void ScriptResourceManager::SeedRand(unsigned int seed)
{
  randSeed = seed;
}
#endif