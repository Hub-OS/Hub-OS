#include "bnTile.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnSpell.h"
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

  Tile::~Tile(void) {
    entities.clear();
  }

  void Tile::SetField(Field* _field) {
    field = _field;
  }

  int Tile::GetX() const {
    return x;
  }

  int Tile::GetY() const {
    return y;
  }

  Team Tile::GetTeam() const {
    return team;
  }

  void Tile::SetTeam(Team _team) {
    team = _team;
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
  }

  const TileState Tile::GetState() const {
    return state;
  }

  void Tile::SetState(TileState _state) {
    if (_state == TileState::CRACKED && (state == TileState::EMPTY || state == TileState::BROKEN)) {
      return;
    }

    if (_state == TileState::CRACKED) {
      cooldown = cooldownLength;
    }

    state = _state;
  }

  void Tile::RefreshTexture() {
    sf::Vector2f test = getPosition();
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

  void Tile::AddEntity(Entity* _entity) {
    if (!ContainsEntity(_entity)) {
      _entity->SetTile(this);
      entities.push_back(_entity);

      // Sort by layer (draw order)
      // e.g. layer 0 draws first so it must be last in the draw list
      std::sort(entities.begin(), entities.end(), [](Entity* a, Entity* b) { return a->GetLayer() > b->GetLayer(); });
    }
  }

  void Tile::RemoveEntity(Entity* _entity) {
    auto it = find(entities.begin(), entities.end(), _entity);
    if (it != entities.end()) {
      if (IsCracked() && !(*it)->HasFloatShoe()) {
        state = TileState::BROKEN;
        AUDIO.Play(AudioType::PANEL_CRACK);
      }
      auto tagged = std::find_if(taggedSpells.begin(), taggedSpells.end(), [&_entity](int ID) { return ID == _entity->GetID(); });
      if (tagged != taggedSpells.end()) {
        taggedSpells.erase(tagged);
      }

      entities.erase(it);
    }
  }

  bool Tile::ContainsEntity(Entity* _entity) const {
    vector<Entity*> copy = this->entities;
    return find(copy.begin(), copy.end(), _entity) != copy.end();
  }

  void Tile::AffectEntities(Spell* caller) {
    if (std::find_if(taggedSpells.begin(), taggedSpells.end(), [&caller](int ID) { return ID == caller->GetID(); }) != taggedSpells.end())
      return;

    for (auto it = entities.begin(); it != entities.end(); ++it) {
      if (*it == nullptr) {
        it = entities.erase(it);
        continue;
      }
    }

    // Spells dont cause damage when the battle is over
    if (this->isBattleActive) {
      for (auto it = entities.begin(); it != entities.end(); ++it) {
        if (*it == nullptr || *it == caller)
          continue;

        if (!(*it)->IsPassthrough() && dynamic_cast<Character*>(*it)) {
          caller->Attack(*it);
        }
      }
    }

    taggedSpells.push_back(caller->GetID());
  }

  bool Tile::GetNextEntity(Entity*& out) const {
    static int x = 0;
    while (x < (int)this->entities.size()) {
      out = this->entities.at(x);
      x++;
      return true;
    }
    x = 0;
    return false;
  }

  void Tile::Update(float _elapsed) {
    hasSpell = false;

    vector<Entity*> copies = entities;
    for (vector<Entity*>::iterator entity = copies.begin(); entity != copies.end(); entity++) {

      if ((*entity) == nullptr || (*entity)->IsDeleted())
        continue;

      if (!hasSpell) {
        Spell* isSpell = dynamic_cast<Spell*>(*entity);

        if (isSpell)
          hasSpell = isSpell->IsTileHighlightEnabled();
      }

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
}