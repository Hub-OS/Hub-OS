#include "bnEntity.h"
#include "bnComponent.h"
#include "bnTile.h"
#include "bnField.h"
#include <Swoosh/Ease.h>

long Entity::numOfIDs = 0;

// First entity ID begins at 1
Entity::Entity()
  : tile(nullptr),
  next(nullptr),
  previous(nullptr),
  field(nullptr),
  previousDirection(Direction::NONE),
  direction(Direction::NONE),
  team(Team::UNKNOWN),
  isBattleActive(false),
  deleted(false),
  passthrough(false),
  ownedByField(false),
  isSliding(false),
  element(Element::NONE),
  tileOffset(sf::Vector2f(0,0)),
  slideTime(sf::milliseconds(100)),
  defaultSlideTime(slideTime),
  elapsedSlideTime(0),
  lastComponentID(0)
{
  this->ID = ++Entity::numOfIDs;
  alpha = 255;
}

// Entity's own the components still attached
// Use FreeComponent() to preserve a component upon entity's deletion
Entity::~Entity() {
  for (int i = 0; i < shared.size(); i++) {
    delete shared[i];
  }

  shared.clear();
}

const bool Entity::IsSuperEffective(Element _other) const {
    switch(this->GetElement()) {
        case Element::AQUA:
            return _other == Element::ELEC;
            break;
        case Element::FIRE:
            return _other == Element::AQUA;
            break;
        case Element::WOOD:
            return _other == Element::FIRE;
            break;
        case Element::ELEC:
            return _other == Element::WOOD;
            break;
    }
    
    return false;
}

void Entity::Update(float _elapsed) {
  // Update all components
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->Update(_elapsed);
  }

  // Do not upate if the entity's current tile pointer is null
  if (_elapsed <= 0 || !tile)
    return;

  // Only slide if we have a valid next tile pointer
  if (isSliding && this->next) {
    elapsedSlideTime += _elapsed;

    // Get a value from 0.0 to 1.0
    float delta = swoosh::ease::linear((float)elapsedSlideTime, slideTime.asSeconds(), 1.0f);

    sf::Vector2f pos = this->slideStartPosition;
    sf::Vector2f tar = this->next->getPosition();

    // Interpolate the sliding position from the start position to the end position
    auto interpol = tar * delta + (pos*(1.0f - delta));
    this->tileOffset = interpol - pos;

    // Once halfway, the mmbn entities switch to the next tile
    // and the slide position offset must be readjusted 
    if (delta >= 0.5f) {
      // conditions of the target tile may change, ensure by the time we switch
      if (this->CanMoveTo(next)) {
        if (tile != next) {
          // Remove the entity from its previous tile pointer 
          previous->RemoveEntityByID(this->GetID());
          
          // Previous tile is now the current tile
          previous = tile;

          // Curent tile is now the next tile
          this->AdoptTile(next);
        }

        // Adjust for the new current tile, begin halfway approaching the current tile
        this->tileOffset = -tar + pos + tileOffset;
      }
      else {
        // Slide back into the origin tile if we can no longer slide to the next tile
        this->next = this->tile;
      }
    } 

    // When delta is 1.0, the slide duration is complete
    if(delta == 1.0f)
    {
      elapsedSlideTime = 0;
      Battle::Tile* prevTile = this->GetTile();
      this->tileOffset = sf::Vector2f(0, 0);

      // If we slide onto an ice block and we don't have float shoe enabled, slide
      if (this->tile->GetState() == TileState::ICE && !this->HasFloatShoe()) {
        // Move again in the same direction as before
        this->Move(this->GetPreviousDirection());

        // calculate our new entity's position
        this->UpdateSlideStartPosition();

        // TODO: shorten with this->GetPreviousDirection() switch
        if (this->previous->GetX() > prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() - 1, this->GetTile()->GetY());
        }
        else if (this->previous->GetX() < prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() + 1, this->GetTile()->GetY());
        }
        else if (this->previous->GetY() < prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() + 1);
        }
        else if (this->previous->GetY() > prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() - 1);
        }

        // If the next tile is not available, not ice, or we are ice element, don't slide
        if (((this->next && this->tile->GetState() != TileState::ICE) 
          || !this->CanMoveTo(next)) 
          || (this->GetElement() == Element::ICE)) {
          // Conditions not met
          isSliding = false;
        }
      }
      else {
        // Invalidate the next tile poiter
        this->next = nullptr;
      }
    }
  }
  else {
    // If we don't have a valid next tile pointer or are not sliding,
    // Keep centered in the current tile with no offset
    this->tileOffset = sf::Vector2f(0, 0);
    isSliding = false;
  }

  // If this entity is flagged for deletion, remove it from its current tile
  if (IsDeleted()) {
    if (tile) {
      tile->RemoveEntityByID(this->GetID());
    }
  }
}

void Entity::SetAlpha(int value)
{
  alpha = value;
  sf::Color c = this->getColor();
  c.a = alpha;

  this->setColor(c);
}


bool Entity::Move(Direction _direction) {
  bool moved = false;

  // Update the entity direction 
  this->direction = _direction;

  Battle::Tile* temp = tile;
  
  // Based on the input direction grab the tile we wish 
  // to move to and check to see if this entity is allowed
  // to move onto it with CanMoveTo() 
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::DOWN) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::RIGHT) {
    if (tile->GetX() + 1 <= static_cast<int>(field->GetWidth())) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }

  // If the next tile pointer is valid and is different from our current tile, we are moving
  if (next && next != tile) {
    this->previousDirection = _direction;

    previous = temp;

    // If we are sliding onto this tile, prevent move callbacks by returning false
    if (isSliding) return false;

    moved = true;
  }
  else {
    this->direction = Direction::NONE;
    isSliding = false;
  }

  return moved;
}

bool Entity::Teleport(int col, int row) {
  bool moved = false;

  Battle::Tile* temp = tile;

  this->direction = Direction::NONE;


  next = field->GetAt(col, row);

  if (!(next && CanMoveTo(next))) {
    next = nullptr;
  }

  if (next) {
    this->previousDirection = Direction::NONE;

    previous = temp;

    moved = true;
  }

  isSliding = false;

  return moved;
}

// Default implementation of CanMoveTo() checks 
// 1) if the tile is walkable
// 2) if not, if the entity can float 
// 3) if the tile is valid and the next tile is the same team
bool Entity::CanMoveTo(Battle::Tile * next)
{
  bool valid = next? (this->HasFloatShoe()? true : next->IsWalkable()) : false;
  return valid && Teammate(next->GetTeam());
}

const long Entity::GetID() const
{
  return this->ID;
}

/** \brief Unkown team entities are friendly to all spaces @see Cubes */
bool Entity::Teammate(Team _team) {
  return (team == Team::UNKNOWN) || (team == _team);
}

void Entity::SetTile(Battle::Tile* _tile) {
  tile = _tile;
}

Battle::Tile* Entity::GetTile() const {
  return tile;
}

const Battle::Tile* Entity::GetNextTile() const {
  return next;
}

void Entity::SlideToTile(bool enabled)
{
  isSliding = enabled;

  // capture potential slide starting position
  this->UpdateSlideStartPosition();
}

const bool Entity::IsSliding() const
{
  return isSliding;
}

void Entity::SetField(Field* _field) {
  field = _field;
}

Field* Entity::GetField() const {
  return field;
}

Team Entity::GetTeam() const {
  return team;
}
void Entity::SetTeam(Team _team) {
  team = _team;
}

void Entity::SetPassthrough(bool state)
{
  passthrough = state;
}

bool Entity::IsPassthrough()
{
  return passthrough;
}

void Entity::SetFloatShoe(bool state)
{
  floatShoe = state;
}

void Entity::SetAirShoe(bool state) {
  airShoe = state;
}

bool Entity::HasFloatShoe()
{
  return floatShoe;
}

bool Entity::HasAirShoe() {
  return airShoe;
}

void Entity::SetDirection(Direction dir) {
  this->direction = dir;
}

Direction Entity::GetDirection()
{
  return direction;
}

Direction Entity::GetPreviousDirection()
{
  return previousDirection;
}

void Entity::Delete()
{
  deleted = true;
  this->FreeAllComponents();
}

bool Entity::IsDeleted() const {
  return deleted;
}

void Entity::SetElement(Element _elem)
{
  this->element = _elem;
}

const Element Entity::GetElement() const
{
  return element;
}

void Entity::AdoptNextTile()
{
  if (next == nullptr) {
    return;
  }

  if (previous != nullptr) {
    previous->RemoveEntityByID(this->GetID());
  }

  this->AdoptTile(next);

  // Slide if the tile we are moving to is ICE
  if (next->GetState() == TileState::ICE && !this->HasFloatShoe()) {
    this->SlideToTile(true);
  }

  // Adopting a tile is the last step in the move procedure
  // Increase the move count
  moveCount++;
}

void Entity::SetBattleActive(bool state)
{
  isBattleActive = state;
}

const bool Entity::IsBattleActive()
{
  return isBattleActive;
}

void Entity::FreeAllComponents()
{
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->FreeOwner();
  }

  shared.clear();
}

void Entity::FreeComponentByID(long ID) {
  for (int i = 0; i < shared.size(); i++) {
    if (shared[i]->GetID() == ID) {
      shared[i]->FreeOwner();
      shared.erase(shared.begin() + i);
      return;
    }
  }
}

Component* Entity::RegisterComponent(Component* c) {
  shared.push_back(c);

  // Newest components appear first in the list for easy referencing
  std::sort(shared.begin(), shared.end(), [](Component* a, Component* b) { return a->GetID() > b->GetID(); });

  return c;
}

void Entity::UpdateSlideStartPosition()
{
  if (tile) {
    slideStartPosition = sf::Vector2f(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
  }
}
