#include <random>
#include <time.h>
#include <algorithm>

#include "bnProtoManSummon.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSwordEffect.h"

#include "bnChipSummonHandler.h"

#define RESOURCE_PATH "resources/spells/protoman_summon.animation"

ProtoManSummon::ProtoManSummon(ChipSummonHandler* _summons) : Spell(_summons->GetCaller()->GetField(), _summons->GetCaller()->GetTeam())
{
  summons = _summons;
  SetPassthrough(true);
  EnableTileHighlight(false); // Do not highlight where we move
  random = rand() % 20 - 20;

  int lr = (team == Team::RED) ? 1 : -1;
  setScale(2.0f*lr, 2.0f);

  Battle::Tile* _tile = summons->GetCaller()->GetTile();

  this->field->AddEntity(*this, _tile->GetX(), _tile->GetY());

  AUDIO.Play(AudioType::APPEAR);

  setTexture(*TEXTURES.LoadTextureFromFile("resources/spells/protoman_summon.png"), true);

  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Load();

  Battle::Tile* next = nullptr;

  auto allTiles = field->FindTiles([](Battle::Tile* tile) { return true; });
  auto iter = allTiles.begin();

  while (iter != allTiles.end()) {
    next = (*iter);

	  if (next->ContainsEntityType<Character>() && next->GetTeam() != this->GetTeam()) {
		Battle::Tile* prev = field->GetAt(next->GetX() - 1, next->GetY());

		auto characters = prev->FindEntities([_summons](Entity* in) {
			return _summons->GetCaller() != in && (dynamic_cast<Character*>(in) && in->GetTeam() != Team::UNKNOWN);
		});

	    bool blocked = (characters.size() > 0) || !prev->IsWalkable();

	    if(!blocked) {
			targets.push_back(next);
	    }
	  }

    iter++;
  }

  // TODO: noodely callbacks desgin might be best abstracted by ActionLists
  animationComponent->SetAnimation("APPEAR", [this] {
	auto handleAttack = [this] () {
	    this->DoAttackStep();
    };
    this->animationComponent->SetAnimation("MOVE", handleAttack);
  });
}

ProtoManSummon::~ProtoManSummon() {
}

void ProtoManSummon::DoAttackStep() {
  if (targets.size() > 0) {
	Battle::Tile* prev = field->GetAt(targets[0]->GetX() - 1, targets[0]->GetY());
    this->GetTile()->RemoveEntityByID(this->GetID());
    prev->AddEntity(*this);

	this->animationComponent->SetAnimation("ATTACK", [this, prev] {
	  this->animationComponent->SetAnimation("MOVE", [this, prev] {
		this->DoAttackStep();
	  });
	});

	this->animationComponent->AddCallback(4,  [this]() {
		this->targets[0]->AffectEntities(this);
		this->targets.erase(targets.begin());
	}, std::function<void()>(), true);
  }
  else {
	this->animationComponent->SetAnimation("MOVE", [this] {
	// TODO: queue for removal by next update, dont delete immediately
	// esp during anim callbacks...
    this->Delete();
	});
  }
}

void ProtoManSummon::OnUpdate(float _elapsed) {
  if (tile != nullptr) {
    setPosition(tile->getPosition());
  }
}

bool ProtoManSummon::Move(Direction _direction) {
  return false;
}

void ProtoManSummon::Attack(Character* _entity) {
  SwordEffect* effect = new SwordEffect(GetField());
  GetField()->AddEntity(*effect, _entity->GetTile()->GetX(), _entity->GetTile()->GetY());
  this->summons->SummonEntity(effect, false);

  auto props = Hit::DefaultProperties;
  props.damage = 120;
  props.flags |= Hit::recoil;
  _entity->Hit(props);


  AUDIO.Play(AudioType::SWORD_SWING);
}
