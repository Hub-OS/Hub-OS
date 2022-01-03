#include "bnArtifact.h"

Artifact::Artifact() : Entity() {
  SetTeam(Team::unknown);
  SetPassthrough(true);
}