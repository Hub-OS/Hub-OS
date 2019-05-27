#include "bnTile.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnSpell.h"
#include "bnArtifact.h"

#include "bnPlayer.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnField.h"

#define START_X 0.0f
#define START_Y 144.f
#define COOLDOWN 10.f
#define FLICKER 3.0f
#define Y_OFFSET 10.0f

namespace Battle {
  Tile::Tile(int _x, int _y) {
    x = _x;
    y = _y;
    if (x <= 3) {
      team = Team::RED;
    }
    else {
      team = Team::BLUE;
    }
    cooldown = 0.0f;
    cooldownLength = COOLDOWN;
    state = TileState::NORMAL;
    RefreshTexture();
    elapsed = 0;
    entities = vector<Entity*>();
    setScale(2.f, 2.f);
    width = getTextureRect().width * getScale().x;
    height = getTextureRect().height * getScale().y;
    setOrigin(getTextureRect().width / 2.0f, getTextureRect().height / 2.0f);
    setPosition((width/2.0f) + ((x - 1) * width) + START_X, (height/2.0f) + ((y - 1) * (height - Y_OFFSET)) + START_Y);
    hasSpell = false;
    isBattleActive = false;
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

  void Tile::SetTeam(Team _team) {
    // Check if no characters on the opposing team are on this tile
    if (this->GetTeam() == Team::UNKNOWN || this->GetTeam() != _team) {
      size_t size = this->FindEntities([this](Entity* in) {
        return in->GetTeam() == this->GetTeam();
      }).size();

      if (size == 0 && this->reserved.size() == 0) {
        team = _team;
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

  Tile& Tile::operator=(const Tile & other)
  {
    x = other.x;
    y = other.y;
    
    team = other.team;

    cooldown = other.cooldown;
    cooldownLength = COOLDOWN;
    state = other.state;
    RefreshTexture();
    elapsed = other.elapsed;
    entities = other.entities;
    setScale(2.f, 2.f);
    width = getTextureRect().width * getScale().x;
    height = getTextureRect().height * getScale().y;
    setPosition(((x - 1) * width) + START_X, ((y - 1) * (height - Y_OFFSET)) + START_Y);
    hasSpell = other.hasSpell;
    reserved = other.reserved;
    characters = other.characters;
    spells = other.spells;
    entities = other.entities;
    isBattleActive = other.isBattleActive;

    return *this;
  }


  Tile::Tile(const Tile & other)
  {
    x = other.x;
    y = other.y;

    team = other.team;

    cooldown = other.cooldown;
    cooldownLength = COOLDOWN;
    state = other.state;
    RefreshTexture();
    elapsed = other.elapsed;
    entities = other.entities;
    setScale(2.f, 2.f);
    width = getTextureRect().width * getScale().x;
    height = getTextureRect().height * getScale().y;
    setPosition(((x - 1) * width) + START_X, ((y - 1) * (height - Y_OFFSET)) + START_Y);
    hasSpell = other.hasSpell;
    isBattleActive = other.isBattleActive;
    reserved = other.reserved;
    characters = other.characters;
    spells = other.spells;
    entities = other.entities;
  }

  const TileState Tile::GetState() const {
    return state;
  }

  void Tile::SetState(TileState _state) {
    if (_state == TileState::BROKEN && (this->characters.size() || this->reserved.size())) {
      return;
    }

    if (_state == TileState::CRACKED && (state == TileState::EMPTY || state == TileState::BROKEN)) {
      return;
    }

    if (_state == TileState::CRACKED) {
      cooldown = cooldownLength;
    }

    state = _state;
  }

  // Set the right texture based on the team color and state
  void Tile::RefreshTexture() {
    if (state == TileState::NORMAL) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_NORMAL;
      }
      else {
        textureType = TextureType::TILE_RED_NORMAL;
      }
    }
    else if (state == TileState::CRACKED) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_CRACKED;
      }
      else {
        textureType = TextureType::TILE_RED_CRACKED;
      }
    }
    else if (state == TileState::BROKEN) {
      // Broken tiles flicker when they regen
      if (team == Team::BLUE) {
        textureType = ((int)(cooldown * 100) % 2 == 0 && cooldown <= FLICKER) ? TextureType::TILE_BLUE_NORMAL : TextureType::TILE_BLUE_BROKEN;
      }
      else {
        textureType = ((int)(cooldown * 100) % 2 == 0 && cooldown <= FLICKER) ? TextureType::TILE_RED_NORMAL : TextureType::TILE_RED_BROKEN;
      }
    }
    else if (state == TileState::EMPTY) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_EMPTY;
      }
      else {
        textureType = TextureType::TILE_RED_EMPTY;
      }
    }
    else if (state == TileState::ICE) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_ICE;
      }
      else {
        textureType = TextureType::TILE_RED_ICE;
      }
    }
    else if (state == TileState::GRASS) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_GRASS;
      }
      else {
        textureType = TextureType::TILE_RED_GRASS;
      }
    }
    else if (state == TileState::POISON) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_PURPLE;
      }
      else {
        textureType = TextureType::TILE_RED_PURPLE;
      }
    }
    else if (state == TileState::LAVA) {
      if (team == Team::BLUE) {
        textureType = TextureType::TILE_BLUE_LAVA;
      }
      else {
        textureType = TextureType::TILE_RED_LAVA;
      }
    }
    else {
      assert(false && "Tile in invalid state");
    }
    setTexture(*TEXTURES.GetTexture(textureType));
  }

  bool Tile::IsWalkable() const {
    return (state != TileState::BROKEN && state != TileState::EMPTY);
  }

  bool Tile::IsCracked() const {
    return state == TileState::CRACKED;
  }

  bool Tile::IsHighlighted() const {
    return hasSpell;
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
    if (!ContainsEntity((Obstacle*)&_entity)) {
      spells.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Artifact & _entity)
  {
    if (!ContainsEntity((Artifact*)&_entity)) {
      artifacts.push_back(&_entity);
      this->AddEntity(&_entity);
    }
  }

  // Aux function
  void Tile::AddEntity(Entity* _entity) {
    _entity->SetTile(this);
    auto reservedIter = reserved.find(_entity->GetID());
    if (reservedIter != reserved.end()) { reserved.erase(reservedIter); }
    entities.push_back(_entity);

    // Sort by layer (draw order)
    // e.g. layer 0 draws first so it must be last in the draw list
    std::sort(entities.begin(), entities.end(), [](Entity* a, Entity* b) { return a->GetLayer() > b->GetLayer(); });
  }

  void Tile::RemoveEntityByID(int ID)
  {
    auto itEnt   = find_if(entities.begin(), entities.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itSpell = find_if(spells.begin(), spells.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itChar  = find_if(characters.begin(), characters.end(), [&ID](Entity* in) { return in->GetID() == ID; });
    auto itArt   = find_if(artifacts.begin(), artifacts.end(), [&ID](Entity* in) { return in->GetID() == ID; });

    if (itEnt != entities.end()) {
      // TODO: HasFloatShoe and HasAirShoe should be a component and use the component system
      // If removing an entity and the tile was broken, crack the tile
      if(reserved.size() == 0 && dynamic_cast<Character*>(*itEnt) != nullptr && (IsCracked() && !((*itEnt)->HasFloatShoe() || (*itEnt)->HasAirShoe()))) {
        state = TileState::BROKEN;
        AUDIO.Play(AudioType::PANEL_CRACK);
      }
      auto tagged = std::find_if(taggedSpells.begin(), taggedSpells.end(), [&ID](int in) { return ID == in; });
      if (tagged != taggedSpells.end()) {
        taggedSpells.erase(tagged);
      }

      entities.erase(itEnt);
    }

    if (itSpell != spells.end()) {
      spells.erase(itSpell);
    }

    if (itChar != characters.end()) {
      characters.erase(itChar);
    }

    if (itArt != artifacts.end()) {
      artifacts.erase(itArt);
    }
  }

  bool Tile::ContainsEntity(Entity* _entity) const {
    vector<Entity*> copy = this->entities;
    return find(copy.begin(), copy.end(), _entity) != copy.end();
  }

  void Tile::ReserveEntityByID(int ID)
  {
    reserved.insert(ID);
  }

  void Tile::AffectEntities(Spell* caller) {
    // If the spell has already been tagged for this tile, ignore it
    if (std::find_if(taggedSpells.begin(), taggedSpells.end(), [&caller](int ID) { return ID == caller->GetID(); }) != taggedSpells.end())
      return;

    // Cleanup before main loop just in case
    // NOTE: this fuction has been modified since earlier builds that needed this
    //       possibly can remove the following lines
    for (auto it = entities.begin(); it != entities.end(); ++it) {
      if (*it == nullptr) {
        it = entities.erase(it);
        continue;
      }
    }

    auto entities_copy = entities; // list may be modified after hitboxes are resolved

    // Spells dont cause damage when the battle is over
    if (this->isBattleActive) {
      bool tag = false;
      for (auto it = entities_copy.begin(); it != entities_copy.end(); ++it) {
        if (*it == caller)
          continue;

        // TODO: use group buckets to poll by ID instead of dy casting
        Character* c = dynamic_cast<Character*>(*it);

        // If the entity is tangible, the entity is a character (can be hit), and the team isn't the same
        // we call attack
        if (!(*it)->IsPassthrough() && c && (c->GetTeam() != caller->GetTeam() || (c->GetTeam() == Team::UNKNOWN && caller->GetTeam() == Team::UNKNOWN))) {
          if (!c->CheckDefenses(caller)) {
            caller->Attack(c);
          }
          
          // Tag the spell
          tag = true;
        }
      }

      // only ignore spells that have already hit something on a tile
      // this is similar to the hitbox being removed in mmbn mechanics
      if (tag) {
        taggedSpells.push_back(caller->GetID());
      }
    }
  }

  void Tile::Update(float _elapsed) {
    hasSpell = false;

    /*
    // NOTE: There has got to be some opportunity for optimization around here
    */

    auto itEnt = entities.begin();
    auto itSpell = spells.begin();
    auto itChar = characters.begin();
    auto itArt = artifacts.begin();

    // Step through the entity bucket (all entity types)
    while (itEnt != entities.end()) {
      // If the entity is marked for deletion
      if ((*itEnt)->IsDeleted()) {
        long ID = (*itEnt)->GetID();
        
        // If the entity was in the reserved list, remove it
        auto reservedIter = reserved.find(ID);
        if (reservedIter != reserved.end()) { reserved.erase(reservedIter); }

        // Find other buckets this belongs to
        auto fitEnt = find_if(entities.begin(), entities.end(), [&ID](Entity* in) { return in->GetID() == ID; });
        auto fitSpell = find_if(spells.begin(), spells.end(), [&ID](Entity* in) { return in->GetID() == ID; });
        auto fitChar = find_if(characters.begin(), characters.end(), [&ID](Entity* in) { return in->GetID() == ID; });
        auto fitArt = find_if(artifacts.begin(), artifacts.end(), [&ID](Entity* in) { return in->GetID() == ID; });

        // Remove them from the tile's bucket
        
        if (fitSpell != spells.end()) {
          spells.erase(fitSpell);
        }

        if (fitChar != characters.end()) {
          characters.erase(fitChar);
        }

        if (fitArt != artifacts.end()) {
          artifacts.erase(fitArt);
        }

        // free memory
        delete *itEnt;
        
        // update the iterator
        itEnt = entities.erase(itEnt);
      }
      else {
        itEnt++;
      }
    }
    
    // Update every type bucket

    vector<Artifact*> artifacts_copy = artifacts;
    for (vector<Artifact*>::iterator entity = artifacts_copy.begin(); entity != artifacts_copy.end(); entity++) {

      if ((*entity)->IsDeleted() || (*entity) == nullptr)
        continue;

      (*entity)->SetBattleActive(isBattleActive);
      (*entity)->Update(_elapsed);
    }

    vector<Spell*> spells_copy = spells;
    for (vector<Spell*>::iterator entity = spells_copy.begin(); entity != spells_copy.end(); entity++) {

      if ((*entity)->IsDeleted() || (*entity) == nullptr)
        continue;

        hasSpell = hasSpell || (*entity)->IsTileHighlightEnabled();

      (*entity)->SetBattleActive(isBattleActive);
      (*entity)->Update(_elapsed);
    }

    vector<Character*> characters_copy = characters;
    for (vector<Character*>::iterator entity = characters_copy.begin(); entity != characters_copy.end(); entity++) {

      if ((*entity)->IsDeleted() || (*entity) == nullptr)
        continue;

      (*entity)->SetBattleActive(isBattleActive);
      (*entity)->Update(_elapsed);
    }

    this->RefreshTexture();
    
    if (state == TileState::BROKEN) {
      cooldown -= 1 * _elapsed;
    }

    if (cooldown <= 0.0f && state == TileState::BROKEN) {
      state = TileState::NORMAL;
    }

  }
  
  void Tile::SetBattleActive(bool state)
  {
    isBattleActive = state;
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

}

