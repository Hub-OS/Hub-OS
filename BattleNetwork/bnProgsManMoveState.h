#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManMoveState : public AIState<ProgsMan>
{
private:
  Direction nextDirection;
  bool isMoving;
public:
  ProgsManMoveState();
  ~ProgsManMoveState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
};

