#pragma once
#include "bnMobFactory.h"

class MetalManBossFight2 :
  public MobFactory
{
public:
  MetalManBossFight2(Field* field);
  ~MetalManBossFight2();

  Mob* Build();
};
