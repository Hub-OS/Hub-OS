#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseIndesctructable
 * @author mav
 * @date 05/05/19
 * @brief Nothing ever hits and drops guard tink effect
 *
 */
class DefenseIndestructable : public DefenseRule {
  bool breakCollidingObjectOnHit; /*!< Whether or not colliding objects delete on contact. This does will not resolve true for entity->Hit() calls */

public:
  DefenseIndestructable(bool breakCollidingObjectOnHit = false);

  virtual ~DefenseIndestructable();

  /**
   * @brief Check for breaking properties
   * @param in the attack
   * @param owner the character this is attached to
   * @return always true. Nothing ever passes.
   */
  virtual const bool Check(Spell* in, Character* owner);
};
