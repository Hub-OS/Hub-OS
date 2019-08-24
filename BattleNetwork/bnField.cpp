#include "bnField.h"

#include "bnObstacle.h"
#include "bnArtifact.h"
#include "bnTextureResourceManager.h"

constexpr auto TILE_ANIMATION_PATH = "resources/tiles/tiles.animation";

Field::Field(int _width, int _height)
  : width(_width),
  height(_height),
  tiles(vector<vector<Battle::Tile*>>())
  {
  // Moved tile resource acquisition to field so we only them once for all tiles
  Animation a(TILE_ANIMATION_PATH);
  a.Reload();
  a << Animate::Mode::Loop;

  auto t_a_b = TEXTURES.GetTexture(TextureType::TILE_ATLAS_BLUE);
  auto t_a_r = TEXTURES.GetTexture(TextureType::TILE_ATLAS_RED);

  for (int y = 0; y < _height; y++) {
    vector<Battle::Tile*> row = vector<Battle::Tile*>();
    for (int x = 0; x < _width; x++) {
      Battle::Tile* tile = new Battle::Tile(x + 1, y + 1);
      tile->SetField(this);
      tile->animation = a;
      tile->blue_team_atlas = t_a_b;
      tile->red_team_atlas = t_a_r;
      row.push_back(tile);
    }
    tiles.push_back(row);
  }

  isBattleActive = false;
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
  character.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    character.AdoptTile(tile);
  }
}


void Field::AddEntity(Spell & spell, int x, int y)
{
  spell.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    spell.AdoptTile(tile);
  }
}

void Field::AddEntity(Obstacle & obst, int x, int y)
{
  obst.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    obst.AdoptTile(tile);
  }
}

void Field::AddEntity(Artifact & art, int x, int y)
{
  art.SetField(this);

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    art.AdoptTile(tile);
  }
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
  tiles[_y - 1][_x - 1]->SetTeam(_team);
}

Battle::Tile* Field::GetAt(int _x, int _y) const {
  if (_x <= 0 || _x > 6) return nullptr;
  if (_y <= 0 || _y > 3) return nullptr;

  return tiles[_y - 1][_x - 1];
}

void Field::Update(float _elapsed) {
  int entityCount = 0;

  int redTeamFarCol = 0; // from red's perspective, 5 is the farthest - begin at the first (0 col) index and increment
  int blueTeamFarCol = 5; // from blue's perspective, 0  is the farthest - begin at the first (5th col) index and increment

  // tile cols to check to restore team state
  std::map<int, bool> backToRed;
  std::map<int, bool> backToBlue;

  float syncBlueTeamCooldown = 0;
  float syncRedTeamCooldown = 0;

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      tiles[y][x]->Update(_elapsed);

      auto t = tiles[y][x];

      for(auto it = t->characters.begin(); it != t->characters.end(); it++) {
        if((*it)->GetTeam() == Team::RED && redTeamFarCol <= x) { redTeamFarCol = x; }
        else if((*it)->GetTeam() == Team::BLUE && blueTeamFarCol >= x) { blueTeamFarCol = x; }
      }

      if(x <= 2) {
        // sync stolen red tiles together
        syncRedTeamCooldown = std::max(syncRedTeamCooldown, t->flickerTeamCooldown);

        // tiles should be red
        if(t->GetTeam() == Team::BLUE) {
          if(t->teamCooldown <= 0) {
            backToRed.insert(std::make_pair(x, true));
          }
        }
      } else{
        // sync stolen blue tiles together
        syncBlueTeamCooldown = std::max(syncBlueTeamCooldown, t->flickerTeamCooldown);

        if(t->GetTeam() == Team::RED) {
          if(t->teamCooldown <= 0) {
            backToBlue.insert(std::make_pair(x, true));
          }
        }
      }

      entityCount += (int)tiles[y][x]->GetEntityCount();
      tiles[y][x]->SetBattleActive(isBattleActive);
    }
  }

  /*if (syncRedTeamCooldown != 0.f && syncBlueTeamCooldown != 0.f) {
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        auto t = tiles[y][x];
        if (x <= 2) {
          t->flickerTeamCooldown = syncRedTeamCooldown;
        }
        else {
          t->flickerTeamCooldown = syncBlueTeamCooldown;
        }
      }
    }
  }*/

  // Restore **whole** col team states not just a single tile...
  // col must be ahead of the furthest character of the same team
  // e.g. red team characters must be behind the col row
  //      blue team characters must be ahead the col row
  // otherwise we risk trapping characters in a striped battle field
  for(auto p : backToBlue) {
    if (p.first <= redTeamFarCol) continue;

    for(int i = 0; i < 3; i++) {
      tiles[i][p.first]->SetTeam(Team::BLUE, true);
    }
  }

  backToBlue.clear();

  for(auto p : backToRed) {
    if (p.first >= blueTeamFarCol) continue;

    for(int i = 0; i < 3; i++) {
      tiles[i][p.first]->SetTeam(Team::RED, true);
    }
  }

  backToRed.clear();
}

void Field::SetBattleActive(bool state)
{
  isBattleActive = state;
}
