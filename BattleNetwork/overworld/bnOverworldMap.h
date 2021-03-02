#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <memory>

#include "bnOverworldSprite.h"
#include "bnOverworldTile.h"
#include "bnOverworldObject.h"
#include "../bnAnimation.h"

namespace Overworld {
  class SceneBase;
  class ShapeObject;
  class TileObject;

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
    class Layer {
    public:
      // ShapeObject can not be copied due to storing a unique_ptr
      // delete the copy constructor to forward this property
      Layer(const Layer&) = delete;
      Layer(Layer&&) = default;

      Tile& GetTile(int x, int y);
      Tile& GetTile(float x, float y);
      Tile& SetTile(int x, int y, Tile tile);
      Tile& SetTile(int x, int y, unsigned int gid);
      Tile& SetTile(float x, float y, unsigned int gid);

      TileObject& GetTileObject(unsigned int id);
      const std::vector<TileObject>& GetTileObjects();
      void AddTileObject(TileObject tileObject);

      ShapeObject& GetShapeObject(unsigned int id);
      const std::vector<ShapeObject>& GetShapeObjects();
      void AddShapeObject(ShapeObject shapeObject);

    private:
      Layer(size_t cols, size_t rows);

      size_t cols, rows;
      std::vector<Tile> tiles;
      std::vector<ShapeObject> shapeObjects;
      std::vector<TileObject> tileObjects;
      std::vector<std::shared_ptr<WorldSprite>> spritesForAddition;

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
    std::shared_ptr<TileMeta> GetTileMeta(unsigned int tileGid);
    std::shared_ptr<Tileset> GetTileset(std::string name);
    std::shared_ptr<Tileset> GetTileset(unsigned int tileGid);
    void SetTileset(std::shared_ptr<Tileset> tileset, std::shared_ptr<TileMeta> tileMeta);
    size_t GetLayerCount() const;
    Layer& GetLayer(size_t index);
    Layer& AddLayer();
    bool CanMoveTo(float x, float y, int layer); // todo: move to layer?
    void RemoveSprites(SceneBase& scene);

  protected:
    unsigned cols{}, rows{}; /*!< map is made out of Cols x Rows tiles */
    int tileWidth{}, tileHeight{}; /*!< tile dimensions */
    std::string name, backgroundName, songPath;
    std::vector<Layer> layers;
    std::vector<std::shared_ptr<Tileset>> tileToTilesetMap;
    std::unordered_map<std::string, std::shared_ptr<Tileset>> tilesets;
    std::vector<std::shared_ptr<TileMeta>> tileMetas;

    /**
     * @brief Transforms an iso vector into an orthographic vector
     * @param iso position in isometric space
     * @return vector in orthographic space
     */
    const sf::Vector2f IsoToOrthogonal(const sf::Vector2f& iso) const;
  };
}