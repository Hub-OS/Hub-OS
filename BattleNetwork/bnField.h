#pragma once
#include <vector>
using std::vector;
#include <iostream>
using std::cout;
using std::endl;

#include "bnTile.h"
#include "bnEntity.h"
#include "bnPlayer.h"
#include "bnMemory.h"

class Field {
public:
  Field(void);
  Field(int _width, int _height);
  ~Field(void);

  int GetWidth() const;
  int GetHeight() const;

  bool GetNextTile(Battle::Tile*& out);

  void AddEntity(Entity* _entity, int x, int y);
  void OwnEntity(Entity* _entity, int x, int y);
  void RemoveEntity(Entity* _entity);

  // TODO: do we need this anymore?
  Battle::Tile* FindEntity(Entity* _entity) const;

  std::vector<Entity*> FindEntities(std::function<bool(Entity* e)> query);

  bool GetNextEntity(Entity*& out, int _depth) const;

  void SetAt(int _x, int _y, Team _team);
  Battle::Tile* GetAt(int _x, int _y) const;

  void Update(float _elapsed);

  void SetBattleActive(bool state);

private:
  bool isBattleActive;
  int width;
  int height;
  vector<vector<Battle::Tile*>> tiles;
  vector<Entity*> entities;
};