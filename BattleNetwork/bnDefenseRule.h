#pragma once
#include "bnHitProperties.h"

class Spell;
class Character;

typedef int Priority;

/**
 * @class DefenseRule
 * @author mav
 * @date 05/05/19
 * @brief This class is used specifically for checks in the attack step
 * 
 * Build custom defense rules for special chips using this class
 * and then add them to entities
 */
class DefenseRule {
private:
  Priority priorityLevel; /*!< Lowest priority goes first */
  bool replaced; /*!< If this rule has been replaced by another one in the entity*/
public:
  friend class Character;

  DefenseRule(Priority level);

  const Priority GetPriorityLevel() const;
  const bool IsReplaced() const;

  virtual ~DefenseRule();

  /**
   * @brief Filters status effects before applying them to character. e.g. SuperArmor
   * @return reference to modified input statuses
   */
  virtual Hit::Properties& FilterStatuses(Hit::Properties& statuses);
  
  /**
    * @brief Returns false if spell passes through this defense, true if defense prevents it
    */
  virtual const bool Check(Spell* in, Character* owner) = 0;
};