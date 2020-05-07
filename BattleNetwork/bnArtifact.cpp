#include "bnArtifact.h"
#include <random>
#include <time.h>

#include "bnArtifact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"

Artifact::Artifact(Field* _field) {
  SetField(_field);
  SetTeam(Team::unknown);
  SetPassthrough(true);
}

Artifact::~Artifact() {
}

void Artifact::Update(float elapsed) {
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

