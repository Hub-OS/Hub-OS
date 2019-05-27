#include "bnField.h"
<<<<<<< HEAD
#include "bnMemory.h"
#include "bnObstacle.h"
#include "bnArtifact.h"

Field::Field(void) {
=======
#include "bnObstacle.h"
#include "bnArtifact.h"

Field::Field() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  isBattleActive = false;
}

Field::Field(int _width, int _height)
  : width(_width),
  height(_height),
  tiles(vector<vector<Battle::Tile*>>())
  {
  for (int y = 0; y < _height; y++) {
    vector<Battle::Tile*> row = vector<Battle::Tile*>();
    for (int x = 0; x < _width; x++) {
      Battle::Tile* tile = new Battle::Tile(x + 1, y + 1);
      tile->SetField(this);
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
<<<<<<< HEAD
  }
}

/*void Field::RemoveEntityByID(int ID)
{
}*/

std::vector<Entity*> Field::FindEntities(std::function<bool(Entity* e)> query)
{
  std::vector<Entity*> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);

      Entity* next = nullptr;
      while (tile->GetNextEntity(next)) {
        if (query(next)) {
          res.push_back(next);
        }
      }
    }
  }

  return res;
}

/*bool Field::GetNextEntity(Entity*& out, int _depth) const {
  static int i = 0;
  for (i; i < (int)entities.size(); i++) {
    if (entities.at(i)->GetTile()->GetY() == _depth) {
      out = entities.at(i++);
      return true;
=======
  }
}

std::vector<Entity*> Field::FindEntities(std::function<bool(Entity* e)> query)
{
  std::vector<Entity*> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);
      
      std::vector<Entity*> found = tile->FindEntities(query);
      std::vector<Entity*> merged = res;
      merged.reserve(res.size() + found.size()); // preallocate memory
      merged.insert(merged.end(), res.begin(), res.end());
      merged.insert(merged.end(), found.begin(), found.end());
    
      res = merged;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    }
  }

<<<<<<< HEAD
  return false;
}*/
=======
  return res;
}
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

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

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      tiles[y][x]->Update(_elapsed);
<<<<<<< HEAD
      entityCount += (int)tiles[y][x]->entities.size();
=======
      entityCount += (int)tiles[y][x]->GetEntityCount();
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      tiles[y][x]->SetBattleActive(isBattleActive);
    }
  }
}

void Field::SetBattleActive(bool state)
{
  isBattleActive = state;
}
