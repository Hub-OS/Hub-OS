#include "bnSpell.h"
#include "bnAnimation.h"
class VolcanoErupt : public Spell {
  Animation eruptAnim;

public:
  VolcanoErupt(Field* field);
  ~VolcanoErupt();

  void OnUpdate(double elapsed) override;
  void OnDelete() override;
  void OnCollision() override;
  void Attack(Character* _entity) override;
};