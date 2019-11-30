/*! \brief Builds an Alpha boss fight with game-like ending */
#pragma once
#include "bnMobFactory.h"
class AlphaBossFight : public MobFactory
{
public:
  AlphaBossFight(Field* field);
  ~AlphaBossFight();

  /**
   * @brief Builds and returns the mob pointer
   * @return Mob*
   */
  Mob* Build();
};

