#include "bnSpell.h"
#include "bnAnimation.h"
class VolcanoErupt : public Spell {
  Animation eruptAnim;

public:
  VolcanoErupt();
  ~VolcanoErupt();

  void OnUpdate(double elapsed) override;
  void OnDelete() override;
  void OnCollision(const Character*) override;
  void Attack(Character* _entity) override;
};