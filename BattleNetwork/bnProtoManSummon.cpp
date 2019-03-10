#include <random>
#include <time.h>

#include "bnProtoManSummon.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnChipSummonHandler.h"

#define RESOURCE_PATH "resources/spells/protoman_summon.animation"

ProtoManSummon::ProtoManSummon(ChipSummonHandler* _summons) : Spell()
{
  summons = _summons;
  SetPassthrough(true);
  EnableTileHighlight(false); // Do not highlight where we move

  field = summons->GetCaller()->GetField();
  team = summons->GetCaller()->GetTeam();

  direction = Direction::NONE;
  deleted = false;
  hit = false;
  progress = 0.0f;
  hitHeight = 0.0f;
  random = rand() % 20 - 20;

  int lr = (team == Team::RED) ? 1 : -1;
  setScale(2.0f*lr, 2.0f);

  Battle::Tile* _tile = summons->GetCaller()->GetTile();

  this->field->AddEntity(*this, _tile->GetX(), _tile->GetY());

  AUDIO.Play(AudioType::APPEAR);

  setTexture(*TEXTURES.LoadTextureFromFile("resources/spells/protoman_summon.png"), true);
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
	
  Battle::Tile* next = nullptr;
  while(field->GetNextTile(next)) {
	  if (next->ContainsEntityType<Character>() && next->GetTeam() != this->GetTeam()) {
		Battle::Tile* prev = field->GetAt(next->GetX() - 1, next->GetY());

		auto characters = prev->FindEntities([](Entity* in) {
			return dynamic_cast<Character*>(in);
		});
	
	    bool blocked = (characters.size() > 0) || !prev->IsWalkable();
	    
	    if(!blocked) {
			targets.push_back(next);
	    }
	  }
  }
    
  // TODO: noodely callbacks desgin might be best abstracted by ActionLists
  animationComponent.SetAnimation("APPEAR", [this] { 
	auto handleAttack = [this] () {
	    this->DoAttackStep();
    };
    
    this->animationComponent.SetAnimation("MOVE", handleAttack);
  });
}

ProtoManSummon::~ProtoManSummon() {
}

void ProtoManSummon::DoAttackStep() {
  if (targets.size() > 0) {
	Battle::Tile* prev = field->GetAt(targets[0]->GetX() - 1, targets[0]->GetY());
    this->GetTile()->RemoveEntityByID(this->GetID());
    prev->AddEntity(*this);
		
	this->animationComponent.SetAnimation("ATTACK", [this, prev] {
	  this->animationComponent.SetAnimation("MOVE", [this, prev] {
		this->DoAttackStep();
	  });
	});

	this->animationComponent.AddCallback(4,  [this]() { 
		this->targets[0]->AffectEntities(this); 
		this->targets.erase(targets.begin()); 
	}, std::function<void()>(), true);
	
  }
  else {
	this->animationComponent.SetAnimation("MOVE", [this] {
	  this->summons->RemoveEntity(this);
	});
  }
}

void ProtoManSummon::Update(float _elapsed) {
  animationComponent.Update(_elapsed);

  if (tile != nullptr) {
    setPosition(tile->getPosition());
  }

  Entity::Update(_elapsed);
}

bool ProtoManSummon::Move(Direction _direction) {
  return true;
}

void ProtoManSummon::Attack(Character* _entity) {
  if (hit || deleted) {
    return;
  }

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    if (!_entity->IsPassthrough()) {
      auto props = Hit::DefaultProperties;
      props.damage = 120;
      props.flags |= Hit::recoil;
      _entity->Hit(props);

      AUDIO.Play(AudioType::SWORD_SWING);
    }
  }
}
