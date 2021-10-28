#pragma once
#include <SFML/Graphics.hpp>
#include "bnInputManager.h"
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
    const sf::Color active{ 205, 205, 205, 255 };

    enum class states : char {
      inactive = 0,
      active,
      opened
    } currState{ 0 };
  public:
    class Layout {
    private:
      Widget* parent{ nullptr };

    public:
      Layout(Widget* parent) : parent(parent) {
        parent->SetLayout(this);
      }

      virtual ~Layout() {}

      Widget* GetParent() const {
        return parent;
      }

      virtual const sf::FloatRect CalculateBounds() const = 0;
    };

    Widget(Widget* parent);
    Widget();
    virtual ~Widget();

    /*!
     * @brief submenu item has a popout direction
    */
    struct item {
      Direction dir{ Direction::none };
      std::shared_ptr<Widget> submenu{ nullptr };
    };

    /*!
     * @brief represents some values along a boxy surface
    */
    struct edges {
        float left{}, right{}, up{}, down{};
    };

    const sf::FloatRect CalculateBounds() const;
    std::pair<bool, size_t> GetActiveSubmenuIndex();
    Widget* GetParentWidget();
    Widget& GetDeepestSubmenu();
    const size_t CountSubmenus() const;
    void SelectSubmenu(size_t at);
    void AddSubmenu(Direction openDir, std::shared_ptr<Widget> item);

    /*!
     * @brief color-matches the background based on state and surrounds the content with a box
     * @param target draw target
     * @param states render states
    */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
    virtual void ReceiveEvents(InputManager& input);
    static void UpdateAll(InputManager& input, double elapsed);

    void SetPadding(float value);
    void SetDefaultColor(const sf::Color& color);
    void SetActiveColor(const sf::Color& color);
    void SetOpenedColor(const sf::Color& color);

    void SelectChild(size_t at) {
      if (at < children.size()) {
        if (children[at]->IsActive() == false) {
          children[selectedChildIndex]->Inactive();
          selectedChildIndex = at;
          children[at]->Active();
        }
      }
    }

    std::vector<std::shared_ptr<Widget>> GetChildren() {
      return children;
    }

   size_t GetSelectedChildIndex() const {
      return selectedChildIndex;
    }


    std::shared_ptr<Widget> GetChild(size_t index) {
      if (index < children.size()) {
        return children[index];
      }

      return nullptr;
    }

    void AddWidget(std::shared_ptr<Widget> content) {
      dirty = true;
      content->parent = this;
      children.push_back(content);
      AddNode(content);
    }

    const bool IsOpen() const;
    const bool IsActive() const;
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
    void Init(Widget* parent);
    void SetLayout(Widget::Layout* layout);

    size_t submenuIndex{}, selectedChildIndex{};
    mutable bool dirty{ false }; //!< if the container needs recalculating
    mutable edges padding{}, margin{}, cachedPadding{}, cachedMargin{}; //!< sizes and cached sizes
    mutable sf::RectangleShape container{ {1.f,1.f} }; //!< widget container (box)
    sf::Color bg{ grey }, bgActive{ active }, bgOpen{ darkGrey }; //!< state colors
    std::vector<item> submenus; //!< submenu items with popout directions
    std::vector<std::shared_ptr<Widget>> children; //!< child widgets
    Layout* layout{ nullptr }; //!< dictates the bounds of a widget and rearranges items if required
    Widget* parent{ nullptr }; //!< parent widget
    std::shared_ptr<Widget> open{ nullptr }; //! active submenu ptr (only valid if owning widget is also OPEN)
    static Widget* widgetInFocus; //!< Keep context of currently selected widget
};

class VerticalLayout final : public Widget::Layout {
private:
  mutable float maxWidth{ 1.0 };
  mutable float rowWidth{ 0.f };
  mutable sf::FloatRect bounds{};

public:
  VerticalLayout(Widget* parent, float maxWidth) : 
    maxWidth(maxWidth),
    Layout(parent)
  {
  }

  const sf::FloatRect CalculateBounds() const override {
    bounds = {};

    auto items = GetParent()->GetChildren();
    Widget* last = nullptr;

    for (auto& item : items) {
      const auto& contentBounds = item->CalculateBounds();

      if (rowWidth + contentBounds.left + contentBounds.width >= maxWidth)
      {
        if (last) {
          const auto& lastBounds =last->CalculateBounds();
          item->setPosition({ 0.f, lastBounds.height });
          bounds.height += lastBounds.top + lastBounds.height;
        }
        else {
          bounds.height += contentBounds.top + contentBounds.height;
        }
        rowWidth = contentBounds.left + contentBounds.width;
      }
      else {
        if (last) {
          const auto& lastBounds = last->CalculateBounds();
          item->setPosition({ lastBounds.left + lastBounds.width, lastBounds.top });
        }

        rowWidth += contentBounds.width;
      }

      bounds.width = std::max(rowWidth, bounds.width);
      bounds.width = std::min(maxWidth, bounds.width);

      last = item.get();
    }

    return bounds;
  }

  ~VerticalLayout() {
  }
};