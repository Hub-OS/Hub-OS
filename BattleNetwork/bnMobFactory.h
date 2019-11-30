#pragma once
#include "bnMob.h"

/*! \brief interface to build complex battle challenges and give ranked rewards */
class MobFactory
{
protected:
  Field * field; /*!< The field the battle will take place on */
public:
  /**
   * @brief constructor 
   * @param _field newly allocated field pointer
   */
  MobFactory(Field* _field) { field = _field; }
  
  /**
   * @brief deconstructor
   */
  virtual ~MobFactory() { ;  }
 
  /**
   * @brief Must be implemented. Builds the mob.
   * @return Mob*
   */
  virtual Mob* Build() = 0;
};

