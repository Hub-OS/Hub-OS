#pragma once
#include <SFML/Graphics.hpp>
using sf::RectangleShape;
using sf::Sprite;
#include <vector>
using std::vector;
#include <algorithm>
using std::find;

#include "bnTeam.h"
class Entity;
class Spell;
class Field;
#include "bnMemory.h"
#include "bnTextureType.h"
#include "bnTileState.h"

namespace Battle {
  class Tile : public Sprite {
  public:
    Tile(void);
    Tile(int _x, int _y);
    ~Tile(void);

    Tile(const Tile& rhs);
    Tile& operator=(const Tile& other);

    const TileState GetState() const;

    void SetField(Field* _field);

    int GetX() const;
    int GetY() const;

    Team GetTeam() const;
    void SetTeam(Team _team);

    float GetWidth() const;
    float GetHeight() const;

    void SetState(TileState _state);
    void RefreshTexture();

    bool IsWalkable() const;
    bool IsCracked() const;

    bool IsHighlighted() const;

    void AddEntity(Entity* _entity);
    void RemoveEntity(Entity* _entity);
    bool ContainsEntity(Entity* _entity) const;
    template<class Type> bool ContainsEntityType();
    void AffectEntities(Spell* caller);
    bool GetNextEntity(Entity*& out) const;

    void Update(float _elapsed);

    vector<Entity*> entities;
  private:
    int x;
    int y;
    Team team;
    TileState state;
    TextureType textureType;
    float elapsed;

    float width;
    float height;
    Field* field;
    float cooldown;
    float cooldownLength;
    float slideCooldown;
    float slideCooldownLength;
    float lavaBurnCooldown;
    float lavaBurnCooldownLength;
    bool hasSpell;
  };


  template<class Type>
  bool Tile::ContainsEntityType() {
    // std::cout << "len of entities is: " << entities.size() << "\n";

    for (vector<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it) {
      if (dynamic_cast<Type*>(*it) != nullptr) {
        return true;
      }
    }

    return false;
  }
}