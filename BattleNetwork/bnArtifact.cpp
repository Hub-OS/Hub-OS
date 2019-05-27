#include "bnArtifact.h"
#include <random>
#include <time.h>

#include "bnArtifact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"

Artifact::Artifact(Field* _field, Team _team) {
  this->SetField(_field);
  this->SetTeam(_team);
  this->SetPassthrough(true);
}

Artifact::Artifact() {
  SetLayer(0);
  texture = nullptr;
  this->SetPassthrough(true);
}

<<<<<<< HEAD
Artifact::~Artifact(void) {
=======
Artifact::~Artifact() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void Artifact::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!isSliding) {
    this->setPosition(tile->getPosition());
  }
}
