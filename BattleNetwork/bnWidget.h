#pragma once
#include <SFML/Graphics.hpp>
#include "bnSceneNode.h"
#include "bnDirection.h"

/*!
 * @class Widget 
 * @brief Drawable content and sub-menu options
*/
class Widget : public SceneNode {
  private:
    const sf::Color grey{ 125, 125, 125, 255 };

    enum class states : char {
      inactive = 0,
      active,
      opened
    } currState{ 0 };

  public:
    /*!
     * @brief Content must calculate its encompassing bounds for the widget parent
    */
    class Content : public SceneNode  {
    public:
      virtual sf::FloatRect CalculateBounds() = 0;
      Widget* GetParentWidget() { return dynamic_cast<Widget*>(GetParent()); }
      Content() : SceneNode() { }
      virtual ~Content() { }
    };

    /*!
     * @brief submenu item has a popout direction
    */
    struct item {
      std::shared_ptr<Widget> submenu;
      Direction dir{ Direction::none };
    };

    /*!
     * @brief represents some values along a boxy surface
    */
    struct edges {
        double left{}, right{}, up{}, down{};
    };

    sf::Vector2f GetBounds();

    std::weak_ptr<Widget> GetOpenedSubmenu();
    const unsigned CountSubmenus() const;
    void SelectSubmenu(unsigned at);

    /*!
     * @brief color-matches the background based on state and surrounds the content with a box
     * @param target draw target
     * @param states render states
    */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    void SetPadding(double value);
    void SetDefaultColor(const sf::Color& color);
    void SetActiveColor(const sf::Color& color);
    void SetOpenedColor(const sf::Color& color);
    void Open();
    void Close();
    void Active();
    void Inactive();
    virtual void OnOpen();
    virtual void OnClose();
    virtual void OnActive();
    virtual void OnInactive();
  private:
    void RecalculateContainer() const;

    mutable bool dirty{ false }; //!< if the container needs recalculating
    mutable edges padding{}, margin{}, cachedPadding{}, cachedMargin{}; //!< sizes and cached sizes
    mutable sf::RectangleShape container{ {1.f,1.f} }; //!< widget container (box)
    sf::Color bg{ grey }, bgActive{ grey }, bgOpen{ grey }; //!< state colors
    std::unique_ptr<Content> content; //!< drawable content ptr
    std::weak_ptr<Widget> open; // active submenu ptr
    std::vector<item> submenus; //!< submenu items with popout directions
};