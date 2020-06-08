#include <list>
#include <functional>

#include "bnTile.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnSpell.h"
#include "bnArtifact.h"
#include "bnDefenseFrameStateJudge.h"
#include "bnDefenseRule.h"

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
      team = Team::red;
    }
    else {
      team = Team::blue;
    }

    state = TileState::normal;
    entities = vector<Entity*>();
    setScale(2.f, 2.f);
    width = TILE_WIDTH * getScale().x;
    height = TILE_HEIGHT * getScale().y;
    setOrigin(TILE_WIDTH / 2.0f, TILE_HEIGHT / 2.0f);
    setPosition((width/2.0f) + ((x - 1) * width) + START_X, (height/2.0f) + ((y - 1) * (height - Y_OFFSET)) + START_Y);
    willHighlight = false;
    isTimeFrozen = true;
    isBattleOver = false;
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
    isTimeFrozen = other.isTimeFrozen;
    isBattleOver = other.isBattleOver;
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
    entities = other.entities;
    setScale(2.f, 2.f);
    width = other.width;
    height = other.height;
    animState = other.animState;
    setPosition(((x - 1) * width) + START_X, ((y - 1) * (height - Y_OFFSET)) + START_Y);
    willHighlight = other.willHighlight;
    isTimeFrozen = other.isTimeFrozen;
    isBattleOver = other.isBattleOver;
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
    if (GetTeam() == Team::unknown || GetTeam() != _team) {
      size_t size = FindEntities([this](Entity* in) {
        return in->GetTeam() == GetTeam();
      }).size();

      if (size == 0 && reserved.size() == 0) {
        team = _team;
        
        if(useFlicker) {
          flickerTeamCooldown = flickerTeamCooldownLength;
        }
        else {
          flickerTeamCooldown = 0; // cancel 
          teamCooldown = teamCooldownLength;
        }

        RefreshTexture();
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

    if (_state == TileState::broken) {
      if(characters.size() || reserved.size()) {
        return;
      } else {
        brokenCooldown = brokenCooldownLength;
      }
    }

    if (_state == TileState::cracked && (state == TileState::empty || state == TileState::broken)) {
      return;
    }

    state = _state;
  }

  // Set the right texture based on the team color and state
  void Tile::RefreshTexture() {
    // Hack to toggle between team color without rewriting redundant code
    auto currTeam = team;
    auto otherTeam = (team == Team::unknown) ? Team::unknown : (team == Team::red) ? Team::blue : Team::red;

    auto prevAnimState = animState;

    ((int)(flickerTeamCooldown * 100) % 2 == 0 && flickerTeamCooldown <= flickerTeamCooldownLength) ? currTeam : currTeam = otherTeam;

    if (state == TileState::broken) {
      // Broken tiles flicker when they regen
      animState = ((int)(brokenCooldown * 100) % 2 == 0 && brokenCooldown <= FLICKER) ? std::move(GetAnimState(TileState::normal)) : std::move(GetAnimState(state));
    }
    else {
      animState = std::move(GetAnimState(state));
    }

    if (currTeam == Team::red) {
      setTexture(*red_team_atlas);
    }
    else {
      setTexture(*blue_team_atlas);
    }

    if (prevAnimState != animState) {
      animation.SetAnimation(animState);
      animation << Animator::Mode::Loop;
    }
  }

  bool Tile::IsWalkable() const {
    return (state != TileState::broken && state != TileState::empty && !IsEdgeTile());
  }

  bool Tile::IsCracked() const {
    return state == TileState::cracked;
  }

  bool Tile::IsEdgeTile() const
  {
    return GetX() == 0 || GetX() == 7 || GetY() == 0 || GetY() == 4;
  }

  bool Tile::IsHighlighted() const {
    return willHighlight;
  }

  void Tile::RequestHighlight(Highlight mode)
  {
    highlightMode = mode;
  }

  bool Tile::IsReservedByCharacter()
  {
    return (reserved.size() != 0) && (characters.size() != 0);
  }

  void Tile::AddEntity(Spell & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      spells.push_back(&_entity);
      AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Character & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      characters.push_back(&_entity);
      AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Obstacle & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      characters.push_back(&_entity);
      spells.push_back(&_entity);
      AddEntity(&_entity);
    }
  }

  void Tile::AddEntity(Artifact & _entity)
  {
    if (!ContainsEntity(&_entity)) {
      artifacts.push_back(&_entity);
      AddEntity(&_entity);
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

  bool Tile::RemoveEntityByID(Entity::ID_t ID)
  {
    bool modified = false;

    // This is for queued entities that have not been spawned yet
    // But are requested to be removed on the same frame
    field->TileRequestsRemovalOfQueued(this, ID);

    // If the entity was in the reserved list, remove it
    auto reservedIter = reserved.find(ID);
    if (reservedIter != reserved.end()) { reserved.erase(reservedIter); }

    bool doBreakState = false;

    auto itEnt   = find_if(entities.begin(), entities.end(), [ID](Entity* in) { return in->GetID() == ID; });
    auto itSpell = find_if(spells.begin(), spells.end(), [ID](Entity* in) { return in->GetID() == ID; });
    auto itChar  = find_if(characters.begin(), characters.end(), [ID](Entity* in) { return in->GetID() == ID; });
    auto itArt   = find_if(artifacts.begin(), artifacts.end(), [ID](Entity* in) { return in->GetID() == ID; });

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

      auto tagged = std::find_if(taggedSpells.begin(), taggedSpells.end(), [ID](Entity::ID_t in) { return ID == in; });
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
      SetState(TileState::broken);
      Audio().Play(AudioType::PANEL_CRACK);
    }

    return modified;
  }

  bool Tile::ContainsEntity(const Entity* _entity) const {
    vector<Entity*> copy = entities;
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

  void Tile::Update(float _elapsed) {
    willHighlight = false;
    totalElapsed += _elapsed;

    if (!isTimeFrozen) {
        // LAVA TILES
        elapsedBurnTime -= _elapsed;
    }

    CleanupEntities();

    UpdateSpells(_elapsed);

    ExecuteAllSpellAttacks();

    UpdateArtifacts(_elapsed);

    UpdateCharacters(_elapsed);

    // Update our tile animation and texture
    if (!isTimeFrozen) {
      if (teamCooldown > 0) {
        teamCooldown -= 1.0f * _elapsed;
        if (teamCooldown < 0) teamCooldown = 0;
      }

      if (flickerTeamCooldown > 0) {
        flickerTeamCooldown -= 1.0f * _elapsed;
        if (flickerTeamCooldown < 0) flickerTeamCooldown = 0;
      }

      if (state == TileState::broken) {
        brokenCooldown -= 1.0f * _elapsed;

        if (brokenCooldown < 0) { brokenCooldown = 0; state = TileState::normal; }
      }
    }

    RefreshTexture();

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

    // animation will want to override the sprite's origin. Use setOrigin() to fix this.
    setOrigin(TILE_WIDTH / 2.0f, TILE_HEIGHT / 2.0f);
    highlightMode = Highlight::none;
  }

  void Tile::ToggleTimeFreeze(bool state)
  {
    if (isTimeFrozen == state) return;
    isTimeFrozen = state;

    for (auto&& entity : entities) {
      entity->ToggleTimeFreeze(isTimeFrozen);
    }
  }

  void Tile::BattleStart()
  {
    for (auto&& e : entities) {
        e->OnBattleStart();
    }
    isBattleOver = false;
  }

  void Tile::BattleStop() {
    for (auto&& e : entities) {
        e->OnBattleStop();
    }
    isBattleOver = true;
  }

  void Tile::HandleTileBehaviors(Obstacle* obst) {
    if (!isTimeFrozen || !isBattleOver) {

      // DIRECTIONAL TILES
      auto directional = Direction::none;
      auto notMoving = obst->GetNextTile() == nullptr;

      switch (GetState()) {
      case TileState::directionDown:
        directional = Direction::down;
        break;
      case TileState::directionUp:
        directional = Direction::up;
        break;
      case TileState::directionLeft:
        directional = Direction::left;
        break;
      case TileState::directionRight:
        directional = Direction::right;
        break;
      }

      if (directional != Direction::none) {
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

    if (!(isTimeFrozen || isBattleOver)) {
      // LAVA TILES
      if (!character->HasFloatShoe()) {
        if (GetState() == TileState::poison) {
          if (elapsedBurnTime <= 0) {
            if (character->Hit(Hit::Properties({ 1, Hit::pierce, Element::none, nullptr, Direction::none }))) {
              elapsedBurnTime = burncycle;
            }
          }
        }
        else {
          elapsedBurnTime = 0;
        }

        if (GetState() == TileState::lava && character->GetElement() != Element::fire) {
          if (character->Hit(Hit::Properties({ 50, Hit::pierce | Hit::flinch, Element::fire, nullptr, Direction::none }))) {
            Artifact* explosion = new Explosion(field, GetTeam(), 1);
            field->AddEntity(*explosion, GetX(), GetY());
            SetState(TileState::normal);
          }
        }
      }

      // DIRECTIONAL TILES
      auto directional = Direction::none;

      auto notMoving = character->GetNextTile() == nullptr;

      switch (GetState()) {
      case TileState::directionDown:
        directional = Direction::down;
        break;
      case TileState::directionUp:
        directional = Direction::up;
        break;
      case TileState::directionLeft:
        directional = Direction::left;
        break;
      case TileState::directionRight:
        directional = Direction::right;
        break;
      }

      if (directional != Direction::none) {
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

    for(auto iter = entities.begin(); iter != entities.end(); iter++ ) {
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
    case TileState::broken:
      str = str + "broken";
      break;
    case TileState::cracked:
      str = str + "cracked";
      break;
    case TileState::empty:
      str = str + "empty";
      break;
    case TileState::grass:
      str = str + "grass";
      break;
    case TileState::ice:
      str = str + "ice";
      break;
    case TileState::lava:
      str = str + "lava";
      break;
    case TileState::normal:
      str = str + "normal";
      break;
    case TileState::poison:
      str = str + "poison";
      break;
    case TileState::directionDown:
      str = str + "direction_down";
      break;
    case TileState::directionLeft:
      str = str + "direction_left";
      break;
    case TileState::directionRight:
      str = str + "direction_right";
      break;
    case TileState::directionUp:
      str = str + "direction_up";
      break;
    case TileState::volcano:
      str = str + "volcano";
      break;
    case TileState::holy:
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

  void Tile::CleanupEntities()
  {
    int i = 0;

    // Step through the entity bucket (all entity types)
    while (i < entities.size()) {
      auto ptr = entities[i];

      // If the entity is marked for removal
      if (ptr->WillRemoveLater()) {
        ptr->FreeAllComponents();

        // free memory
        Entity::ID_t ID = ptr->GetID();

        if (RemoveEntityByID(ID)) {
          // TODO: make hash of entity ID to character pointers and then grab character by entity's ID...
          Character* character = dynamic_cast<Character*>(ptr);

          // We only want to know about character deletions since they are the actors in the battle
          if (character) {
            field->CharacterDeletePublisher::Broadcast(*character);
          }

          // Don't track this entity anymore
          field->ForgetEntity(ID);
          delete ptr;
          continue;
        } 
      }  

      i++;
    }
  }

  void Tile::ExecuteAllSpellAttacks()
  {
    // Spells dont cause damage when the battle is over
    if (isBattleOver) return;

      // Now that spells and characters have updated and moved, they are due to check for attack outcomes
      auto characters_copy = characters; // may be modified after hitboxes are resolved

      for (auto it = characters_copy.begin(); it != characters_copy.end(); ++it) {
        // the entity is a character (can be hit) and the team isn't the same
        // we see if it passes defense checks, then call attack

        Character& character = *(*it);
        bool retangible = false;
        DefenseFrameStateJudge judge; // judge for this character's defenses

        for (Entity::ID_t ID : queuedSpells) {
          Spell* spell = dynamic_cast<Spell*>(field->GetEntity(ID));

          if (character.GetID() == spell->GetID()) // Case: prevent obstacles from attacking themselves
            continue;

          bool unknownTeams = character.GetTeam() == Team::unknown;
          unknownTeams = unknownTeams && (spell->GetTeam() == Team::unknown);

          if (character.GetTeam() == spell->GetTeam() && !unknownTeams) // Case: prevent friendly fire
            continue;

          character.DefenseCheck(judge, *spell, DefenseOrder::always);

          bool alreadyTagged = false;

          if (judge.IsDamageBlocked()) {
            alreadyTagged = true;
            // Tag the spell
            // only ignore spells that have already hit something on a tile
            // this is similar to the hitbox being removed in mmbn mechanics
            taggedSpells.push_back(spell->GetID());
          }

          // Collision here means "we are able to hit" 
          // either with a hitbox that can peirce a defense or by tangibility
          auto props = spell->GetHitboxProperties();
          if (!character.HasCollision(props)) continue;

          if (!alreadyTagged) {
            // If not collided by the earlier defense types, tag it now
            // since we have a definite collision
            taggedSpells.push_back(spell->GetID());
          }

          // Retangible flag takes characters out of passthrough status
          retangible = retangible || ((props.flags & Hit::retangible) == Hit::retangible);

          // The spell passed at least one defense check
          character.DefenseCheck(judge, *spell, DefenseOrder::collisionOnly);

          if (!judge.IsDamageBlocked()) {

            // Attack() routine has Hit() which immediately subtracts HP
            // We make sure to apply any tile bonuses at this stage
            if (GetState() == TileState::holy) {
              auto props = spell->GetHitboxProperties();
              props.damage /= 2;
              spell->SetHitboxProperties(props);
            }

            if (isTimeFrozen) {
              auto props = spell->GetHitboxProperties();
              props.flags |= Hit::shake;
              spell->SetHitboxProperties(props);
            }

            spell->Attack(&character);
            spell->SetHitboxProperties(props);
          }

          judge.PrepareForNextAttack();
      } // end each spell loop

      judge.ExecuteAllTriggers();
      if (retangible) character.SetPassthrough(false);
    } // end each character loop

    // empty previous frame queue to be used this current frame
    queuedSpells.clear();
  }

  void Tile::UpdateSpells(const float elapsed)
  {
    vector<Spell*> spells_copy = spells;
    for (vector<Spell*>::iterator entity = spells_copy.begin(); entity != spells_copy.end(); entity++) {
      int request = (int)(*entity)->GetTileHighlightMode();

      if (request > (int)highlightMode) {
        highlightMode = (Highlight)request;
      }

      field->UpdateEntityOnce(*entity, elapsed);
    }
  }

  void Tile::UpdateArtifacts(const float elapsed)
  {
    vector<Artifact*> artifacts_copy = artifacts;
    for (vector<Artifact*>::iterator entity = artifacts_copy.begin(); entity != artifacts_copy.end(); entity++) {
      field->UpdateEntityOnce(*entity, elapsed);
    }
  }

  void Tile::UpdateCharacters(const float elapsed)
  {
    vector<Character*> characters_copy = characters;
    for (vector<Character*>::iterator entity = characters_copy.begin(); entity != characters_copy.end(); entity++) {
      // Allow user input to move them out of tiles if they are frame perfect
      field->UpdateEntityOnce(*entity, elapsed);
      HandleTileBehaviors(*entity);
    }
  }
}