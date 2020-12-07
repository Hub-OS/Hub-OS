#include "bnField.h"
#include "bnObstacle.h"
#include "bnCharacter.h"
#include "bnSpell.h"
#include "bnArtifact.h"
#include "bnTextureResourceManager.h"

constexpr auto TILE_ANIMATION_PATH = "resources/tiles/tiles.animation";

Field::Field(int _width, int _height)
  : width(_width),
  height(_height),
  pending(),
  revealCounterFrames(false),
  tiles(vector<vector<Battle::Tile*>>())
  {
  ResourceHandle handle;

  // Moved tile resource acquisition to field so we only them once for all tiles
  Animation a(TILE_ANIMATION_PATH);
  a.Reload();
  a << Animator::Mode::Loop;

  auto t_a_b = handle.Textures().GetTexture(TextureType::TILE_ATLAS_BLUE);
  auto t_a_r = handle.Textures().GetTexture(TextureType::TILE_ATLAS_RED);

  for (int y = 0; y < _height+2; y++) {
    vector<Battle::Tile*> row = vector<Battle::Tile*>();
    for (int x = 0; x < _width+2; x++) {
      Battle::Tile* tile = new Battle::Tile(x, y);
      tile->SetField(this);
      tile->animation = a;
      tile->blue_team_atlas = t_a_b;
      tile->red_team_atlas = t_a_r;
      tile->useParentShader = true;
      row.push_back(tile);
    }
    tiles.push_back(row);
  }

#ifdef ONB_DEBUG
  // DEBUGGING
  // invisible tiles surround the arena for some entities to slide off of
  for (int i = 0; i < _width + 2; i++) {
    tiles[0][i]->setColor(sf::Color(255, 255, 255, 50));
  }

  for (int i = 0; i < _width + 2; i++) {
    tiles[_height + 1][i]->setColor(sf::Color(255, 255, 255, 50));
  }

  for (int i = 1; i < _height + 1; i++) {
    tiles[i][0]->setColor(sf::Color(255, 255, 255, 50));
    tiles[i][_width + 1]->setColor(sf::Color(255, 255, 255, 50));

  }
#endif

  isTimeFrozen = true;
  isBattleActive = false;
  isUpdating = false;
}

Field::~Field() {
  for (size_t i = 0; i < tiles.size(); i++) {
    for (size_t j = 0; j < tiles[i].size(); j++) {
      delete tiles[i][j];
    }
    tiles[i].clear();
  }
  tiles.clear();
}

int Field::GetWidth() const {
  return width;
}

int Field::GetHeight() const {
  return height;
}

std::vector<Battle::Tile*> Field::FindTiles(std::function<bool(Battle::Tile* t)> query)
{
  std::vector<Battle::Tile*> res;
  
  for(int i = 0; i < tiles.size(); i++) {
    for(int j = 0; j < tiles[i].size(); j++) {
      if(query(tiles[i][j])) {
          res.push_back(tiles[i][j]);
      }
    }
  }
    
  return res;
}

Field::AddEntityStatus Field::AddEntity(Character & character, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, character ));
    return Field::AddEntityStatus::queued;
  }

  character.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    character.AdoptTile(tile);
    allEntityHash.insert(std::make_pair(character.GetID(), &character));
    return Field::AddEntityStatus::added;
  }
  else {
    delete &character;
  }

  return Field::AddEntityStatus::deleted;
}

Field::AddEntityStatus Field::AddEntity(Character & character, Battle::Tile & dest)
{
  return AddEntity(character, dest.GetX(), dest.GetY());
}


Field::AddEntityStatus Field::AddEntity(Spell & spell, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, spell ));
    return Field::AddEntityStatus::queued;
  }

  spell.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    spell.AdoptTile(tile);
    allEntityHash.insert(std::make_pair(spell.GetID(), &spell));
    return Field::AddEntityStatus::added;
  }
  else {
    delete &spell;
  }

  return Field::AddEntityStatus::deleted;
}

Field::AddEntityStatus Field::AddEntity(Spell & spell, Battle::Tile & dest)
{
  return AddEntity(spell, dest.GetX(), dest.GetY());
}

Field::AddEntityStatus Field::AddEntity(Obstacle & obst, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, obst));
    return Field::AddEntityStatus::queued;
  }

  obst.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    obst.AdoptTile(tile);
    allEntityHash.insert(std::make_pair(obst.GetID(), &obst));
    return Field::AddEntityStatus::added;
  }
  else {
    delete &obst;
  }

  return Field::AddEntityStatus::deleted;
}

Field::AddEntityStatus Field::AddEntity(Obstacle & obst, Battle::Tile & dest)
{
  return AddEntity(obst, dest.GetX(), dest.GetY());
}

Field::AddEntityStatus Field::AddEntity(Artifact & art, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, art ));
    return Field::AddEntityStatus::queued;
  }

  art.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    art.AdoptTile(tile);
    allEntityHash.insert(std::make_pair(art.GetID(), &art));
    return Field::AddEntityStatus::added;
  }
  else {
    delete &art;
  }

  return Field::AddEntityStatus::deleted;
}

Field::AddEntityStatus Field::AddEntity(Artifact & art, Battle::Tile & dest)
{
  return AddEntity(art, dest.GetX(), dest.GetY());
}

std::vector<Entity*> Field::FindEntities(std::function<bool(Entity* e)> query)
{
  std::vector<Entity*> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);

      std::vector<Entity*> found = tile->FindEntities(query);
      res.reserve(res.size() + found.size()); // preallocate memory
      res.insert(res.end(), found.begin(), found.end());
    }
  }

  return res;
}

std::vector<const Entity*> Field::FindEntities(std::function<bool(Entity* e)> query) const
{
  std::vector<const Entity*> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);

      std::vector<Entity*> found = tile->FindEntities(query);
      res.reserve(res.size() + found.size()); // preallocate memory
      res.insert(res.end(), found.begin(), found.end());
    }
  }

  return res;
}

void Field::SetAt(int _x, int _y, Team _team) {
  if (_x < 0 || _x > 7) return;
  if (_y < 0 || _y > 4) return;

  tiles[_y][_x]->SetTeam(_team);
}

Battle::Tile* Field::GetAt(int _x, int _y) const {
  if (_x < 0 || _x > 7) return nullptr;
  if (_y < 0 || _y > 4) return nullptr;

  return tiles[_y][_x];
}

void Field::Update(float _elapsed) {
  // This is a state flag that decides if entities added this update tick will be
  // put into a pending queue bucket or added directly onto the field
  isUpdating = true;

  int entityCount = 0;

  // These are the columns that an entity was found on
  int redTeamFarCol = 0; // from blue's perspective, 7 is the farthest - begin at the first (0 col) index and increment
  int blueTeamFarCol = 7; // from red's perspective, 0  is the farthest - begin at the first (7th col) index and decrement

  // tile cols to check to restore team state
  std::list<int> backToRed;
  std::list<int> backToBlue;

  float syncBlueTeamCooldown = 0;
  float syncRedTeamCooldown = 0;

  for (int i = 0; i < tiles.size(); i++) {
      for (int j = 0; j < tiles[i].size(); j++) {
          tiles[i][j]->Update(_elapsed);
      }
  }

  for (int i = 0; i < tiles.size(); i++) {
      for (int j = 0; j < tiles[i].size(); j++) {
          auto&& t = tiles[i][j];

          // How far has entity of either moved across the map?
          // This check will help prevent trapping moving characters 
          // when their tiles' team type resets
          for(auto&& it = t->characters.begin(); it != t->characters.end(); it++) {
            Team team = (*it)->GetTeam();
            if (team == Team::red) { redTeamFarCol = std::max(redTeamFarCol, j); }
            else if(team == Team::blue) { blueTeamFarCol = std::min(blueTeamFarCol, j); }
          }

          if(j <= 3) {
            // This tile was originally red
            if(t->GetTeam() == Team::blue) {
              syncRedTeamCooldown = std::max(syncRedTeamCooldown, t->teamCooldown);

              if(t->teamCooldown <= 0) {
                backToRed.insert(backToRed.begin(), j);
              }
            }
          } else{
            // This tile was originally blue
            if(t->GetTeam() == Team::red) {
              syncBlueTeamCooldown = std::max(syncBlueTeamCooldown, t->flickerTeamCooldown);

              if(t->teamCooldown <= 0) {
                backToBlue.insert(backToBlue.begin(), j);
              }
            }
          }

          // now that the loop for this tile is over
          // and it has been updated, we calculate how many entities remain
          // on the field
          entityCount += (int)tiles[i][j]->GetEntityCount();
        }
  }

    // plan: Restore column team states not just a single tile
    // col must be relatively ahead of the furthest character of the same team
    // e.g. red team characters must be behind the col row
    //      blue team characters must be after the col row
    //      otherwise we risk trapping characters in a striped battle field

    // stolen blue tiles
    for(auto&& p : backToBlue) {
        if (p > redTeamFarCol && syncBlueTeamCooldown <= 0.0f) {
            for (int i = 0; i < tiles.size(); i++) {
                tiles[i][p]->SetTeam(Team::blue, true);
            }
        }
        else {
            // resync
            for (int i = 0; i < tiles.size(); i++) {
                tiles[i][p]->flickerTeamCooldown = Battle::Tile::flickerTeamCooldownLength;
            }
        }
    }

    backToBlue.clear();

    // stolen red tiles
    for(auto&& p : backToRed) {
        if (p < blueTeamFarCol && syncRedTeamCooldown <= 0.0f) {
            for (int i = 0; i < tiles.size(); i++) {
                tiles[i][p]->SetTeam(Team::red, true);
            }
        }
        else {
            // resync
            for (int i = 0; i < tiles.size(); i++) {
                tiles[i][p]->flickerTeamCooldown = Battle::Tile::flickerTeamCooldownLength;
            }
        }
    }

  backToRed.clear();

  // Now that updating is complete any entities being added to the field will be added directly
  isUpdating = false;

  short combatEvaluationIteration = BN_MAX_COMBAT_EVALUATION_STEPS;
  while(HasPendingEntities() && combatEvaluationIteration > 0) {
      // This may force battle steps to evaluate again
      SpawnPendingEntities();

      // Apply new spells into this frame's combat resolution
      for (int i = 0; i < tiles.size(); i++) {
          for (int j = 0; j < tiles[i].size(); j++) {
              tiles[i][j]->ExecuteAllSpellAttacks();
          }
      }

      combatEvaluationIteration--;
  }

  updatedEntities.clear();
}

void Field::ToggleTimeFreeze(bool state)
{
  if (isTimeFrozen == state) return;

  isTimeFrozen = state;

  for (int i = 0; i < tiles.size(); i++) {
      for (int j = 0; j < tiles[i].size(); j++) {
          tiles[i][j]->ToggleTimeFreeze(isTimeFrozen);
      }
  }
}

void Field::RequestBattleStart()
{
    isBattleActive = true;

    for (int i = 0; i < tiles.size(); i++) {
        for (int j = 0; j < tiles[i].size(); j++) {
            tiles[i][j]->BattleStart();
        }
    }
}

void Field::RequestBattleStop()
{
    isBattleActive = false;

    for (int i = 0; i < tiles.size(); i++) {
        for (int j = 0; j < tiles[i].size(); j++) {
            tiles[i][j]->BattleStop();
        }
    }
}

void Field::TileRequestsRemovalOfQueued(Battle::Tile* tile, Entity::ID_t ID)
{
  auto q = pending.begin();
  while(q != pending.end()) {
    if (q->x == tile->GetX() && q->y == tile->GetY()) {
      if (q->ID == ID) {
        q = pending.erase(q);
        allEntityHash.erase(ID);
        break;
      }
    }

    q++;
  }
}

void Field::SpawnPendingEntities()
{
    
    while (pending.size()) {
        auto next = pending.back();
        pending.pop_back();

        switch (next.entity_type) {
        case queueBucket::type::artifact:
            if (AddEntity(*next.data.artifact, next.x, next.y) == Field::AddEntityStatus::added) {
                next.data.artifact->Update(0);
            }
            break;
        case queueBucket::type::character:
            if (AddEntity(*next.data.character, next.x, next.y) == Field::AddEntityStatus::added) {
                next.data.character->Update(0);
            }
            break;
        case queueBucket::type::obstacle:
            if (AddEntity(*next.data.obstacle, next.x, next.y) == Field::AddEntityStatus::added) {
                next.data.obstacle->Update(0);
            }
            break;
        case queueBucket::type::spell:
            if (AddEntity(*next.data.spell, next.x, next.y) == Field::AddEntityStatus::added) {
                next.data.spell->Update(0);
            }
            break;
        }
    }
}

const bool Field::HasPendingEntities() const
{
    return pending.size();
}

void Field::UpdateEntityOnce(Entity * entity, const float elapsed)
{
    if(entity == nullptr || updatedEntities.find(entity->GetID()) != updatedEntities.end())
        return;

    entity->Update(elapsed);
    updatedEntities.insert(std::make_pair(entity->GetID(), (void*)0));
}

void Field::ForgetEntity(Entity::ID_t ID)
{
    allEntityHash.erase(ID);
}

Entity * Field::GetEntity(Entity::ID_t ID)
{
    return allEntityHash[ID];
}

void Field::RevealCounterFrames(bool enabled)
{
  this->revealCounterFrames = enabled;
}

const bool Field::DoesRevealCounterFrames() const
{
  return this->revealCounterFrames;
}

Field::queueBucket::queueBucket(int x, int y, Character& d) : x(x), y(y), entity_type(Field::queueBucket::type::character)
{
  data.character = &d;
  ID = d.GetID();
}

Field::queueBucket::queueBucket(int x, int y, Obstacle& d) : x(x), y(y), entity_type(Field::queueBucket::type::obstacle)
{
  data.obstacle = &d;
  ID = d.GetID();
}

Field::queueBucket::queueBucket(int x, int y, Artifact& d) : x(x), y(y), entity_type(Field::queueBucket::type::artifact)
{
  data.artifact = &d;
  ID = d.GetID();
}

Field::queueBucket::queueBucket(int x, int y, Spell& d) : x(x), y(y), entity_type(Field::queueBucket::type::spell)
{
  data.spell = &d;
  ID = d.GetID();
}
