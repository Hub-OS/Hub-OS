/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will be replaced by a script parser to load extra content without 
 * needing to recompile... */

#pragma once

// TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

// Register these navis
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"
#include "bnMegaman.h"
#include "bnStarman.h"
#include "bnProtoman.h"
#include "bnTomahawkman.h"
#include "bnRoll.h"
#include "bnForte.h"
#include "stx/zip_utils.h"

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedPlayer.h"
#endif

static inline void QueuNaviRegistration() {
  ResourceHandle handle;

  /*********************************************************************
  **********            Register megaman            ********************
  **********************************************************************/
  auto megamanInfo = NAVIS.AddClass<Megaman>();  // Create and register navi info object
  megamanInfo->SetSpecialDescription("Well rounded stats"); // Set property
  megamanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/megaman/mega_face.png"));
  megamanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/megaman/preview.png"));
  megamanInfo->SetMugshotTexturePath("resources/navis/megaman/mug.png");
  megamanInfo->SetMugshotAnimationPath("resources/navis/megaman/mug.animation");
  megamanInfo->SetEmotionsTexturePath("resources/navis/megaman/emotions.png");
  megamanInfo->SetOverworldTexturePath("resources/navis/megaman/overworld.png");
  megamanInfo->SetOverworldAnimationPath("resources/navis/megaman/overworld.animation");
  megamanInfo->SetSpeed(1);
  megamanInfo->SetAttack(1);
  megamanInfo->SetChargedAttack(10);
  megamanInfo->SetPackageID("com.builtins.engine.hero1");
  NAVIS.Commit(megamanInfo);

  // Register Roll
  auto rollInfo = NAVIS.AddClass<Roll>();
  rollInfo->SetSpecialDescription("High HP w/ FloatShoe enabled.");
  rollInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/roll/roll_face.png"));
  rollInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/roll/preview.png"));
  rollInfo->SetMugshotTexturePath("resources/navis/roll/mug.png");
  rollInfo->SetMugshotAnimationPath("resources/navis/roll/mug.animation");
  rollInfo->SetEmotionsTexturePath("resources/navis/emotions.png");
  rollInfo->SetOverworldTexturePath("resources/navis/roll/overworld.png");
  rollInfo->SetOverworldAnimationPath("resources/navis/roll/overworld.animation");
  rollInfo->SetSpeed(2);
  rollInfo->SetAttack(2);
  rollInfo->SetChargedAttack(10);
  rollInfo->SetPackageID("com.builtins.engine.hero2");
  NAVIS.Commit(rollInfo);

  // Register Starman
  auto starmanInfo = NAVIS.AddClass<Starman>();
  starmanInfo->SetSpecialDescription("Fastest navi w/ rapid fire");
  starmanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/starman/star_face.png"));
  starmanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/starman/preview.png"));
  starmanInfo->SetMugshotTexturePath("resources/navis/starman/mug.png");
  starmanInfo->SetMugshotAnimationPath("resources/navis/starman/mug.animation");
  starmanInfo->SetEmotionsTexturePath("resources/navis/emotions.png");
  starmanInfo->SetOverworldTexturePath("resources/navis/starman/starman_OW.png");
  starmanInfo->SetOverworldAnimationPath("resources/navis/starman/starman_OW.animation");
  starmanInfo->SetSpeed(3);
  starmanInfo->SetAttack(2);
  starmanInfo->SetChargedAttack(10);
  starmanInfo->SetPackageID("com.builtins.engine.hero3");
  NAVIS.Commit(starmanInfo);

  // Register Protoman
  auto protomanInfo = NAVIS.AddClass<Protoman>();
  protomanInfo->SetSpecialDescription("Elite class navi w/ sword");
  protomanInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/protoman/protoman_face.png"));
  protomanInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/protoman/preview.png"));
  protomanInfo->SetMugshotTexturePath("resources/navis/protoman/mug.png");
  protomanInfo->SetMugshotAnimationPath("resources/navis/protoman/mug.animation");
  protomanInfo->SetEmotionsTexturePath("resources/navis/emotions.png");
  protomanInfo->SetOverworldTexturePath("resources/navis/protoman/protoman_OW.png");
  protomanInfo->SetOverworldAnimationPath("resources/navis/protoman/protoman_OW.animation");
  protomanInfo->SetSpeed(3);
  protomanInfo->SetAttack(1);
  protomanInfo->SetChargedAttack(20);
  protomanInfo->SetIsSword(true);
  protomanInfo->SetPackageID("com.builtins.engine.hero4");
  NAVIS.Commit(protomanInfo);

  // Register Tomahawkman
  auto thawkInfo = NAVIS.AddClass<Tomahawkman>();
  thawkInfo->SetSpecialDescription("Spec. Axe ability!");
  thawkInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/tomahawk/tomahawk_face.png"));
  thawkInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/tomahawk/preview.png"));
  thawkInfo->SetMugshotTexturePath("resources/navis/tomahawk/mug.png");
  thawkInfo->SetMugshotAnimationPath("resources/navis/tomahawk/mug.animation");
  thawkInfo->SetEmotionsTexturePath("resources/navis/emotions.png");
  thawkInfo->SetOverworldTexturePath("resources/navis/tomahawk/tomahawk_OW.png");
  thawkInfo->SetOverworldAnimationPath("resources/navis/tomahawk/tomahawk_OW.animation");
  thawkInfo->SetSpeed(2);
  thawkInfo->SetAttack(1);
  thawkInfo->SetChargedAttack(20);
  thawkInfo->SetPackageID("com.builtins.engine.hero5");
  NAVIS.Commit(thawkInfo);

  // Register Forte
  auto forteInfo = NAVIS.AddClass<Forte>();
  forteInfo->SetSpecialDescription("Too angry to die. Spawns w/ aura");
  forteInfo->SetIconTexture(handle.Textures().LoadTextureFromFile("resources/navis/forte/forte_face.png"));
  forteInfo->SetPreviewTexture(handle.Textures().LoadTextureFromFile("resources/navis/forte/preview.png"));
  forteInfo->SetMugshotTexturePath("resources/navis/forte/mug.png");
  forteInfo->SetMugshotAnimationPath("resources/navis/forte/mug.animation");
  forteInfo->SetEmotionsTexturePath("resources/navis/emotions.png");
  forteInfo->SetOverworldTexturePath("resources/navis/forte/forte_OW.png");
  forteInfo->SetOverworldAnimationPath("resources/navis/forte/forte_OW.animation");
  forteInfo->SetSpeed(2);
  forteInfo->SetAttack(2);
  forteInfo->SetChargedAttack(20);
  forteInfo->SetPackageID("com.builtins.engine.antihero1");
  NAVIS.Commit(forteInfo);

#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  // Script resource manager load scripts from designated folder "resources/mods/players"
  std::string path_str = "resources/mods/players";
  for (const auto& entry : std::filesystem::directory_iterator(path_str)) {
    auto full_path = std::filesystem::absolute(entry).string();

    if (full_path.find(".zip") == std::string::npos) {
      NAVIS.LoadNaviFromPackage(full_path);
    }
  }
#endif
}