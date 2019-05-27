#pragma once

<<<<<<< HEAD
/*
This class is used specifically for checks in the attack step*/

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class Spell;
class Character;

typedef int Priority;

<<<<<<< HEAD
class DefenseRule {
private:
  Priority priorityLevel;
=======
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

public:
  DefenseRule(Priority level);

  const Priority GetPriorityLevel() const;

  virtual ~DefenseRule();

<<<<<<< HEAD
  // Returns true if spell passes through this defense, false if defense prevents it
=======
  /**
    * @brief Returns false if spell passes through this defense, true if defense prevents it
    */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual const bool Check(Spell* in, Character* owner) = 0;
};