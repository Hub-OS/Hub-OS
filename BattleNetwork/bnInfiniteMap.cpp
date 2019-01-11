#include "bnInfiniteMap.h"

namespace Overworld {

  InfiniteMap::InfiniteMap(int branchDepth, int numOfCols, int tileWidth, int tileHeight) 
    : Overworld::Map(numOfCols, 0, tileWidth, tileHeight)
  {
    this->branchDepth = branchDepth;

    ToggleLighting(false);

    DeleteTiles();

    // One long road
    int row = 0;
    for (int i = 0; i < numOfCols; i++) {
      map.push_back(new Tile(sf::Vector2f((float)i*tileWidth, (float)row*tileHeight)));
    }

    // Add the arrow at the top
    map.insert(map.begin(), new Tile(&LOAD_TEXTURE(MAIN_MENU_ARROW), sf::Vector2f(-(float)((LOAD_TEXTURE(MAIN_MENU_ARROW)).getSize().x*1.25), 0)));

    // Make a pointer to the start of the map
    head = map.back();

    // Load NPC animations
    animator = Animate();
    animator << Animate::Mode(Animate::Mode::Loop);

    progAnimations = Animation("resources/backgrounds/main_menu/prog.animation");
    progAnimations.Reload();

    numbermanAnimations = Animation("resources/backgrounds/main_menu/numberman.animation");
    numbermanAnimations.Reload();
  }

  InfiniteMap::~InfiniteMap()
  {
  }


  void InfiniteMap::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const {
    //std::cout << "map size: " << map.size() << "\n";
    for (int i = 0; i < map.size(); i++) {
      sf::Sprite tileSprite(map[i]->GetTexture());


      tileSprite.setScale(2.0f, 2.0f);
      sf::Vector2f pos = map[i]->GetPos();

      if (cam) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;

        pos = OrthoToIsometric(pos*2.0f);

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

    /*

      pos = map[i]->GetPos();

      if (cam) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;
      }

      pos = OrthoToIsometric(pos*2.0f);

      if (cam) {
        tileSprite.setPosition(pos);

        if (cam->IsInView(tileSprite)) {
          target.draw(tileSprite, states);
        }
      }
    } */
  }
  
  void InfiniteMap::Update(double elapsed)
  {
    static float total = 0;
    total += (float)elapsed;

    for (auto npc : npcs) {
      if (!npc)
        break;

      switch (npc->type) {
      case NPCType::MR_PROG_DOWN:
      {
        animator(total, npc->sprite, progAnimations.GetFrameList("PROG_DR"));

        sf::Vector2f newPos = npc->sprite.getPosition();
        newPos.x += 1 * (float)elapsed;
        npc->sprite.setPosition(newPos);
      }
      break;
      case NPCType::MR_PROG_LEFT:
      {
        animator(total, npc->sprite, progAnimations.GetFrameList("PROG_UR"));
      }
      break;
      case NPCType::MR_PROG_RIGHT:
      {
        animator(total, npc->sprite, progAnimations.GetFrameList("PROG_DR"));

        // sf::Vector2f newPos = npc->sprite.getPosition();
        // newPos.x += 10 * elapsed;
        // npc->sprite.setPosition(newPos);
      }
      break;
      case NPCType::MR_PROG_UP:
      {
        animator(total, npc->sprite, progAnimations.GetFrameList("PROG_UR"));

      }
      break;
      case NPCType::MR_PROG_FIRE:
      {
        animator(total, npc->sprite, progAnimations.GetFrameList("PROG_DR_FIRE"));
      }
      break;
      case NPCType::NUMBERMAN_DANCE:
      {
        animator(total, npc->sprite, numbermanAnimations.GetFrameList("NUMBERMAN_DANCE"));
      }
      break;
      case NPCType::NUMBERMAN_DOWN:
      {
        animator(total, npc->sprite, numbermanAnimations.GetFrameList("NUMBERMAN_IDLE_DR"));
      }
      break;
      }
    }

    Map::Update(elapsed);

    std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));

    if (std::max((int)(map.size()-branchDepth), 0) < cols*50) {

      Overworld::Tile* tile = new Tile(sf::Vector2f(head->GetPos().x + this->GetTileSize().x, head->GetPos().y));
      map.push_back(tile);

      head = tile;

      std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));

      int randBranch = rand() % 100;

      int depth = 0;

      if (randBranch < 100) {
        Overworld::Tile* offroad = head;

        int lastDirection = -1;
        int distFromPath = 0;

        while (depth < branchDepth) {
          int randDirection = rand() % 3;
          int randSpawnNPC = rand() % 10;

          if (randDirection == 0 && lastDirection == 1)
            continue;

          if (randDirection == 1 && lastDirection == 0)
            continue;

          if (randDirection == 0) {
            offroad = new Tile(sf::Vector2f(offroad->GetPos().x, offroad->GetPos().y + this->GetTileSize().y));
            map.push_back(offroad);

            depth++;
          }
          else if (randDirection == 1) {
            offroad = new Tile(sf::Vector2f(offroad->GetPos().x, offroad->GetPos().y - this->GetTileSize().y));
            map.push_back(offroad);

            if (randSpawnNPC == 0 && distFromPath != 0) {

              if (rand() % 10 > 3) {
                npcs.push_back(new NPC{ sf::Sprite(LOAD_TEXTURE(OW_MR_PROG)), NPCType::MR_PROG_UP });
              }

              sf::Vector2f pos = offroad->GetPos();
              pos += sf::Vector2f(45, 0);

              npcs.back()->sprite.setPosition(pos);
              this->AddSprite(&npcs.back()->sprite);
            }

            depth++;
          }
          else if (depth > 1) {
            offroad = new Tile(sf::Vector2f(offroad->GetPos().x + this->GetTileSize().x, offroad->GetPos().y));
            map.push_back(offroad);

            if (randSpawnNPC == 0 && distFromPath != 0) {
              npcs.push_back(new NPC { sf::Sprite(LOAD_TEXTURE(OW_MR_PROG)), NPCType::MR_PROG_RIGHT });

              sf::Vector2f pos = offroad->GetPos();
              pos += sf::Vector2f(45, 0);

              npcs.back()->sprite.setPosition(pos);
              this->AddSprite(&npcs.back()->sprite);
            }

            depth++;
          }

          /*int randLight = rand() % 100;

          sf::Vector2f pos = offroad->GetPos();
          pos += sf::Vector2f(45, 0);

          if (randLight < 10) {
            sf::Uint8 lighten = 180;
            sf::Uint8 r = rand() % (256 - lighten);
            sf::Uint8 g = rand() % (256 - lighten);
            sf::Uint8 b = rand() % (256 - lighten);
            double radius = (double)(rand() % 120);

            if(randLight < 3)
              this->AddLight(new Light(pos, sf::Color(r + lighten, 0, r + lighten, 255), radius));
            else if (randLight < 6)
              this->AddLight(new Light(pos, sf::Color(0, g + lighten, b + lighten, 255), radius));
            else
              this->AddLight(new Light(pos, sf::Color(0, 0, b + lighten, 255), radius));
          }

          if (randDirection != 2) {
            distFromPath = distFromPath + (randDirection ? -randDirection : 1);
          }*/

          lastDirection = randDirection;

          std::sort(map.begin(), map.end(), InfiniteMap::TileComparitor(this));
        }
      }
    }
  }
}