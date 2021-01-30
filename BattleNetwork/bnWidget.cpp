#include "bnWidget.h"

sf::Vector2f Widget::GetBounds()
{
  return container.getSize();
}

std::weak_ptr<Widget> Widget::GetOpenedSubmenu()
{
  return this->open;
}

const unsigned Widget::CountSubmenus() const
{
  return submenus.size();
}

void Widget::SelectSubmenu(unsigned at)
{
  if (at < submenus.size()) {
    open = submenus.at(at).submenu;
  }
}

void Widget::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= getTransform();

  if (dirty) {
    RecalculateContainer();
  }

  target.draw(container, states);
  SceneNode::draw(target, states);

  sf::Vector2f offset = container.getSize();

  // draw options if any
  if (currState == states::opened) {
    for (auto& item : this->submenus) {
      switch (item.dir) {
      case Direction::up:
        offset += { 0, -item.submenu->GetBounds().y };
        break;
      case Direction::down:
        offset += { 0, item.submenu->GetBounds().y };
        break;
      case Direction::left:
        offset += { -item.submenu->GetBounds().x, 0 };
        break;
      case Direction::right:
        offset += { item.submenu->GetBounds().x, 0 };
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
  if (currState != states::opened) {
    currState = states::opened;
    container.setFillColor(bgOpen);

    if (!submenus.empty()) {
      open = submenus.at(0).submenu;
    }
    OnOpen();
  }
}

void Widget::Close()
{
  if (currState == states::opened) {
    open.reset();
    OnClose();
    Active();
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

  if (content) {
    auto rect = content->CalculateBounds();
    sz.x = rect.width + padding.left + padding.right;
    sz.y = rect.height + padding.up + padding.down;
  }
  container.setSize(sz);
  container.setPosition(sf::Vector2f(-padding.left, -padding.up));
}
