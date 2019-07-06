#include "bnMettaur.h"
#include "bnMettaurIdleState.h"
#include "bnMettaurAttackState.h"
#include "bnMettaurMoveState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"

const std::string RESOURCE_PATH = "resources/mobs/mettaur/mettaur.animation";

vector<int> Mettaur::metIDs = vector<int>(); 
int Mettaur::currMetIndex = 0; 

Mettaur::Mettaur(Rank _rank)
  :  AI<Mettaur>(this), AnimatedCharacter(_rank) {
  //this->ChangeState<MettaurIdleState>();
  name = "Mettaur";
  SetTeam(Team::BLUE);

  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  if (GetRank() == Rank::SP) {
    SetHealth(200);
    animationComponent->SetPlaybackSpeed(1.2);
    animationComponent->SetAnimation("SP_IDLE");
  }
  else {
	  SetHealth(40);
    //Components setup and load
    animationComponent->SetAnimation("IDLE");
  }

  hitHeight = 0;

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_METTAUR));

  setScale(2.f, 2.f);

  this->SetHealth(health);

  metID = (int)Mettaur::metIDs.size();
  Mettaur::metIDs.push_back((int)Mettaur::metIDs.size());

  animationComponent->OnUpdate(0);
}

Mettaur::~Mettaur() {
}

void Mettaur::OnDelete() {
    this->ChangeState<ExplodeState<Mettaur>>();

    if (Mettaur::metIDs.size() > 0) {
        vector<int>::iterator it = find(Mettaur::metIDs.begin(), Mettaur::metIDs.end(), metID);

        if (it != Mettaur::metIDs.end()) {
            // Remove this mettaur out of rotation...
            Mettaur::currMetIndex++;

            Mettaur::metIDs.erase(it);
            if (Mettaur::currMetIndex >= Mettaur::metIDs.size()) {
                Mettaur::currMetIndex = 0;
            }
        }
    }

    this->LockState();
}

void Mettaur::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);
  setPosition(getPosition() + tileOffset);

  this->AI<Mettaur>::Update(_elapsed);
}

const bool Mettaur::OnHit(const Hit::Properties props) {
    Logger::Log("Mettaur OnHit");

  return true;
}

const float Mettaur::GetHitHeight() const {
  return hitHeight;
}

const bool Mettaur::IsMettaurTurn() const
{
  if(Mettaur::metIDs.size() > 0)
    return (Mettaur::metIDs.at(Mettaur::currMetIndex) == this->metID);

  return false;
}

void Mettaur::NextMettaurTurn() {
  Mettaur::currMetIndex++;

  if (Mettaur::currMetIndex >= Mettaur::metIDs.size()) {
    Mettaur::currMetIndex = 0;
  }
}
