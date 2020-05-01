/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will be replaced by a script parser to load extra content without 
 * needing to recompile... */

#pragma once

// Register these navis
#include "bnMegaman.h"
#include "bnStarman.h"
#include "bnRoll.h"
#include "bnForte.h"

void QueuNaviRegistration() {
  /*********************************************************************
  **********            Register megaman            ********************
  **********************************************************************/
  auto megamanInfo = NAVIS.AddClass<Megaman>();  // Create and register navi info object
  megamanInfo->SetSpecialDescription("Star of the series. Well rounded stats."); // Set property


  // THIS IS OK B/C WE'RE LOADING FROM FILE NOT OUR RESOURCES- this should be taken out later
  megamanInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/megaman/preview.png"));

  megamanInfo->SetOverworldAnimationPath("resources/navis/megaman/megaman.animation");
  megamanInfo->SetSpeed(1);
  megamanInfo->SetAttack(1);
  megamanInfo->SetChargedAttack(10);

  // Register Roll
  auto rollInfo = NAVIS.AddClass<Roll>();
  rollInfo->SetSpecialDescription("High HP and quick to recover from hits. FloatShoe enabled.");

  // THIS IS OK B/C WE'RE LOADING FROM FILE NOT OUR RESOURCES- this should be taken out later
  rollInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/roll/preview.png"));

  rollInfo->SetOverworldAnimationPath("resources/navis/roll/roll.animation");
  rollInfo->SetSpeed(2);
  rollInfo->SetAttack(1);
  rollInfo->SetChargedAttack(10);

  // Register Starman
  auto starmanInfo = NAVIS.AddClass<Starman>();
  starmanInfo->SetSpecialDescription("Fastest navi w/ rapid fire");

  // THIS IS OK B/C WE'RE LOADING FROM FILE NOT OUR RESOURCES- this should be taken out later
  starmanInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/starman/preview.png"));

  starmanInfo->SetOverworldAnimationPath("resources/navis/starman/starman.animation");
  starmanInfo->SetSpeed(3);
  starmanInfo->SetAttack(1);
  starmanInfo->SetChargedAttack(10);

  // Register Forte
  auto forteInfo = NAVIS.AddClass<Forte>();
  forteInfo->SetSpecialDescription("Literally too angry to die. Spawns with aura.");

  // THIS IS OK B/C WE'RE LOADING FROM FILE NOT OUR RESOURCES- this should be taken out later
  forteInfo->SetPreviewTexture(LOAD_TEXTURE_FILE("resources/navis/forte/preview.png"));

  forteInfo->SetOverworldAnimationPath("resources/navis/forte/forte.animation");
  forteInfo->SetSpeed(2);
  forteInfo->SetAttack(3);
  forteInfo->SetChargedAttack(20);
}