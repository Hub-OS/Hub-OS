#include "bnSpell.h"
#include "bnAnimation.h"
class VolcanoErupt : public Spell {
  Animation eruptAnim;

public:
  VolcanoErupt();
  ~VolcanoErupt();

  void OnUpdate(double elapsed) override;
  void OnDelete() override;
  void OnCollision(const std::shared_ptr<Character>) override;
  void Attack(std::shared_ptr<Character> _entity) override;
};