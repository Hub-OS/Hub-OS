#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <sstream>

#include "bnOverworldLight.h"
#include "../bnSpriteProxyNode.h"
#include "../bnTextureResourceManager.h"
#include "../bnCamera.h"


namespace Overworld {
/*! \brief Incredibly simple overworld map class.
 * 
 * This generates a WxH isometric map. 
 * 
 * It sorts all sprites by Y and gives illusion of depth
 * 
 * If something it outside of the camera view, it is not drawn
 * 
 * The map also supports psuedo lighting by multiplying sprites
 * by the light color. Limit light sources because this is slow.
 */
  class Map : public sf::Drawable, public sf::Transformable, public ResourceHandle

  {
  public:
    struct Tile {
      size_t ID{}; //!< ID of 0 should always be empty
      bool solid{ true }; //!< Default are non-moveable spaces
      std::string token;
    };

    class Tileset {
    public:
      struct Item {
        sf::Sprite sprite;
      };

      const sf::Sprite& Graphic(size_t ID) const;

      void Register(size_t ID, const sf::Sprite& sprite);

      const size_t size() const;

    private:
      std::map<size_t, Item> idToSpriteHash;
    };

  protected:
    struct MapSprite {
      SpriteProxyNode* node{ nullptr };
      int layer{ 0 };

    };
    Tile** tiles{ nullptr };
    Tileset tileset{};
    std::vector<Overworld::Light*> lights; /*!< light sources */
    std::vector<MapSprite> sprites; /*!< other sprites in the scene */
    
    bool enableLighting{ false }; /*!< if true, enables light shading */

    unsigned cols{}, rows{}; /*!< map is made out of Cols x Rows tiles */
    int tileWidth{}, tileHeight{}; /*!< tile dimensions */
    Camera* cam{ nullptr }; /*!< camera */
    std::string name;

    /**
     * @brief Transforms an ortho vector into an isometric vector
     * @param ortho position in orthographic space
     * @return vector in isometric space
     */
    const sf::Vector2f OrthoToIsometric(const sf::Vector2f& ortho) const;
    
    /**
     * @brief Transforms an iso vector into an orthographic vector
     * @param iso position in isometric space
     * @return vector in orthographic space
     */
    const sf::Vector2f IsoToOrthogonal(const sf::Vector2f& iso) const;
    
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

    public:
    /**
     * \brief Simple constructor
     */
    Map();

    void Load(Tileset tilset, Tile** tiles, unsigned cols, unsigned rows);

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
     * @brief Transforms a point in-world to screen cordinates
     * @param screen vector from world
     * @return screen coordinates
     */
    const sf::Vector2f WorldToScreen(sf::Vector2f screen) const;

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
    void AddSprite(SpriteProxyNode* _sprite, int layer);
    
    /**
     * @brief Remove a sprite
     * @param _sprite
     */
    void RemoveSprite(const SpriteProxyNode* _sprite);

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

    const size_t GetTilesetItemCount() const;

    /**
     * @brief Returns a tile at the the pos
     * @param pos position in cartesian space
     * @return Map::Tile if valid position otherwise an empty tile is returned
     */
    const Map::Tile GetTileAt(const sf::Vector2f& pos) const;

    void SetTileAt(const sf::Vector2f& pos, const Tile& newTile);

    const std::vector<sf::Vector2f> FindToken(const std::string& token);

    static const std::pair<bool, Map::Tile**> LoadFromFile(Map& map, const std::string& path);
    static const std::pair<bool, Map::Tile**> LoadFromStream(Map& map, std::istringstream& stream);

    std::pair<unsigned, unsigned> OrthoToRowCol(const sf::Vector2f& ortho) const;
    std::pair<unsigned, unsigned> IsoToRowCol(const sf::Vector2f& iso) const;
    std::pair<unsigned, unsigned> PixelToRowCol(const sf::Vector2i& px) const;

    const std::string& GetName() const;
    void SetName(const std::string& name);

    const unsigned GetCols() const;
    const unsigned GetRows() const;
  };
}