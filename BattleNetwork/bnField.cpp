#include "bnField.h"
#include "bnObstacle.h"
#include "bnCharacter.h"
#include "bnSpell.h"
#include "bnArtifact.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"

constexpr auto TILE_ANIMATION_PATH = "resources/tiles/tiles.animation";

Field::Field(int _width, int _height) :
  width(_width),
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
      tile->SetupGraphics(t_a_r, t_a_b, a);

      if (x <= 3) {
        tile->SetTeam(Team::red);
        tile->SetFacing(Direction::right);
      }
      else {
        tile->SetTeam(Team::blue);
        tile->SetFacing(Direction::left);
      }

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

void Field::SetScene(const Scene* scene)
{
  this->scene = scene;

  for (auto& row : tiles) {
    for (auto& tile : row) {
      tile->SetField(shared_from_this());
    }
  }
}

int Field::GetWidth() const {
  return width;
}

int Field::GetHeight() const {
  return height;
}

Field::NotifyID_t Field::CallbackOnDelete(Entity::ID_t target, const std::function<void(std::shared_ptr<Entity>)>& callback)
{
  auto iter = entityDeleteObservers.find(target);
  Field::NotifyID_t ID = nextID;
  nextID++;

  if (iter != entityDeleteObservers.end()) {
    DeleteObserver dobs;
    dobs.ID = ID;
    dobs.callback1 = callback;
    iter->second.emplace_back(std::move(dobs));
  }
  else {
    // We do not have a bucket for this target entity
    entityDeleteObservers.insert(std::make_pair(target, std::vector<DeleteObserver>()));

    DeleteObserver dobs;
    dobs.ID = ID;
    dobs.callback1 = callback;
    entityDeleteObservers[target].emplace_back(std::move(dobs));
  }

  notify2TargetHash.insert(std::make_pair(ID, target));
  return ID;
}

Field::NotifyID_t Field::NotifyOnDelete(
  Entity::ID_t target,
  Entity::ID_t observer,
  const std::function<void(std::shared_ptr<Entity>, std::shared_ptr<Entity>)>& callback
) {
  auto iter = entityDeleteObservers.find(target);
  NotifyID_t ID = {};

  if (iter != entityDeleteObservers.end()) {
    for (auto& elem : iter->second) {
      if (elem.observer == observer) {
        ID = elem.ID;
        break;
      }
    }

    // If we reached this line, we did not find a matching observer
    // Add one
    ID = nextID;
    nextID++;
    DeleteObserver dobs;
    dobs.ID = ID;
    dobs.observer = observer;
    dobs.callback2 = callback;
    iter->second.emplace_back(std::move(dobs));
  }
  else {
    // We do not have a bucket for this target entity
    entityDeleteObservers.insert(std::make_pair(target, std::vector<DeleteObserver>()));

    DeleteObserver dobs;
    dobs.ID = nextID;
    dobs.observer = observer;
    dobs.callback2 = callback;
    entityDeleteObservers[target].emplace_back(std::move(dobs));
    ID = nextID;
    nextID++;

    // add notify ID lookup for this target
    notify2TargetHash.insert(std::make_pair(ID, target));
  }

  return ID;
}

void Field::DropNotifier(NotifyID_t notifier)
{
  auto iter = notify2TargetHash.find(notifier);
  if (iter != notify2TargetHash.end()) {
    Entity::ID_t targetID = iter->second;
    auto listIter = entityDeleteObservers.find(targetID);

    if (listIter != entityDeleteObservers.end()) {
      for (auto deleteIter = listIter->second.begin(); deleteIter != listIter->second.end(); deleteIter++) {
        if (deleteIter->ID == notifier) {
          listIter->second.erase(deleteIter);

          if (listIter->second.empty()) {
            entityDeleteObservers.erase(listIter);
            notify2TargetHash.erase(notifier);
          }
          return;
        }
      }
    }
  }
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

Field::AddEntityStatus Field::AddEntity(std::shared_ptr<Entity> entity, int x, int y)
{
  if (!entity->HasInit()) {
    entity->Init();
  }

  if (isUpdating) {
    pending.push_back(queueBucket(x, y, entity));
    return Field::AddEntityStatus::queued;
  }

  entity->SetField(shared_from_this());

  Battle::Tile* tile = GetAt(x, y);

  if (tile) {
    if (!entity->IsMoving()) {
      entity->setPosition(tile->getPosition());
    }

    tile->AddEntity(entity);
    allEntityHash.insert(std::make_pair(entity->GetID(), entity));
    return Field::AddEntityStatus::added;
  }

  return Field::AddEntityStatus::deleted;
}

Field::AddEntityStatus Field::AddEntity(std::shared_ptr<Entity> entity, Battle::Tile & dest)
{
  return AddEntity(entity, dest.GetX(), dest.GetY());
}

std::vector<std::shared_ptr<Entity>> Field::FindEntities(std::function<bool(std::shared_ptr<Entity> e)> query) const
{
  std::vector<std::shared_ptr<Entity>> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);

      std::vector<std::shared_ptr<Entity>> found = tile->FindEntities(query);
      res.reserve(res.size() + found.size()); // preallocate memory
      res.insert(res.end(), found.begin(), found.end());
    }
  }

  return res;
}

std::vector<std::shared_ptr<Character>> Field::FindCharacters(std::function<bool(std::shared_ptr<Character> e)> query) const
{
  std::vector<std::shared_ptr<Character>> res;

  for (int y = 1; y <= height; y++) {
    for (int x = 1; x <= width; x++) {
      Battle::Tile* tile = GetAt(x, y);

      std::vector<std::shared_ptr<Character>> found = tile->FindCharacters(query);
      res.reserve(res.size() + found.size()); // preallocate memory
      res.insert(res.end(), found.begin(), found.end());
    }
  }

  return res;
}

std::vector<std::shared_ptr<Character>> Field::FindNearestCharacters(const std::shared_ptr<Character> test, std::function<bool(std::shared_ptr<Character> e)> filter) const
{
  auto list = this->FindCharacters(filter);

  std::sort(list.begin(), list.end(), [test](const std::shared_ptr<Character> first, const std::shared_ptr<Character> next) {
    auto& t0 = *test->GetTile();
    auto& t1 = *first->GetTile();
    auto& t2 = *next->GetTile();

    unsigned int t1_dist = std::abs(t0.Distance(t1));
    unsigned int t2_dist = std::abs(t0.Distance(t2));
    return (t1_dist < t2_dist);
  });

  return list;
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

void Field::Update(double _elapsed) {
  // This is a state flag that decides if entities added this update tick will be
  // put into a pending queue bucket or added directly onto the field
  isUpdating = true;

  int entityCount = 0;

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->CleanupEntities(*this);
      tiles[i][j]->UpdateSpells(*this, _elapsed);
    }
  }

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->ExecuteAllAttacks(*this);
    }
  }

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->UpdateArtifacts(*this, _elapsed);
      // TODO: tiles[i][j]->UpdateObjects(_elapsed);
    }
  }

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->Update(*this, _elapsed);
    }
  }

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      tiles[i][j]->UpdateCharacters(*this, _elapsed);
    }
  }

  std::set<int> charCol = {}; // columns with characters in them
  std::set<int> syncCol = {}; // synchronize columns
  std::set<int> restCol = {}; // restore columns

  for (int i = 0; i < tiles.size(); i++) {
    for (int j = 0; j < tiles[i].size(); j++) {
      auto& t = tiles[i][j];
      if (t->teamCooldown > 0) {
        syncCol.insert(syncCol.begin(), j);
      }
      else if(t->GetTeam() != t->ogTeam){
        restCol.insert(restCol.begin(), j);
      }

      if (t->characters.size() || t->reserved.size()) {
        charCol.insert(charCol.begin(), j);
      }
      
      // now that the loop for this tile is over
      // and it has been updated, we calculate how many entities remain
      // on the field
      entityCount += (int)t->GetEntityCount();
    }
  }

  // any columns with a stolen tile do not need to revert
  for (auto syncIter = syncCol.begin(); syncIter != syncCol.end(); syncIter++) {
    auto iter = restCol.find(*syncIter);

    if (iter != restCol.end()) {
      restCol.erase(iter);
    }
  }

  // any columns with a character in them from a different team do not revert
  for (auto charIter = charCol.begin(); charIter != charCol.end(); charIter++) {
    for (size_t i = 1; i <= GetHeight(); i++) {
      auto& t = tiles[i][*charIter];

      auto matchIter = std::find_if(t->characters.begin(), t->characters.end(), 
        [team = t->ogTeam](std::shared_ptr<Character> in) { return !in->Teammate(team); });

      // We found a character on a different team than us
      if (matchIter != t->characters.end()) {
        // erase this column from the revert bucket
        // and break early
        auto iter = restCol.find(*charIter);

        if (iter != restCol.end()) {
          restCol.erase(iter);
        }

        // Follow this column and prevent other columns that it points to from reverting
        Battle::Tile* next = t + t->ogFacing;
        Team tileTeam = t->ogTeam;

        // Follow the facing directions to prevent all other teammate tiles
        // from reverting since a character has made it further across our side
        while (next && next->ogFacing != Direction::none) {
          if (next->ogTeam != tileTeam)
            break;

          auto iter = restCol.find(next->GetX());

          if (iter != restCol.end()) {
            restCol.erase(iter);
          }

          next = next + next->ogFacing;
        }

        break;
      }
    } // end column
    // begin the next column
  }

  // revert strategy for tiles:
  for (auto col : restCol) {
    for (size_t i = 1; i <= GetHeight(); i++) {
      auto& t = tiles[i][col];

      t->SetTeam(t->ogTeam, true);
      t->SetFacing(t->ogFacing);
    }
  }

  // Finally, sync stolen tiles with their corresponding columns
  for (auto col : syncCol) {
    double maxTimer = 0.0;
    for (size_t i = 1; i <= GetHeight(); i++) {
      maxTimer = std::max(maxTimer, tiles[i][col]->teamCooldown);
    }
    for (size_t i = 1; i <= GetHeight(); i++) {
      auto& t = tiles[i][col];

      if (t->GetTeam() != t->ogTeam) {
        t->teamCooldown = maxTimer;
      }
    }
  }

  // Now that updating is complete any entities being added to the field will be added directly
  isUpdating = false;

  short combatEvaluationIteration = BN_MAX_COMBAT_EVALUATION_STEPS;
  while(HasPendingEntities() && combatEvaluationIteration > 0) {
    // This may force battle steps to evaluate again
    SpawnPendingEntities();

    // Apply new spells into this frame's combat resolution
    for (int i = 0; i < tiles.size(); i++) {
      for (int j = 0; j < tiles[i].size(); j++) {
        tiles[i][j]->ExecuteAllAttacks(*this);
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
        // DeallocEntity(ID); // TODO: Check if commenting this out is a problem. This is a nasty side effect and really oughtta not be here...
        break;
      }
    }

    q++;
  }
}

void Field::SpawnPendingEntities()
{
  while (pending.size()) {
    auto& next = pending.back();

    if (AddEntity(next.entity, next.x, next.y) == Field::AddEntityStatus::added) {
      next.entity->Update(0);
    }

    pending.pop_back();
  }
}

const bool Field::HasPendingEntities() const
{
  return pending.size();
}

void Field::UpdateEntityOnce(std::shared_ptr<Entity> entity, const double elapsed)
{
  if(entity == nullptr || updatedEntities.find(entity->GetID()) != updatedEntities.end())
      return;

  entity->Update(elapsed);
  updatedEntities.insert(std::make_pair(entity->GetID(), (void*)0));
}

void Field::ForgetEntity(Entity::ID_t ID)
{
  auto entityIter = allEntityHash.find(ID);
  if (entityIter != allEntityHash.end()) {
    std::shared_ptr<Entity> target = entityIter->second;

    auto deleteIter = entityDeleteObservers.find(ID);

    if (deleteIter != entityDeleteObservers.end()) {
      auto& deleteObservers = deleteIter->second;

      for (auto& d : deleteObservers) {
        if (d.observer.has_value()) {
          if (std::shared_ptr<Entity> observer = this->GetEntity(d.observer.value())) {
            d.callback2(target, observer);
          }
        }
        else {
          d.callback1(target);
        }
      }
    }
  }

  allEntityHash.erase(ID);
  entityDeleteObservers.erase(ID);
}

void Field::DeallocEntity(Entity::ID_t ID)
{
  auto iter = allEntityHash.find(ID);

  if (iter != allEntityHash.end()) {
    auto& entity = iter->second;
    entity->GetTile()->RemoveEntityByID(ID);
    ForgetEntity(ID);
    // todo: break entity shared_ptr cycles
  }
}

std::shared_ptr<Entity> Field::GetEntity(Entity::ID_t ID)
{
  return allEntityHash[ID];
}

std::shared_ptr<Character> Field::GetCharacter(Entity::ID_t ID)
{
  return std::dynamic_pointer_cast<Character>(GetEntity(ID));
}

void Field::RevealCounterFrames(bool enabled)
{
  this->revealCounterFrames = enabled;
}

const bool Field::DoesRevealCounterFrames() const
{
  return this->revealCounterFrames;
}

Field::queueBucket::queueBucket(int x, int y, std::shared_ptr<Entity> e) : x(x), y(y), entity(e)
{
  ID = e->GetID();
}
