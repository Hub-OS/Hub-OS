#pragma once
#include "bnMobFactory.h"

/*! \brief same as MetalManBossFight but every tile is now ICE */
class MetalManBossFight2 :
  public MobFactory
{
public:
  MetalManBossFight2(Field* field);
  ~MetalManBossFight2();

  Mob* Build();
};
