#pragma once

class Entity;

/**
 * @class Agent
 * @author mav
 * @date 06/05/19
 * @file bnAgent.h
 * @brief Agents pursue a target
 */
class Agent {
private:
  Entity * target;
public:
  void SetTarget(Entity* _target) {
    target = _target;
  }

  void FreeTarget() {
    SetTarget(nullptr);
  }

  Entity* GetTarget() const { return target; }
};