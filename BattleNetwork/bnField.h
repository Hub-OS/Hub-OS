#pragma once
#include <vector>
#include <iostream>

using std::vector;
using std::cout;
using std::endl;

#include "bnTile.h"
#include "bnEntity.h"
#include "bnPlayer.h"
#include "bnMemory.h"

class Character;
class Spell;
class Obstacle;
class Artifact;

class Field {
public:
  Field();
  Field(int _width, int _height);
  ~Field();

  int GetWidth() const;
  int GetHeight() const;

  bool GetNextTile(Battle::Tile*& out);

  void AddEntity(Character& character, int x, int y);
  void AddEntity(Spell& spell, int x, int y);
  void AddEntity(Obstacle& obst, int x, int y);
  void AddEntity(Artifact& art, int x, int y);

  //void RemoveEntityByID(int ID);

  // TODO: do we need this anymore?
  //Battle::Tile* FindEntity(Entity* _entity) const;

  std::vector<Entity*> FindEntities(std::function<bool(Entity* e)> query);

  // bool GetNextEntity(Entity*& out, int _depth) const;

  void SetAt(int _x, int _y, Team _team);
  Battle::Tile* GetAt(int _x, int _y) const;

  void Update(float _elapsed);

  void SetBattleActive(bool state);

private:
  bool isBattleActive;
  int width;
  int height;
  vector<vector<Battle::Tile*>> tiles;
  //vector<Entity*> entities;
};