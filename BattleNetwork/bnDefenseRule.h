#pragma once

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

public:
  DefenseRule(Priority level);

  const Priority GetPriorityLevel() const;

  virtual ~DefenseRule();

  /**
    * @brief Returns false if spell passes through this defense, true if defense prevents it
    */
  virtual const bool Check(Spell* in, Character* owner) = 0;
};