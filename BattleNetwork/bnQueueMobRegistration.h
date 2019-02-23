#pragma once
#include "bnMobRegistration.h"

// Register these mobs
#include "bnTwoMettaurMob.h"
#include "bnCanodumbMob.h"
#include "bnRandomMettaurMob.h"

/***********************************************************************
************    Register your custom mobs here    *********************
************************************************************************/

void QueueMobRegistration() {
  auto info = MOBS.AddClass<TwoMettaurMob>();  // Create and register mob info object
  info->SetDescription("Tutorial ranked mettaurs, you got this!"); // Set property
  info->SetPlaceholderTexturePath("resources/mobs/mettaur/preview.png");
  info->SetName("Mettaurs");
  info->SetSpeed(1);
  info->SetAttack(10);
  info->SetHP(80);

  info = MOBS.AddClass<CanodumbMob>();  // Create and register mob info object
  info->SetDescription("Family of cannon virii - Watch out!"); // Set property
  info->SetName("Canodumb Triplets");
  info->SetPlaceholderTexturePath("resources/mobs/canodumb/preview.png");
  info->SetSpeed(0);
  info->SetAttack(20);
  info->SetHP(500);

  info = MOBS.AddClass<ProgsManBossFight>();  // Create and register mob info object
  info->SetDescription("A rogue Mr.Prog! Can you stop it?"); // Set property
  info->SetName("Enter ProgsMan");
  info->SetPlaceholderTexturePath("resources/mobs/progsman/preview.png");
  info->SetSpeed(5);
  info->SetAttack(20);
  info->SetHP(600);

  info = MOBS.AddClass<RandomMettaurMob>();  // Create and register mob info object
  info->SetDescription("MetalMan throws blades, shoots missiles, and can shatter the ground."); // Set property
  info->SetName("BN4 MetalMan");
  info->SetPlaceholderTexturePath("resources/mobs/metalman/preview.png");
  info->SetSpeed(6);
  info->SetAttack(20);
  info->SetHP(1000);
}