#include "bnWidget.h"

Widget::Widget(std::shared_ptr<Widget> parent) : parent(parent)
{
  // initial state values
  currState = states::inactive;
  container.setFillColor(bg);
  container.setOutlineColor(sf::Color::Blue);
  container.setOutlineThickness(1.0f);
}

std::pair<bool, size_t> Widget::GetActiveSubmenuIndex()
{
  if (currState == states::opened) {
    return { true, submenuIndex };
  }

  return { false,{} };
}

std::shared_ptr<Widget> Widget::GetParentWidget()
{
  return parent;
}

std::shared_ptr<Widget> Widget::GetDeepestSubmenu()
{
  auto recent = this->open;

  while (recent) {
    auto next = recent->GetDeepestSubmenu();
    if (next) {
      recent = next;
    }
    else {
      break;
    }
  }

  return recent;
}

const unsigned Widget::CountSubmenus() const
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
  submenus.push_back(Widget::item{ openDir, item });
}

void Widget::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= getTransform();

  if (true || dirty) {
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

void Widget::SetPadding(double value)
{
  // for now, just increase padding equally
  padding.left = padding.right = padding.up = padding.down = value;
}

void Widget::SetDefaultColor(const sf::Color& color)
{
  this->bg = color;
}

void Widget::SetActiveColor(const sf::Color& color)
{
  this->bgActive = color;
}

void Widget::SetOpenedColor(const sf::Color& color)
{
  this->bgOpen = color;
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
      open.reset();
    }
    OnClose();
    Inactive();
  }
  else {
    if (parent) {
      parent->open.reset();
      parent->Close();
    }
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
    open.reset();
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
