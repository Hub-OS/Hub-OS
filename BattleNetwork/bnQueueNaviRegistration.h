/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will be replaced by a script parser to load extra content without 
 * needing to recompile... */

#pragma once

#include <filesystem>

// Register these navis
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"
#include "bnMegaman.h"
#include "bnStarman.h"
#include "bnProtoman.h"
#include "bnTomahawkman.h"
#include "bnRoll.h"
#include "bnForte.h"
#include "bnScriptedPlayer.h"

static inline void QueuNaviRegistration() {
  ResourceHandle handle;

  /*********************************************************************
  **********            Register megaman            ********************
  **********************************************************************/
  auto megamanInfo = NAVIS.AddClass<Megaman>();  // Create and register navi info object
  megamanInfo->SetSpecialDescription("Well rounded stats"); // Set property
  megamanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/megaman/mega_face.png"));
  megamanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/megaman/preview.png"));
  megamanInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/megaman/overworld.png"));
  megamanInfo->SetOverworldAnimationPath("resources/navis/megaman/overworld.animation");
  megamanInfo->SetSpeed(1);
  megamanInfo->SetAttack(1);
  megamanInfo->SetChargedAttack(10);

  // Register Roll
  auto rollInfo = NAVIS.AddClass<Roll>();
  rollInfo->SetSpecialDescription("High HP w/ FloatShoe enabled.");
  // rollInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/roll/roll_face.png"));
  rollInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/roll/preview.png"));
  rollInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/roll/overworld.png"));
  rollInfo->SetOverworldAnimationPath("resources/navis/roll/overworld.animation");
  rollInfo->SetSpeed(2);
  rollInfo->SetAttack(2);
  rollInfo->SetChargedAttack(10);

  // Register Starman
  auto starmanInfo = NAVIS.AddClass<Starman>();
  starmanInfo->SetSpecialDescription("Fastest navi w/ rapid fire");
  // starmanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/starman/starman_face.png"));
  starmanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/starman/preview.png"));
  starmanInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/starman/starman_OW.png"));
  starmanInfo->SetOverworldAnimationPath("resources/navis/starman/starman_OW.animation");
  starmanInfo->SetSpeed(3);
  starmanInfo->SetAttack(2);
  starmanInfo->SetChargedAttack(10);

  // Register Protoman
  auto protomanInfo = NAVIS.AddClass<Protoman>();
  protomanInfo->SetSpecialDescription("Elite class navi w/ sword");
  protomanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/protoman/protoman_face.png"));
  protomanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/protoman/preview.png"));
  protomanInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/protoman/protoman_OW.png"));
  protomanInfo->SetOverworldAnimationPath("resources/navis/protoman/protoman_OW.animation");
  protomanInfo->SetSpeed(3);
  protomanInfo->SetAttack(1);
  protomanInfo->SetChargedAttack(20);
  protomanInfo->SetIsSword(true);

  // Register Tomahawkman
  auto thawkInfo = NAVIS.AddClass<Tomahawkman>();
  thawkInfo->SetSpecialDescription("Spec. Axe ability!");
  thawkInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/tomahawk/tomahawk_face.png"));
  thawkInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/tomahawk/preview.png"));
  thawkInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/tomahawk/tomahawk_OW.png"));
  thawkInfo->SetOverworldAnimationPath("resources/navis/tomahawk/tomahawk_OW.animation");
  thawkInfo->SetSpeed(2);
  thawkInfo->SetAttack(1);
  thawkInfo->SetChargedAttack(20);

  // Register Forte
  auto forteInfo = NAVIS.AddClass<Forte>();
  forteInfo->SetSpecialDescription("Too angry to die. Spawns w/ aura");
  //forteInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/forte/forte_face.png"));
  forteInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/forte/preview.png"));
  forteInfo->SetOverworldTexture(handle.Textures().LoadTextureFromFile("resources/navis/forte/forte_OW.png"));
  forteInfo->SetOverworldAnimationPath("resources/navis/forte/forte_OW.animation");
  forteInfo->SetSpeed(2);
  forteInfo->SetAttack(2);
  forteInfo->SetChargedAttack(20);

  // Script resource manager load scripts from designated folder "resources/mods/players"
  std::string path = "resources/mods/players";
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    const auto& path = entry.path();
    std::string characterName = path.filename().string();
    auto res = handle.Scripts().LoadScript(path.string() + "/entry.lua");

    if (res.result.valid()) {
      sol::state& state = *res.state;
      auto customInfo = NAVIS.AddClass<ScriptedPlayer>(state);

      // run script on meta info object
      state["load_info"](customInfo);
    }
    else {
      sol::error error = res.result;
      Logger::Logf("Failed to load player mod %s. Reason: %s", path.string().c_str(), error.what());
    }
  }
}