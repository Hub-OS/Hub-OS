#include "bnArtifact.h"
#include <random>
#include <time.h>

#include "bnArtifact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"

Artifact::Artifact() : Entity() {
  SetTeam(Team::unknown);
  SetPassthrough(true);
}

Artifact::~Artifact() {
}

void Artifact::Update(double elapsed) {
  if (IsTimeFrozen()) return;
  Entity::Update(elapsed);
  OnUpdate(elapsed);
}

void Artifact::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    setPosition(tile->getPosition());
  }
}
