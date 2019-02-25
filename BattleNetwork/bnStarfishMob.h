#pragma once
#include "bnMobFactory.h"
#include "bnStarfish.h"
#include "bnStarfishIdleState.h"

class StarfishMob :
  public MobFactory
{
public:
  StarfishMob(Field* field);
  ~StarfishMob();

  Mob* Build();
};