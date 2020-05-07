#pragma once
#include "bnHitProperties.h"
#include "bnDefenseFrameStateArbiter.h"

class Spell;
class Character;

typedef int Priority;

/**
 * @class DefenseRule
 * @author mav
 * @date 05/05/19
 * @brief This class is used specifically for checks in the attack step
 * 
 * Build custom defense rules for special cards using this class
 * and then add them to entities
 */
class DefenseRule {
private:
  Priority priorityLevel; /*!< Lowest priority goes first */
  bool replaced; /*!< If this rule has been replaced by another one in the entity*/

public:
  friend class Character;

  /**
  * @brief Constructs a defense rule with a priority level
  */
  DefenseRule(Priority level);

  /**
  * @brief Returns the priority level of this defense rule
  */
  const Priority GetPriorityLevel() const;
  
  /**
  * @brief True if the defense rule has been flagged for replacement by another with a priority level of equal value
  */
  const bool IsReplaced() const;

  /**
  * @brief Deconstructor
  */
  virtual ~DefenseRule();

  /**
   * @brief Filters status effects before applying them to character. e.g. SuperArmor
   * @return reference to modified input statuses
   */
  virtual Hit::Properties& FilterStatuses(Hit::Properties& statuses);
  
  /**
    * @brief Returns false if spell passes through this defense, true if defense prevents it
    */
  virtual void CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner) = 0;
};