/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will be replaced by a script parser to load extra content without 
 * needing to recompile... */

#pragma once

// Register these navis
#include "bnMegaman.h"
#include "bnStarman.h"
#include "bnProtoman.h"
#include "bnTomahawkman.h"
#include "bnRoll.h"
#include "bnForte.h"

void QueuNaviRegistration() {
  /*********************************************************************
  **********            Register megaman            ********************
  **********************************************************************/
  auto megamanInfo = NAVIS.AddClass<Megaman>();  // Create and register navi info object
  megamanInfo->SetSpecialDescription("Star of the series. Well rounded stats."); // Set property
  megamanInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/megaman/mega_face.png"));
  megamanInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/megaman/preview.png"));
  megamanInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/megaman/overworld.png"));
  megamanInfo->SetOverworldAnimationPath("resources/navis/megaman/overworld.animation");
  megamanInfo->SetSpeed(1);
  megamanInfo->SetAttack(1);
  megamanInfo->SetChargedAttack(10);

  // Register Roll
  auto rollInfo = NAVIS.AddClass<Roll>();
  rollInfo->SetSpecialDescription("High HP and quick to recover from hits. FloatShoe enabled.");
  // rollInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/roll/roll_face.png"));
  rollInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/roll/preview.png"));
  rollInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/roll/overworld.png"));
  rollInfo->SetOverworldAnimationPath("resources/navis/roll/overworld.animation");
  rollInfo->SetSpeed(2);
  rollInfo->SetAttack(2);
  rollInfo->SetChargedAttack(10);

  // Register Starman
  auto starmanInfo = NAVIS.AddClass<Starman>();
  starmanInfo->SetSpecialDescription("Fastest navi w/ rapid fire");
  // starmanInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/starman/starman_face.png"));
  starmanInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/starman/preview.png"));
  starmanInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/starman/starman_OW.png"));
  starmanInfo->SetOverworldAnimationPath("resources/navis/starman/starman_OW.animation");
  starmanInfo->SetSpeed(3);
  starmanInfo->SetAttack(2);
  starmanInfo->SetChargedAttack(10);

  // Register Protoman
  auto protomanInfo = NAVIS.AddClass<Protoman>();
  protomanInfo->SetSpecialDescription("Elite class navi w/ sword");
  protomanInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/protoman/protoman_face.png"));
  protomanInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/protoman/preview.png"));
  protomanInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/protoman/protoman_OW.png"));
  protomanInfo->SetOverworldAnimationPath("resources/navis/protoman/protoman_OW.animation");
  protomanInfo->SetSpeed(3);
  protomanInfo->SetAttack(1);
  protomanInfo->SetChargedAttack(20);
  protomanInfo->SetIsSword(true);

  // Register Tomahawkman
  auto thawkInfo = NAVIS.AddClass<Tomahawkman>();
  thawkInfo->SetSpecialDescription("Spec. Axe ability!");
  thawkInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/tomahawk/tomahawk_face.png"));
  thawkInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/tomahawk/preview.png"));
  thawkInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/tomahawk/tomahawk_OW.png"));
  thawkInfo->SetOverworldAnimationPath("resources/navis/tomahawk/tomahawk_OW.animation");
  thawkInfo->SetSpeed(2);
  thawkInfo->SetAttack(1);
  thawkInfo->SetChargedAttack(20);

  // Register Forte
  auto forteInfo = NAVIS.AddClass<Forte>();
  forteInfo->SetSpecialDescription("Literally too angry to die. Spawns with aura.");
  //forteInfo->SetIconTexture(LOAD_TEXTURE_FILE("resources/navis/forte/forte_face.png"));
  forteInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/forte/preview.png"));
  forteInfo->SetOverworldTexture(LOAD_TEXTURE_FILE("resources/navis/forte/forte_OW.png"));
  forteInfo->SetOverworldAnimationPath("resources/navis/forte/forte_OW.animation");
  forteInfo->SetSpeed(2);
  forteInfo->SetAttack(2);
  forteInfo->SetChargedAttack(20);
}