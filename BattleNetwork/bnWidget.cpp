#include "bnWidget.h"

// static defines
Widget* Widget::widgetInFocus{ nullptr };

Widget::Widget(Widget* parent) : parent(parent)
{
  Init(parent);
}

Widget::Widget()
{
  Init(nullptr);
}

Widget::~Widget()
{
}

const sf::FloatRect Widget::CalculateBounds() const
{
  if (layout) {
    return layout->CalculateBounds();
  }

  return sf::FloatRect();
}

std::pair<bool, size_t> Widget::GetActiveSubmenuIndex()
{
  if (currState == states::opened) {
    return { true, submenuIndex };
  }

  return { false,{} };
}

Widget* Widget::GetParentWidget()
{
  return parent;
}

Widget& Widget::GetDeepestSubmenu()
{
  if (currState != states::opened)
    return *this;

  auto* recent = this->open.get();

  while (recent) {
    auto next = &recent->GetDeepestSubmenu();
    if (next != recent) {
      recent = next;
    }
    else {
      if (recent->children.size() && (recent->IsActive() || recent->IsOpen())) {
        recent = &recent->children[recent->selectedChildIndex]->GetDeepestSubmenu();
      }
      break;
    }
  }

  return *recent;
}

const size_t Widget::CountSubmenus() const
{
  return submenus.size();
}

void Widget::SelectSubmenu(size_t at)
{
  if (at < submenus.size()) {
    open = submenus.at(at).submenu;
    open->Active();
    submenuIndex = at;
  }
}

void Widget::AddSubmenu(Direction openDir, std::shared_ptr<Widget> item)
{
  item->parent = this;
  submenus.push_back(Widget::item{ openDir, item });
}

void Widget::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= getTransform();

  if (dirty) {
    RecalculateContainer();
  }

  const auto& bounds = container.getSize();

  target.draw(container, states);
  SceneNode::draw(target, states);

  sf::Vector2f offset = {};

  // draw options if any
  if (currState == states::opened) {
    for (auto& item : this->submenus) {
      switch (item.dir) {
      case Direction::up:
        offset += { 0, -bounds.y };
        break;
      case Direction::down:
        offset += { 0, bounds.y };
        break;
      case Direction::left:
        offset += { -bounds.x, 0 };
        break;
      case Direction::right:
        offset += { bounds.x, 0 };
        break;
      default:
        // do nothing
        break;
      }

      item.submenu->setPosition(offset);
      item.submenu->draw(target, states);
    }
  }
}

void Widget::ReceiveEvents(InputManager& input)
{
}

void Widget::SetPadding(float value)
{
  // for now, just increase padding equally
  padding.left = padding.right = padding.up = padding.down = value;
}

void Widget::SetDefaultColor(const sf::Color& color)
{
  this->bg = color;
}

void Widget::UpdateAll(InputManager& input, double elapsed)
{
 
  if (auto ptr = widgetInFocus) {
    ptr->ReceiveEvents(input);
  }
}

void Widget::SetActiveColor(const sf::Color& color)
{
  this->bgActive = color;
}

void Widget::SetOpenedColor(const sf::Color& color)
{
  this->bgOpen = color;
}

const bool Widget::IsOpen() const
{
  return currState == states::opened;
}

const bool Widget::IsActive() const
{
  return currState == states::active;
}

void Widget::Open()
{
  if (!submenus.empty()) {
    if (currState != states::opened) {
      currState = states::opened;
      container.setFillColor(bgOpen);
      open = submenus.at(0).submenu;
      open->Active();
      OnOpen();
    }
  }
}

void Widget::Close()
{
  if (currState == states::opened) {
    if (open) {
      open->Close();
    }
    OnClose();
    Inactive();
  }
}

void Widget::Active()
{
  if (currState != states::active) {
    currState = states::active;
    container.setFillColor(bgActive);
    OnActive();
  }
}

void Widget::Inactive()
{
  if (currState != states::inactive) {
    currState = states::inactive;
    container.setFillColor(bg);
    open = nullptr;
    OnInactive();
  }
}

void Widget::OnOpen()
{
  // implement in child class
}

void Widget::OnClose()
{
  // implement in child class
}

void Widget::OnActive()
{
  // implement in child class
}

void Widget::OnInactive()
{
  // implement in child class
}

void Widget::RecalculateContainer() const
{
  dirty = false;
  cachedMargin = margin;
  cachedPadding = padding;
  
  sf::Vector2f sz{ 1.f, 1.f };

  auto rect = CalculateBounds();
  sz.x = rect.width + padding.left + padding.right;
  sz.y = rect.height + padding.up + padding.down;

  container.setSize(sz);
  container.setPosition(sf::Vector2f(-padding.left, -padding.up));
}

void Widget::Init(Widget* parent)
{
  // initial state values
  currState = states::inactive;
  container.setFillColor(bg);
  container.setOutlineColor(sf::Color::Blue);
  container.setOutlineThickness(1.0f);
}

void Widget::SetLayout(Widget::Layout* layout)
{
  dirty = true;
  this->layout = layout;
}
