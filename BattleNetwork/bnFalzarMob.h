#include "bnMobFactory.h"

class FalzarMob : public MobFactory {
public:
  FalzarMob(Field* field);

  ~FalzarMob();

  Mob* Build();
};