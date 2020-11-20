#include "bnInfiniteMap.h"

namespace Overworld {

  InfiniteMap::InfiniteMap(int branchDepth, int numOfCols, int tileWidth, int tileHeight) 
    : Overworld::Map(numOfCols, 0, tileWidth, tileHeight), branchDepth(branchDepth)
  {
    ToggleLighting(false);

    DeleteTiles();

    // One long road
    int row = 0;
    for (int i = 0; i < numOfCols; i++) {
      map.push_back(new Tile(sf::Vector2f((float)i*tileWidth*0.5f, (float)row*tileHeight*0.5f)));
    }

    // Add the arrow at the top
    map.insert(map.begin(), new Tile(LOAD_TEXTURE(MAIN_MENU_ARROW), sf::Vector2f(-(float)((LOAD_TEXTURE(MAIN_MENU_ARROW))->getSize().x*1.25*0.5f), 0)));

    // Make a pointer to the start of the map
    head = map.back();

    // Load NPC animations
    animator = Animator();
    animator << Animator::Mode::Loop;

    progAnimations = Animation("resources/backgrounds/main_menu/prog.animation");
    progAnimations.Reload();

    numbermanAnimations = Animation("resources/backgrounds/main_menu/numberman.animation");
    numbermanAnimations.Reload();
  }

  InfiniteMap::~InfiniteMap()
  {
  }


  void InfiniteMap::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const {
    //std::cout << "map size: " << npcs.size() << "\n";
    for (int i = 0; i < map.size(); i++) {
      sf::Sprite tileSprite(map[i]->GetTexture());
      sf::Vector2f pos = OrthoToIsometric(map[i]->GetPos());
      pos = sf::Vector2f(pos.x * getScale().x, pos.y * getScale().y);

      if (cam) {
        tileSprite.setPosition(pos);

        if (!cam->IsInView(tileSprite)) {
          if (pos.x < 0) {
            if (&(*head) != &(*map[i])) {
              map[i]->Cleanup();
              continue;
            }
          }
        }
      }
    }
    
    Map::DrawTiles(target, states);
  }
  
  void InfiniteMap::Update(double elapsed)
  {
    static float total = 0;
    total += (float)elapsed;

    Map::Update(elapsed);

    std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));

    if (std::max((int)(map.size()-branchDepth), 0) < cols*5) {

      Overworld::Tile* tile = new Tile(sf::Vector2f(head->GetPos().x + (GetTileSize().x*0.5f), head->GetPos().y));
      map.push_back(tile);

      head = tile;

      std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));

      int depth = 0;

      Overworld::Tile* offroad = head;

      int lastDirection = -1;
      int distFromPath = 0;

      while (depth < branchDepth) {
        int randDirection = rand() % 3;

        if (randDirection == 0 && lastDirection == 1)
        continue;

        if (randDirection == 1 && lastDirection == 0)
        continue;

        if (randDirection == 0) {
          distFromPath--;

          offroad = new Tile(sf::Vector2f(offroad->GetPos().x, offroad->GetPos().y + GetTileSize().y));
          map.push_back(offroad);
            
          depth++;
        }
        else if (randDirection == 1) {
          distFromPath++;

          offroad = new Tile(sf::Vector2f(offroad->GetPos().x, offroad->GetPos().y - GetTileSize().y));
          map.push_back(offroad);

          depth++;
        }
        else if (depth > 1) {
          offroad = new Tile(sf::Vector2f(offroad->GetPos().x + (GetTileSize().x * 0.5f), offroad->GetPos().y));
          map.push_back(offroad);

          depth++;
        }

        lastDirection = randDirection;

        std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));
      }
    }
  }
}
