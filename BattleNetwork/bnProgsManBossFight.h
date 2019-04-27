/*! \brief Builds a Progsman boss fight with ranked items */

#pragma once
#include "bnMobFactory.h"
class ProgsManBossFight : public MobFactory
{
public:
  ProgsManBossFight(Field* field);
  ~ProgsManBossFight();

  /**
   * @brief Builds and returns the mob pointer
   * @return Mob*
   */
  Mob* Build();
};

