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
  bool breakCollidingObjectOnHit; /*!< Whether or not colliding objects auto-delete on contact. */

public:
  DefenseIndestructable(bool breakCollidingObjectOnHit = false);

  ~DefenseIndestructable();

  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;

  /**
   * @brief Check for breaking properties
   * @param in the attack
   * @param owner the character this is attached to
   */
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};
