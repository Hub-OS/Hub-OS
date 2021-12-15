#pragma once
#include "bnMob.h"

/*! \brief interface to build complex battle challenges and give ranked rewards */
class MobFactory
{
public:
  /**
   * @brief deconstructor
   */
  virtual ~MobFactory() { ;  }
 
  /**
   * @brief Must be implemented. Builds the mob.
   * @return Mob*
   */
  virtual Mob* Build(std::shared_ptr<Field> field, const std::string& dataString = "") = 0;
};

