#pragma once

class Character;

/**
 * @class Agent
 * @author mav
 * @date 06/05/19
 * @file bnAgent.h
 * @brief Agents pursue a target
 */
class Agent {
private:
  std::shared_ptr<Character> target;
public:
  void SetTarget(std::shared_ptr<Character> _target) {
    target = _target;
  }

  void FreeTarget() {
    SetTarget(nullptr);
  }

  std::shared_ptr<Character> GetTarget() const { return target; }
};