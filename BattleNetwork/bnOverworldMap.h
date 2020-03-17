#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "bnTextureResourceManager.h"
#include "bnCamera.h"
#include "bnOverworldLight.h"
#include "bnTile.h"

namespace Overworld {
  /*! \brief Structure to hold tile data */
  class Tile {
    std::shared_ptr<sf::Texture> texture;
    sf::Vector2f pos;

    bool cleanup; /*!< flag to remove tile */

    /**
     * @brief Randomly choose a tile color to add variation
     */
    void LoadTexture() { 
      int randTex = rand() % 100;

      if (randTex > 80) {
        texture = TEXTURES.GetTexture(TextureType::MAIN_MENU_OW2);
      }
      else {
        texture = TEXTURES.GetTexture(TextureType::MAIN_MENU_OW);
      }
    }

  public:
    Tile() { pos = sf::Vector2f(0, 0); LoadTexture(); cleanup = false;  }
    Tile(const Tile& rhs) { texture = rhs.texture; pos = rhs.pos;  cleanup = false; }

    Tile(std::shared_ptr<sf::Texture> _texture, sf::Vector2f pos = sf::Vector2f()) : pos(pos) { texture = _texture; cleanup = false; }
    Tile(sf::Vector2f pos) : pos(pos) { LoadTexture(); cleanup = false;}
    ~Tile() { ; }
    const sf::Vector2f GetPos() const { return pos; }
    const sf::Texture& GetTexture() { return *texture; }

    void Cleanup() {
      cleanup = true;
    }

    bool ShouldRemove() {
      return cleanup;
    }

  };

/*! \brief Incredibly hackey overworld class. Read more.
 * 
 * This generates a WxH isometric map. 
 * 
 * It sorts all sprites by Y and gives illusion of depth
 * 
 * If something it outside of the camera view, it is not drawn
 * Tiles randomly choose texture
 * 
 * The map also supports psuedo lighting by multiplying sprites
 * by the light color. Limit light sources because this is slow.
 * 
 * \warning This is poorly written and far from optimized. This should be
 * redesigned and not used as a base for real overworld maps.
 */
  class Map : public sf::Drawable
  {
  protected:
    std::vector<Tile*> map; /*!< tiles */
    std::vector<Overworld::Light*> lights; /*!< light sources */
    std::vector<const sf::Sprite*> sprites; /*!< other sprites in the scene */
    
    bool enableLighting; /*!< if true, enables light shading */

    int cols, rows; /*!< map is made out of Cols x Rows tiles */
    int tileWidth, tileHeight; /*!< tile dimensions */
    Camera* cam; /*!< camera */

    /**
     * @brief Transforms an ortho vector into an isometric vector
     * @param ortho position in orthographic space
     * @return vector in isometric space
     */
    const sf::Vector2f OrthoToIsometric(sf::Vector2f ortho) const;
    
    /**
     * @brief Transforms an iso vector into an orthographic vector
     * @param iso position in isometric space
     * @return vector in orthographic space
     */
    const sf::Vector2f IsoToOrthogonal(sf::Vector2f iso) const;

    /**
     * @brief Cleanup allocated tiles
     */
    void DeleteTiles();
    
    /**
     * @brief Draws the tiles 
     * @param target
     * @param states
     */
    virtual void DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const ;
    
    /**
     * @brief Draws the sprites in the scene
     * @param target
     * @param states
     */
    virtual void DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const;

    /**
     * @class TileComparitor
     * @brief Used to sort tiles by isometric Y value
     */
    struct TileComparitor
    {
    private:
      Overworld::Map* map;

    public:
      TileComparitor(Map* _map) {
        map = _map;
      }

      inline bool operator() (const Overworld::Tile* a, const Overworld::Tile* b)
      {
        sf::Vector2f isoA = map->OrthoToIsometric(a->GetPos());
        sf::Vector2f isoB = map->OrthoToIsometric(b->GetPos());

        return (isoA.y < isoB.y);
      }
    };

    public:
    /**
     * \brief Builds a map of Cols x Rows with tiles of Width x Height areas
     */
    Map(int numOfCols, int numOfRows, int tileWidth, int tileHeight);

    /**
     * @brief toggle lighting
     * @param state if true, lighting is enabled. If false, lighting is disabled
     */
    void ToggleLighting(bool state = true);

    /**
     * @brief Transforms a point on the screen to in-world coordinates
     * @param screen vector from screen
     * @return world coordinates
     */
    const sf::Vector2f ScreenToWorld(sf::Vector2f screen) const;
 
    /**
     * @brief Deletes tiles and lights.
     */
    ~Map();

    void SetCamera(Camera* _camera);

    /**
     * @brief Add a light
     * @param _light
     */
    void AddLight(Overworld::Light* _light);
    
    /**
     * @brief Add a sprite
     * @param _sprite
     */
    void AddSprite(const sf::Sprite* _sprite);
    
    /**
     * @brief Remove a sprite
     * @param _sprite
     */
    void RemoveSprite(const sf::Sprite * _sprite);

    /**
     * @brief Sorts tiles. Deletes ones marked for removal.
     * @param elapsed in seconds
     */
    virtual void Update(double elapsed);
    
    /**
     * @brief Draws the tiles first and sprites on top with lighting.
     * @param target
     * @param states
     */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    /**
     * @brief Returns tile dimensions as a vector
     * @return const sf::Vector2i(tileWidth, tileHeight)
     */
    const sf::Vector2i GetTileSize() const;
  };
}