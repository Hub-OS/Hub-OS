#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseNodrag
 * @author mav
 * @date 03/24/2021
 * @brief Prevents drag status
 *
 */
class DefenseNodrag : public DefenseRule {
public:
  DefenseNodrag();
  ~DefenseNodrag();

  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;

  /**
   * @brief Check for breaking properties
   * @param in the attack
   * @param owner the character this is attached to
   */
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};
