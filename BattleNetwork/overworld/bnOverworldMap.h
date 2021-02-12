#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <memory>

#include "../bnAnimation.h"
#include "../bnSpriteProxyNode.h"

namespace Overworld {
  class SceneBase;

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
  class Map : public sf::Transformable
  {
  public:
    struct Tileset {
      const std::string name;
      const unsigned int firstGid;
      const unsigned int tileCount;
      const sf::Vector2f offset;
      std::shared_ptr<sf::Texture> texture;
      Animation animation;
    };

    struct TileMeta {
      const unsigned int id;
      const unsigned int gid;
      const sf::Vector2f offset;
      Animation animation;
      sf::Sprite sprite;


      TileMeta(unsigned int id, unsigned int gid, sf::Vector2f offset)
        : id(id), gid(gid), offset(offset) {}
    };

    struct Tile {
      unsigned int gid;
      bool flippedHorizontal;
      bool flippedVertical;
      bool rotated;

      Tile(unsigned int gid) {
        // https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
        this->gid = gid << 3 >> 3;
        flippedHorizontal = (gid >> 31 & 1) == 1;
        flippedVertical = (gid >> 30 & 1) == 1;
        rotated = (gid >> 29 & 1) == 1;
      }
    };

    class Layer;

    struct TileObject {
      unsigned int id;
      Tile tile;
      std::string name;
      bool visible;
      sf::Vector2f position;
      sf::Vector2f size;
      float rotation;

      TileObject(unsigned int id, Tile tile) : id(id), tile(tile) {
        visible = true;
        spriteProxy = std::make_shared<SpriteProxyNode>();
      }

      TileObject(unsigned int id, unsigned int gid) : TileObject(id, Tile(gid)) {}

    private:
      std::shared_ptr<SpriteProxyNode> spriteProxy;

      friend class Map;
      friend class Layer;
    };

    class Layer {
    public:
      Tile& GetTile(int x, int y);
      Tile& GetTile(float x, float y);
      Tile& SetTile(int x, int y, Tile tile);
      Tile& SetTile(int x, int y, unsigned int gid);
      Tile& SetTile(float x, float y, unsigned int gid);

      TileObject& GetTileObject(unsigned int id);
      const std::vector<TileObject>& GetTileObjects();
      void AddTileObject(TileObject tileObject);

    private:
      Layer(size_t cols, size_t rows);

      size_t cols, rows;
      std::vector<Tile> tiles;
      std::vector<TileObject> tileObjects;
      std::vector<std::shared_ptr<SpriteProxyNode>> spriteProxiesForAddition;

      friend class Map;
    };

    /**
     * \brief Simple constructor
     */
    Map(size_t cols, size_t rows, int tileWidth, int tileHeight);

    /**
     * @brief Updates tile animations and tile objects
     * @param elapsed in seconds
     */
    void Update(SceneBase& scene, double elapsed);

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
     * @brief Transforms an ortho vector into an isometric vector
     * @param ortho position in orthographic space
     * @return vector in isometric space
     */
    const sf::Vector2f OrthoToIsometric(const sf::Vector2f& ortho) const;

    /**
     * @brief Returns tile dimensions as a vector
     * @return const sf::Vector2i(tileWidth, tileHeight)
     */
    const sf::Vector2i GetTileSize() const;

    const std::string& GetName() const;
    const std::string& GetBackgroundName() const;
    const std::string& GetSongPath() const;
    void SetName(const std::string& name);
    void SetBackgroundName(const std::string& name);
    void SetSongPath(const std::string& path);
    const unsigned GetCols() const;
    const unsigned GetRows() const;
    unsigned int GetTileCount();
    std::unique_ptr<TileMeta>& GetTileMeta(unsigned int tileGid);
    std::shared_ptr<Tileset> GetTileset(std::string name);
    std::shared_ptr<Tileset> GetTileset(unsigned int tileGid);
    void SetTileset(unsigned int tileGid, unsigned int tileId, std::shared_ptr<Tileset> tileset);
    size_t GetLayerCount() const;
    Layer& GetLayer(size_t index);
    Layer& AddLayer();

  protected:
    unsigned cols{}, rows{}; /*!< map is made out of Cols x Rows tiles */
    int tileWidth{}, tileHeight{}; /*!< tile dimensions */
    std::string name, backgroundName, songPath;
    std::vector<Layer> layers;
    std::vector<std::shared_ptr<Tileset>> tileToTilesetMap;
    std::unordered_map<std::string, std::shared_ptr<Tileset>> tilesets;
    std::vector<std::unique_ptr<TileMeta>> tileMetas;

    /**
     * @brief Transforms an iso vector into an orthographic vector
     * @param iso position in isometric space
     * @return vector in orthographic space
     */
    const sf::Vector2f IsoToOrthogonal(const sf::Vector2f& iso) const;
  };
}