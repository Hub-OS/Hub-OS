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
    const sf::Color darkGrey{ 100, 100, 100, 255 };
    const sf::Color active{ 205, 125, 125, 255 };

    enum class states : char {
      inactive = 0,
      active,
      opened
    } currState{ 0 };
  public:
    Widget(std::shared_ptr<Widget> parent);

    /*!
     * @brief submenu item has a popout direction
    */
    struct item {
      Direction dir{ Direction::none };
      std::shared_ptr<Widget> submenu;
    };

    /*!
     * @brief represents some values along a boxy surface
    */
    struct edges {
        double left{}, right{}, up{}, down{};
    };

    virtual const sf::FloatRect CalculateBounds() const = 0;

    std::pair<bool, size_t> GetActiveSubmenuIndex();
    std::shared_ptr<Widget> GetParentWidget();
    std::shared_ptr<Widget> GetDeepestSubmenu();
    const unsigned CountSubmenus() const;
    void SelectSubmenu(size_t at);
    void AddSubmenu(Direction openDir, std::shared_ptr<Widget> item);

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

    size_t submenuIndex{};
    mutable bool dirty{ false }; //!< if the container needs recalculating
    mutable edges padding{}, margin{}, cachedPadding{}, cachedMargin{}; //!< sizes and cached sizes
    mutable sf::RectangleShape container{ {1.f,1.f} }; //!< widget container (box)
    sf::Color bg{ grey }, bgActive{ active }, bgOpen{ darkGrey }; //!< state colors
    std::shared_ptr<Widget> open; // active submenu ptr
    std::shared_ptr<Widget> parent;
    std::vector<item> submenus; //!< submenu items with popout directions
};

class VerticalLayout final : public Widget {
private:
  float maxWidth{ 1.0 };
  std::vector<std::shared_ptr<Widget>> items;
  float rowWidth{ 0.f };
  sf::FloatRect bounds{ 0.f, 0.f, 1.f, 1.f };

public:
  VerticalLayout(std::shared_ptr<Widget> parent, float maxWidth) : 
    maxWidth(maxWidth),
    Widget(parent)
  {
  }

  const sf::FloatRect CalculateBounds() const override {
    return bounds;
  }

  ~VerticalLayout() {
    for (const auto& item : items) {
      RemoveNode(item.get());
    }
  }

  void AddWidget(std::shared_ptr<Widget> content) {
    const auto& contentBounds = content->CalculateBounds();

    if (rowWidth + contentBounds.left + contentBounds.width >= maxWidth)
    {
      content->setPosition({ 0.f, bounds.height });
      bounds.height += contentBounds.top + contentBounds.height;
      rowWidth = contentBounds.left + contentBounds.width;
    }
    else {
      if (items.size()) {
        const auto& lastBounds = items.back()->CalculateBounds();

        content->setPosition({ lastBounds.left + lastBounds.width, lastBounds.top });
      }
      else {
        bounds.height = contentBounds.top + contentBounds.height;
      }

      rowWidth += contentBounds.width;
    }

    bounds.width = std::max(rowWidth, bounds.width);
    bounds.width = std::min(maxWidth, bounds.width);

    items.push_back(content);
    AddNode(content.get());
  }
};