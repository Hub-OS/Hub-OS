#include "bnArtifact.h"
#include <random>
#include <time.h>

#include "bnArtifact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"

Artifact::Artifact(Field* _field) {
  this->SetField(_field);
  this->SetTeam(Team::UNKNOWN);
  this->SetPassthrough(true);
}

Artifact::~Artifact() {
}

void Artifact::Update(float elapsed) {
  if (IsTimeFrozen()) return;
  Entity::Update(elapsed);
  this->OnUpdate(elapsed);
}

void Artifact::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}

