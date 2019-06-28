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

Artifact::Artifact() {
  SetLayer(0);
  this->SetPassthrough(true);
}

Artifact::~Artifact() {
}

void Artifact::Update(float elapsed) {
  Entity::Update(elapsed);
}

void Artifact::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}
