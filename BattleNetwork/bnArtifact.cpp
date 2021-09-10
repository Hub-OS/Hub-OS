#include "bnArtifact.h"

Artifact::Artifact() : Entity() {
  SetTeam(Team::unknown);
  SetPassthrough(true);
}

void Artifact::Update(double elapsed) {
  Entity::Update(elapsed);
  OnUpdate(elapsed);
}
