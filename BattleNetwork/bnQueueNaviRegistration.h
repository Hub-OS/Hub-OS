/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will soon be replaced by a script parser to load extra content without 
 * needing to recompile */

#pragma once

// Register these navis
#include "bnPlayer.h"
#include "bnStarman.h"
#include "bnRoll.h"


void QueuNaviRegistration() {
  /*********************************************************************
  **********            Register megaman            ********************
  **********************************************************************/
  auto megamanInfo = NAVIS.AddClass<Player>();  // Create and register navi info object
  megamanInfo->SetSpecialDescription("Star of the series. Well rounded stats."); // Set property
  megamanInfo->SetBattleAnimationPath("resources/navis/megaman/megaman.animation");
  megamanInfo->SetOverworldAnimationPath("resources/navis/megaman/megaman.animation");
  megamanInfo->SetSpeed(1);
  megamanInfo->SetAttack(1);
  megamanInfo->SetChargedAttack(10);

  // Register Starman
  auto starmanInfo = NAVIS.AddClass<Starman>();
  starmanInfo->SetSpecialDescription("Projectile chips turn into arrows w/ rapid fire");
  starmanInfo->SetBattleAnimationPath("resources/navis/starman/starman.animation");
  starmanInfo->SetOverworldAnimationPath("resources/navis/starman/starman.animation");
  starmanInfo->SetSpeed(3);
  starmanInfo->SetAttack(1);
  starmanInfo->SetChargedAttack(10);

  // Register Roll
  auto rollInfo = NAVIS.AddClass<Roll>();
  rollInfo->SetSpecialDescription("High HP aand takes no recoil. FloatShoe enabled.");
  rollInfo->SetBattleAnimationPath("resources/navis/roll/roll.animation");
  rollInfo->SetOverworldAnimationPath("resources/navis/roll/roll.animation");
  rollInfo->SetSpeed(3);
  rollInfo->SetAttack(1);
  rollInfo->SetChargedAttack(10);
}