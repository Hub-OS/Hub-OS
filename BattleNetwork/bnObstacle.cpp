#include "bnObstacle.h"

Obstacle::Obstacle(Team _team) : Character()
{
  SetTeam(_team);
  SetFloatShoe(true);
  SetLayer(1);

  auto hitboxProperties = GetHitboxProperties();
  hitboxProperties.flags = Hit::none;
  SetHitboxProperties(hitboxProperties);

  // overwrite bubble...
  RegisterStatusCallback(Hit::bubble, [] {});
}
