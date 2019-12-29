#include "bnTile.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnSpell.h"
#include "bnArtifact.h"

#include "bnPlayer.h"
#include "bnExplosion.h"

#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnField.h"

#define TILE_WIDTH 40.0f
#define TILE_HEIGHT 30.0f
#define START_X 0.0f
#define START_Y 144.f
#define COOLDOWN 10.f
#define FLICKER 3.0f
#define Y_OFFSET 10.0f

namespace Battle {
  float Tile::brokenCooldownLength = COOLDOWN;
  float Tile::teamCooldownLength = COOLDOWN;
  float Tile::flickerTeamCooldownLength = FLICKER;

  Tile::Tile(int _x, int _y) : animation() {
    totalElapsed = 0;
    x = _x;
    y = _y;
    if (x <= 3) {
      team = Team::RED;
    }
    else {
      team = Team::BLUE;
    }

    state = TileState::NORMAL;
    elapsed = 0;
    entities = vector<Entity*>();
    setScale(2.f, 2.f);
    width = TILE_WIDTH * getScale().x;
    height = TILE_HEIGHT * getScale().y;
    setOrigin(TILE_WIDTH / 2.0f, TILE_HEIGHT / 2.0f);
    setPosition((width/2.0f) + ((x - 1) * width) + START_X, (height/2.0f) + ((y - 1) * (height - Y_OFFSET)) + START_Y);
    willHighlight = false;
    isBattleActive = false;
    brokenCooldown = 0;
    flickerTeamCooldown = teamCooldown = 0;
    red_team_atlas = blue_team_atlas = nullptr; // Set by field

    burncycle = 0.12; // milliseconds
    elapsedBurnTime = burncycle;

    highlightMode = Highlight::none;
  }

  Tile& Tile::operator=(const Tile & other)
  {
    x = other.x;
    y = other.y;

    totalElapsed = other.totalElapsed;
    team = other.team;
    state = other.state;
    RefreshTexture();
    elapsed = other.elapsed;
    entities = other.entities;
    setScale(2.f, 2.f);
    width = other.width;
    height = other.height;
    animState = other.animState;
    setPosition(((x - 1) * width) + START_X, ((y - 1) * (height - Y_OFFSET)) + START_Y);
    willHighlight = other.willHighlight;
    reserved = other.reserved;
    characters = other.characters;
    spells = other.spells;
    entities = other.entities;
    isBattleActive = other.isBattleActive;
    brokenCooldown = other.brokenCooldown;
    teamCooldown = other.teamCooldown;
    flickerTeamCooldown = other.flickerTeamCooldown;
    red_team_atlas = other.red_team_atlas;
    blue_team_atlas = other.blue_team_atlas;
    animation = other.animation;
    burncycle = other.burncycle;
    elapsedBurnTime = other.elapsedBurnTime;
    highlightMode = other.highlightMode;


    return *this;
  }


  Tile::Tile(const Tile & other)
  {
    x = other.x;
    y = other.y;

    totalElapsed = other.totalElapsed;
    team = other.team;
    state = other.state;
    RefreshTexture();
    elapsed = other.elapsed;
    entities = other.entities;
    setScale(2.f, 2.f);
    width = other.width;
    height = other.height;
    animState = other.animState;
    setPosition(((x - 1) * width) + START_X, ((y - 1) * (height - Y_OFFSET)) + START_Y);
    willHighlight = other.willHighlight;
    isBattleActive = other.isBattleActive;
    reserved = other.reserved;
    characters = other.characters;
    spells = other.spells;
    entities = other.entities;
    brokenCooldown = other.brokenCooldown;
    teamCooldown = other.teamCooldown;
    flickerTeamCooldown = other.flickerTeamCooldown;
    red_team_atlas = other.red_team_atlas;
    blue_team_atlas = other.blue_team_atlas;
    animation = other.animation;
    burncycle = other.burncycle;
    elapsedBurnTime = other.elapsedBurnTime;
    highlightMode = other.highlightMode;

  }

  Tile::~Tile() {
    // Free memory
    auto iter = entities.begin();

    while (iter != entities.end()) {
      delete (*iter);
      iter++;
    }

    entities.clear();
    spells.clear();
    artifacts.clear();
    characters.clear();
  }

  void Tile::SetField(Field* _field) {
    field = _field;
  }

  const int Tile::GetX() const {
    return x;
  }

  const int Tile::GetY() const {
    return y;
  }

  const Team Tile::GetTeam() const {
    return team;
  }

  void Tile::SetTeam(Team _team, bool useFlicker) {
    // Check if no characters on the opposing team are on this tile
    if (this->GetTeam() == Team::UNKNOWN || this->GetTeam() != _team) {
      size_t size = this->FindEntities([this](Entity* in) {
        return in->GetTeam() == this->GetTeam();
      }).size();

      if (size == 0 && this->reserved.size() == 0) {
        team = _team;
        
        if(useFlicker) {
          this->flickerTeamCooldown = this->flickerTeamCooldownLength;
        }
        else {
          this->flickerTeamCooldown = 0; // cancel 
          this->teamCooldown = this->teamCooldownLength;
        }

        this->RefreshTexture();
      }
    }
  }

  float Tile::GetWidth() const {
    return width;
  }

  float Tile::GetHeight() const {
    return height;
  }

  const TileState Tile::GetState() const {
    return state;
  }

  void Tile::SetState(TileState _state) {
    if (IsEdgeTile()) return; // edge tiles are immutable

    if (_state == TileState::BROKEN) {
      if(this->characters.size() || this->reserved.size()) {
        return;
      } else {
        brokenCooldown = brokenCooldownLength;
      }
    }

    if (_state == TileState::CRACKED && (state == TileState::EMPTY || state == TileState::BROKEN)) {
      return;
    }

    state = _state;
  }

  // Set the right texture based on the team color and state
  void Tile::RefreshTexture() {
    // Hack to toggle between team color without rewriting redundant code
    auto currTeam = team;
    auto otherTeam = (team == Team::UNKNOWN) ? Team::UNKNOWN : (team == Team::RED) ? Team::BLUE : Team::RED;

    auto prevAnimState = animState;

    ((int)(flickerTeamCooldown * 100) % 2 == 0 && flickerTeamCooldown <= flickerTeamCooldownLength) ? currTeam : currTeam = otherTeam;

    if (state == TileState::BROKEN) {
      // Broken tiles flicker when they regen
      animState = ((int)(brokenCooldown * 100) % 2 == 0 && brokenCooldown <= FLICKER) ? std::move(GetAnimState(TileState::NORMAL)) : std::move(GetAnimState(state));
    }
    else {
      animState = std::move(GetAnimState(state));
    }

    if (currTeam == Team::RED) {
      this->setTexture(*red_team_atlas);
    }
    else {
      this->setTexture(*blue_team_atlas);
    }

    if (prevAnimState != animState) {
      animation.SetAnimation(animState);
      animation << Animator::Mode::Loop;
    }
  }

  bool Tile::IsWalkable() const {
    return (state != TileState::BROKEN && state != TileState::EMPTY && !IsEdgeTile());
  }

  bool Tile::IsCracked() const {
    return state == TileState::CRACKED;
  }

  bool Tile::IsEdgeTile() const
  {
    return GetX() == 0 || GetX() == 7 || GetY() == 0 || GetY() == 4;
  }

  bool Tile::IsHighlighted() const {
    return willHighlight;
  }

  bool Tile::IsReservedByCharacter()
  {
    return (this->reserved.size() != 0);
  }

  void Tile::AddEntity(Spell & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      spells.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Character & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      characters.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Obstacle & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      spells.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Artifact & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      artifacts.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  // Aux function
  void Tile::AddEntity(Entity* _entity) {
    _entity->SetTile(this);

    // May be part of the spawn routine
    // First tile set means entity is live and ready to go
    _entity->Spawn(*this);

    auto reservedIter = reserved.find(_entity->GetID());
    if (reservedIter != reserved.end()) { reserved.erase(reservedIter); }
    entities.push_back(_entity);
  }

  bool Tile::RemoveEntityByID(long ID)
  {
    bool modified = false;

    // This is for queued entities that have not been spawned yet
    // But are requested to be removed on the same frame
    field->TileRequestsRemovalOfQueued(this, ID);

    // If the entity was in the reserved list, remove it
    auto reservedIter = reserved.find(ID);
    if (reservedIter != reserved.end()) { reserved.erase(reservedIter); }

    bool doBreakState = false;

    auto itEnt   = find_if(entities.begin(), entities.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itSpell = find_if(spells.begin(), spells.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itChar  = find_if(characters.begin(), characters.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itArt   = find_if(artifacts.begin(), artifacts.end(), [&ID](Entity* in) { return in->GetID() == ID; });

    if (itEnt != entities.end()) {
      // TODO: HasFloatShoe and HasAirShoe should be a component and use the component system

      // If removing an entity and the tile was broken, crack the tile
      if(reserved.size() == 0 && dynamic_cast<Character*>(*itEnt) != nullptr && (IsCracked() && !((*itEnt)->HasFloatShoe() || (*itEnt)->HasAirShoe()))) {
        doBreakState = true;
      }

      entities.erase(itEnt);

      modified = true;
    }

    if (itSpell != spells.end()) {
      spells.erase(itSpell);

      auto tagged = std::find_if(taggedSpells.begin(), taggedSpells.end(), [&ID](int in) { return ID == in; });
      if (tagged != taggedSpells.end()) {
        taggedSpells.erase(tagged);
      }
    }

    if (itChar != characters.end()) {
      characters.erase(itChar);
    }

    if (itArt != artifacts.end()) {
      artifacts.erase(itArt);
    }

    if (doBreakState) {
      SetState(TileState::BROKEN);
      AUDIO.Play(AudioType::PANEL_CRACK);
    }

    return modified;
  }

  bool Tile::ContainsEntity(Entity* _entity) const {
    vector<Entity*> copy = this->entities;
    return find(copy.begin(), copy.end(), _entity) != copy.end();
  }

  void Tile::ReserveEntityByID(long ID)
  {
    reserved.insert(ID);
  }

  void Tile::AffectEntities(Spell* caller) {
    if (std::find_if(taggedSpells.begin(), taggedSpells.end(), [&caller](int ID) { return ID == caller->GetID(); }) != taggedSpells.end())
      return;
    if (std::find_if(queuedSpells.begin(), queuedSpells.end(), [&caller](int ID) { return ID == caller->GetID(); }) != queuedSpells.end())
      return;
    queuedSpells.push_back(caller->GetID());
  }

  void Tile::PerformSpellAttack(Spell* caller) {
    auto entities_copy = entities; // may be modified after hitboxes are resolved

    for (auto it = entities_copy.begin(); it != entities_copy.end(); ++it) {
      if (*it == caller)
        continue;

      // TODO: use group buckets to poll by ID instead of dynam casting
      Character *c = dynamic_cast<Character *>(*it);

      // the entity is a character (can be hit) and the team isn't the same
      // we see if it passes defense checks, then call attack

      if( c && (c->GetTeam() != caller->GetTeam() ||
                                            (c->GetTeam() == Team::UNKNOWN &&
                                            caller->GetTeam() == Team::UNKNOWN))) {
        if (!c->CheckDefenses(caller)) {
          caller->Attack(c);
        }

        // Tag the spell
        // only ignore spells that have already hit something on a tile
        // this is similar to the hitbox being removed in mmbn mechanics
        taggedSpells.push_back(caller->GetID());
      }
    }
  }

  /*

  */
  void Tile::Update(float _elapsed) {
    willHighlight = false;
    totalElapsed += _elapsed;

    if (isBattleActive) {
        // LAVA TILES
        elapsedBurnTime -= _elapsed;
    }

    /*
    // NOTE: There has got to be some opportunity for optimization around here
    */

    int i = 0;

    // Step through the entity bucket (all entity types)
    while (i < entities.size()) {
      auto ptr = entities[i];

      // If the entity is marked for deletion
      if (ptr->IsDeleted()) {
        // free memory
        long ID = ptr->GetID();

        // TODO: do we need to invoke this here?
        //entities[i]->OnDelete();

        if (RemoveEntityByID(ID)) {
          // TODO: STOP DYNAMIC CASTING! SOMEHOW!
          Character* character = dynamic_cast<Character*>(ptr);

          // We only want to know about character deletions since they are the actors in the battle
          if (character) {
            this->field->CharacterDeletePublisher::Broadcast(*character);
          }

          delete ptr;
          continue;
        }
      }
      else {
        ptr->SetBattleActive(this->isBattleActive);
      }

      i++;
    }

    this->highlightMode = Highlight::none;

    vector<Spell*> spells_copy = spells;
    for (vector<Spell*>::iterator entity = spells_copy.begin(); entity != spells_copy.end(); entity++) {
      int request = (int)(*entity)->GetTileHighlightMode();

      if (request > (int)highlightMode) {
        highlightMode = (Highlight)request;
      }

      (*entity)->Update(_elapsed);
    }

    // Spells dont cause damage when the battle is over
    if (this->isBattleActive) {
      // Now that spells and characters have updated and moved, they are due to check for attack outcomes
      for (auto ID : queuedSpells) {
        // TODO: REDO THIS LOOP. IT IS HORRIBLE AND COSTLY!
        auto list = field->FindEntities([ID](Entity* e) {
          return e->GetID() == ID;
        });

        if (list.size()) {
          auto spell = dynamic_cast<Spell*>(list[0]);
          this->PerformSpellAttack(spell);
        }
      }
    }

    vector<Artifact*> artifacts_copy = artifacts;
    for (vector<Artifact*>::iterator entity = artifacts_copy.begin(); entity != artifacts_copy.end(); entity++) {
      (*entity)->Update(_elapsed);
    }

    vector<Character*> characters_copy = characters;
    for (vector<Character*>::iterator entity = characters_copy.begin(); entity != characters_copy.end(); entity++) {
      // Allow user input to move them out of tiles if they are frame perfect
      (*entity)->Update(_elapsed);
      HandleTileBehaviors(*entity);
    }

    // empty queue for next frame
    queuedSpells.clear();

    if (this->isBattleActive) {
      if (teamCooldown > 0) {
        teamCooldown -= 1.0f * _elapsed;
        if (teamCooldown < 0) teamCooldown = 0;
      }

      if (flickerTeamCooldown > 0) {
        flickerTeamCooldown -= 1.0f * _elapsed;
        if (flickerTeamCooldown < 0) flickerTeamCooldown = 0;
      }

      if (state == TileState::BROKEN) {
        brokenCooldown -= 1.0f * _elapsed;

        if (brokenCooldown < 0) { brokenCooldown = 0; state = TileState::NORMAL; }
      }
    }

    this->RefreshTexture();

    animation.SyncTime(totalElapsed);
    animation.Refresh(*this);

    switch (highlightMode) {
    case Highlight::solid:
      willHighlight = true;
      break;
    case Highlight::flash:
      willHighlight = (int)(totalElapsed * 15) % 2 == 0;
      break;
    default:
      willHighlight = false;
    }

    // animation will want to reset the sprite's origin. Prevent this.
    setOrigin(TILE_WIDTH / 2.0f, TILE_HEIGHT / 2.0f);
  }

  void Tile::SetBattleActive(bool state)
  {
    isBattleActive = state;
  }

  void Tile::HandleTileBehaviors(Obstacle* obst) {
    if (this->isBattleActive) {
      // DIRECTIONAL TILES
      auto directional = Direction::NONE;

      auto notMoving = obst->GetNextTile() == nullptr;

      switch (GetState()) {
      case TileState::DIRECTION_DOWN:
        directional = Direction::DOWN;
        break;
      case TileState::DIRECTION_UP:
        directional = Direction::UP;
        break;
      case TileState::DIRECTION_LEFT:
        directional = Direction::LEFT;
        break;
      case TileState::DIRECTION_RIGHT:
        directional = Direction::RIGHT;
        break;
      }

      if (directional != Direction::NONE) {
        if (!obst->HasAirShoe() && !obst->HasFloatShoe()) {
          if (!obst->IsSliding() && notMoving) {
            obst->SlideToTile(true);
            obst->Move(directional);
          }
        }
      }
    }
  }

  void Tile::HandleTileBehaviors(Character* character)
  {
    /*
     Special tile rules for directional pads
     Only if the entity isn't moving this frame (has a null next tile)
     and if they are not floating, we push the entity in a specific direction
     */

    if (this->isBattleActive) {
      // LAVA TILES
      if (!character->HasFloatShoe()) {
        if (GetState() == TileState::POISON) {
          if (elapsedBurnTime <= 0) {
            if (character->Hit(Hit::Properties({ 1, Hit::pierce, Element::NONE, nullptr, Direction::NONE }))) {
              elapsedBurnTime = burncycle;
            }
          }
        }
        else {
          elapsedBurnTime = 0;
        }

        if (GetState() == TileState::LAVA && character->GetElement() != Element::FIRE) {
          if (character->Hit(Hit::Properties({ 50, Hit::pierce | Hit::flinch, Element::FIRE, nullptr, Direction::NONE }))) {
            Artifact* explosion = new Explosion(field, this->GetTeam(), 1);
            field->AddEntity(*explosion, GetX(), GetY());
            SetState(TileState::NORMAL);
          }
        }
      }

      // DIRECTIONAL TILES
      auto directional = Direction::NONE;

      auto notMoving = character->GetNextTile() == nullptr;

      switch (GetState()) {
      case TileState::DIRECTION_DOWN:
        directional = Direction::DOWN;
        break;
      case TileState::DIRECTION_UP:
        directional = Direction::UP;
        break;
      case TileState::DIRECTION_LEFT:
        directional = Direction::LEFT;
        break;
      case TileState::DIRECTION_RIGHT:
        directional = Direction::RIGHT;
        break;
      }

      if (directional != Direction::NONE) {
        if (!character->HasAirShoe() && !character->HasFloatShoe()) {
          if (!character->IsSliding() && notMoving) {
            character->SlideToTile(true);
            character->Move(directional);
          }
        }
      }
    }
  }

  std::vector<Entity*> Tile::FindEntities(std::function<bool(Entity* e)> query)
  {
    std::vector<Entity*> res;

    for(auto iter = this->entities.begin(); iter != this->entities.end(); iter++ ) {
      if (query(*iter)) {
        res.push_back(*iter);
      }
    }

    return res;
  }

  std::string Tile::GetAnimState(const TileState state)
  {
    std::string str = "row_" + std::to_string(4 - GetY()) + "_";

    switch (state) {
    case TileState::BROKEN:
      str = str + "broken";
      break;
    case TileState::CRACKED:
      str = str + "cracked";
      break;
    case TileState::EMPTY:
      str = str + "empty";
      break;
    case TileState::GRASS:
      str = str + "grass";
      break;
    case TileState::ICE:
      str = str + "ice";
      break;
    case TileState::LAVA:
      str = str + "lava";
      break;
    case TileState::NORMAL:
      str = str + "normal";
      break;
    case TileState::POISON:
      str = str + "poison";
      break;
    case TileState::DIRECTION_DOWN:
      str = str + "direction_down";
      break;
    case TileState::DIRECTION_LEFT:
      str = str + "direction_left";
      break;
    case TileState::DIRECTION_RIGHT:
      str = str + "direction_right";
      break;
    case TileState::DIRECTION_UP:
      str = str + "direction_up";
      break;
    case TileState::VOLCANO:
      str = str + "volcano";
      break;
    case TileState::HOLY:
      str = str + "holy";
      break;
    default:
      str = str + "normal";
    }

    if (GetX() == 0 || GetX() == 7 || GetY() == 0 || GetY() == 4) {
      str = "row_1_normal";
    }

    return str;
  }

}