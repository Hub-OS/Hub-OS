#pragma once
#include "bnMobFactory.h"

/*! \brief Spawns metal man, toggles boss flag, uses custom music, and a custom background */
class MetalManBossFight :
  public MobFactory
{
public:
  Mob* Build(Field* field);
};

