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
  tiles(vector<vector<Battle::Tile*>>())
  {
  // Moved tile resource acquisition to field so we only them once for all tiles
  Animation a(TILE_ANIMATION_PATH);
  a.Reload();
  a << Animator::Mode::Loop;

  auto t_a_b = TEXTURES.GetTexture(TextureType::TILE_ATLAS_BLUE);
  auto t_a_r = TEXTURES.GetTexture(TextureType::TILE_ATLAS_RED);

  for (int y = 0; y < _height+2; y++) {
    vector<Battle::Tile*> row = vector<Battle::Tile*>();
    for (int x = 0; x < _width+2; x++) {
      Battle::Tile* tile = new Battle::Tile(x, y);
      tile->SetField(this);
      tile->animation = a;
      tile->blue_team_atlas = t_a_b;
      tile->red_team_atlas = t_a_r;
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

void Field::AddEntity(Character & character, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, character ));
    return;
  }

  character.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    character.AdoptTile(tile);
  }
  else {
    delete &character;
  }
}

void Field::AddEntity(Character & character, Battle::Tile & dest)
{
  AddEntity(character, dest.GetX(), dest.GetY());
}


void Field::AddEntity(Spell & spell, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, spell ));
    return;
  }

  spell.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    spell.AdoptTile(tile);
  }
  else {
    delete &spell;
  }
}

void Field::AddEntity(Spell & spell, Battle::Tile & dest)
{
  AddEntity(spell, dest.GetX(), dest.GetY());
}

void Field::AddEntity(Obstacle & obst, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, obst));
    return;
  }

  obst.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    obst.AdoptTile(tile);
  }
  else {
    delete &obst;
  }
}

void Field::AddEntity(Obstacle & obst, Battle::Tile & dest)
{
  AddEntity(obst, dest.GetX(), dest.GetY());
}

void Field::AddEntity(Artifact & art, int x, int y)
{
  if (isUpdating) {
    pending.push_back(queueBucket(x, y, art ));
    return;
  }

  art.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    art.AdoptTile(tile);
  }
  else {
    delete &art;
  }
}

void Field::AddEntity(Artifact & art, Battle::Tile & dest)
{
  AddEntity(art, dest.GetX(), dest.GetY());
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
  while (pending.size() && isBattleActive) {
    auto next = pending.back();
    pending.pop_back();

    switch (next.entity_type) {
    case queueBucket::type::artifact:
      this->AddEntity(*next.data.artifact, next.x, next.y);
      break;
    case queueBucket::type::character:
      this->AddEntity(*next.data.character, next.x, next.y);
      break;
    case queueBucket::type::obstacle:
      this->AddEntity(*next.data.obstacle, next.x, next.y);
      break;
    case queueBucket::type::spell:
      this->AddEntity(*next.data.spell, next.x, next.y);
      break;
    }
  }

  // This is a state flag that decides if entities added this update tick will be
  // put into a pending queue bucket or added directly onto the field
  this->isUpdating = true;

  int entityCount = 0;

  // These are the columns that an entity was found on
  int redTeamFarCol = 0; // from blue's perspective, 7 is the farthest - begin at the first (0 col) index and increment
  int blueTeamFarCol = 7; // from red's perspective, 0  is the farthest - begin at the first (7th col) index and decrement

  // tile cols to check to restore team state
  std::map<int, bool> backToRed;
  std::map<int, bool> backToBlue;

  float syncBlueTeamCooldown = 0;
  float syncRedTeamCooldown = 0;

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->Update(_elapsed);

      auto&& t = tiles[i][j];

      // How far has entity of either moved across the map?
      // This check will help prevent trapping moving characters 
      // when their tiles' team type resets
      for(auto it = t->characters.begin(); it != t->characters.end(); it++) {
        Team team = (*it)->GetTeam();
        if (team == Team::RED) { redTeamFarCol = std::max(redTeamFarCol, j); }
        else if(team == Team::BLUE) { blueTeamFarCol = std::min(blueTeamFarCol, j); }
      }

      if(j <= 3) {
        // This tile was originally red
        if(t->GetTeam() == Team::BLUE) {
          if(t->teamCooldown <= 0) {
            backToRed.insert(std::make_pair(j, true));
          }
        }
      } else{
        // This tile was originally blue
        if(t->GetTeam() == Team::RED) {
          if(t->teamCooldown <= 0) {
            backToBlue.insert(std::make_pair(j, true));
          }
        }
      }

      // now that the loop for this tile is over
      // and it has been updated, we set the battle active flag
      // to its latest state and calculate how many entities remain
      // on the field
      entityCount += (int)tiles[i][j]->GetEntityCount();
    }
  }

  // plan: Restore column team states not just a single tile
  // col must be ahead of the furthest character of the same team
  // e.g. red team characters must be behind the col row
  //      blue team characters must be ahead the col row
  // otherwise we risk trapping characters in a striped battle field

  // stolen blue tiles
  for(auto&& p : backToBlue) {
    if (p.first <= redTeamFarCol) continue;

    Logger::Logf("redTeamFarCol found %i, while p.first is %i", redTeamFarCol, p.first);

    // these are the tiles (rows) in the column
    for(int i = 0; i < tiles.size(); i++) {
      // sync stolen blue tiles together
      syncBlueTeamCooldown = std::max(syncBlueTeamCooldown, tiles[i][p.first]->flickerTeamCooldown);
    }
    if (syncBlueTeamCooldown <= 0.0f) {
        for (int i = 0; i < tiles.size(); i++) {
            tiles[i][p.first]->SetTeam(Team::BLUE, true);
        }
    }
  }

  backToBlue.clear();

  // stolen red tiles
  for(auto&& p : backToRed) {
    if (p.first >= blueTeamFarCol) continue;

    // these are the tiles (rows) in the column
    for(int i = 0; i < tiles.size(); i++) {
      // sync stolen red tiles together
      syncRedTeamCooldown = std::max(syncRedTeamCooldown, tiles[i][p.first]->flickerTeamCooldown);
    }

    if (syncRedTeamCooldown <= 0.0f) {
        for (int i = 0; i < tiles.size(); i++) {
            tiles[i][p.first]->SetTeam(Team::RED, true);
        }
    }
  }

  backToRed.clear();

  // Now that updating is complete any entities being added to the field will be added directly
  this->isUpdating = false;
}

void Field::SetBattleActive(bool state)
{
  if (isBattleActive == state) return;

  isBattleActive = state;

  for (int i = 0; i < tiles.size(); i++) {
      for (int j = 0; j < tiles[i].size(); j++) {
          tiles[i][j]->SetBattleActive(isBattleActive);
      }
  }
}

void Field::TileRequestsRemovalOfQueued(Battle::Tile* tile, long ID)
{
  auto q = pending.begin();
  while(q != pending.end()) {
    if (q->x == tile->GetX() && q->y == tile->GetY()) {
      if (q->ID == ID) {
        q = pending.erase(q);
      }
    }

    q++;
  }
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
