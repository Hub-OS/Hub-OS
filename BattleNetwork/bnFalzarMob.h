#include "bnMobFactory.h"

class FalzarMob : public MobFactory {
public:
  Mob* Build(Field* field);
};