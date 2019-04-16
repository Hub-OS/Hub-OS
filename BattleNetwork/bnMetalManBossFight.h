#pragma once
#include "bnMobFactory.h"

class MetalManBossFight :
  public MobFactory
{
public:
  MetalManBossFight(Field* field);
  ~MetalManBossFight();

  Mob* Build();
};

