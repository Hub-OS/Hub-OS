#include "bnRevolvingMenuWidget.h"
#include <Swoosh/Ease.h>
#include "../bnDrawWindow.h"
#include "../bnTextureResourceManager.h"
#include "../bnAudioResourceManager.h"

namespace RealPET {
  RevolvingMenuWidget::RevolvingMenuWidget(const Menu::OptionsList& options, const RevolvingMenuWidget::barrel& barrel) :
    barrelParams(barrel),
    optionsList(options),
    infoText(Font::Style::thin),
    time(Font::Style::thick)
  {
    defaultPhaseWidth = barrel.phaseWidth;

    // Load resources
    infoText.setPosition(127, 119);

    // clock
    time.setPosition(480 - 4.f, 4.f);
    time.setScale(2.f, 2.f);
  }

  RevolvingMenuWidget::~RevolvingMenuWidget()
  {
  }

  using namespace swoosh;

  void RevolvingMenuWidget::CreateOptions()
  {
    options.reserve(optionsList.size() * 2);
    optionIcons.reserve(optionsList.size() * 2);

    for (auto&& L : optionsList) {
      // label
      auto sprite = std::make_shared<SpriteProxyNode>();
      sprite->setTexture(widgetTexture);
      sprite->setPosition(36, 26);
      optionAnim << (L.name + "_LABEL");
      optionAnim.SetFrame(1, sprite->getSprite());
      options.push_back(sprite);
      AddNode(sprite);

      if (useIcons) {
        // icon
        auto iconSpr = std::make_shared<SpriteProxyNode>();
        iconSpr->setTexture(widgetTexture);
        iconSpr->setPosition(36, 26);
        optionAnim << L.name;
        optionAnim.SetFrame(1, iconSpr->getSprite());
        optionIcons.push_back(iconSpr);
        AddNode(iconSpr);
      }
    }

    menuItemsMax = optionsList.size();
  }

  unsigned int RevolvingMenuWidget::GetAlpha() const
  {
    return alpha;
  }

  void RevolvingMenuWidget::SetAlpha(unsigned int alpha)
  {
    this->alpha = std::min(alpha, 255u);
  }

  void RevolvingMenuWidget::Update(double elapsed)
  {
    frameTick += from_seconds(elapsed);
    if (frameTick.count() >= 60) {
      frameTick = frames(0);
    }

    easeInTimer.update(sf::seconds(static_cast<float>(elapsed)));
    elapsedThisFrame = elapsed;

    if (!IsOpen()) return;

    using namespace swoosh::ease;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add)) {
      barrelParams.phaseWidth += 0.1;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract)) {
      barrelParams.phaseWidth -= 0.1;
    }

    float parentAlpha = alpha / 255.f;

    // loop over options
    for (size_t i = 0; i < optionsList.size(); i++) {      
      if (i == row) {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(2, options[i]->getSprite());

        if (useIcons) {
          // move the icon inwards to the label
          optionAnim << optionsList[i].name;
          optionAnim.SetFrame(2, optionIcons[i]->getSprite());
        }
      }
      else {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(1, options[i]->getSprite());

        if (useIcons) {
          // move the icon away from the label
          optionAnim << optionsList[i].name;
          optionAnim.SetFrame(1, optionIcons[i]->getSprite());
        }
      }

      int circleIdx = (static_cast<int>(row) - (optionsList.size() >> 1) - i);
      if (circleIdx > 0) {
        circleIdx = static_cast<int>(optionsList.size()) - circleIdx;
      }
      else if (circleIdx < 0) {
        circleIdx = std::abs(circleIdx) % optionsList.size();
      }

      sf::Vector2f pointOnB = barrelParams.calculate_point(circleIdx, optionsList.size());
      float s = wideParabola(static_cast<float>(circleIdx), static_cast<float>(menuItemsMax), parabolaParams.scalePower);
      s = std::max(s, parabolaParams.minScale);

      int layer = -int(s * 100);

      sf::Color c = sf::Color(255, 255, 255, static_cast<unsigned>(255.0f * parentAlpha));
      
      if (i != row) {
        float s = wideParabola(static_cast<float>(circleIdx), static_cast<float>(menuItemsMax), parabolaParams.opacityPower);
        s = std::max(s, parabolaParams.minOpacity);
        c = sf::Color(255, 255, 255, static_cast<unsigned>(255.0f * s * parentAlpha));
      }

      sf::Vector2f pos{};
      
      if (useIcons) {
        pos = optionIcons[i]->getPosition();
        float dx = interpolate(0.5f, pos.x, pointOnB.x);
        float dy = interpolate(0.5f, pos.y, pointOnB.y + getPosition().y);
        optionIcons[i]->setPosition(dx, dy);
        optionIcons[i]->setScale(s, s);
        optionIcons[i]->setColor(c);
        optionIcons[i]->SetLayer(layer);
      }

      pos = options[i]->getPosition();
      float dx = interpolate(0.5f, pos.x, pointOnB.x + 20.f);
      float dy = interpolate(0.5f, pos.y, pointOnB.y + getPosition().y);
      options[i]->setPosition(dx, dy);
      options[i]->setScale(s, s);
      options[i]->setColor(c);
      options[i]->SetLayer(layer);
    }
  }

  void RevolvingMenuWidget::HandleInput(InputManager& input, sf::Vector2f mousePos) {

  }


  void RevolvingMenuWidget::draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    if (IsHidden()) return;

    states.transform *= getTransform();

    if (!IsClosed()) {
      // draw all child nodes
      SceneNode::draw(target, states);
    }

    DrawTime(target);
  }

  void RevolvingMenuWidget::DrawTime(sf::RenderTarget& target) const
  {
    sf::Color shadowColor = sf::Color(105, 105, 105);
    std::string format = (frameTick.count() < 30) ? "%OI:%OM %p" : "%OI %OM %p";
    std::string timeStr = CurrentTime::AsFormattedString(format);
    time.SetString(timeStr);
    time.setOrigin(time.GetLocalBounds().width, 0.f);
    float origin = time.GetLocalBounds().width;

    sf::Vector2f pos = time.getPosition();
    time.SetColor(shadowColor);
    time.setPosition(pos.x + 2.f, pos.y + 2.f);
    target.draw(time);

    time.SetString(timeStr.substr(0, 5));
    time.setPosition(pos);
    time.SetColor(sf::Color::White);
    target.draw(time);

    sf::Color pColor = sf::Color::Red;
    time.SetString("AM");

    if (timeStr[6] != 'A') {
      pColor = sf::Color::Green;
      time.SetString("PM");
    }

    time.setOrigin(time.GetLocalBounds().width, 0.f);
    time.SetColor(pColor);
    target.draw(time);
  } 

  bool RevolvingMenuWidget::ExecuteSelection()
  {
    if (row < optionsList.size()) {
      auto& func = optionsList[row].onSelectFunc;

      if (func) {
        func();
        return true;
      }
    }

    return false;
  }

  bool RevolvingMenuWidget::CursorMoveUp()
  {
    if (--row < 0) {
      row = static_cast<int>(optionsList.size() - 1);
    }

    return true;
  }

  bool RevolvingMenuWidget::CursorMoveDown()
  {
    row = (row + 1u) % (int)optionsList.size();

    return true;
  }

  void RevolvingMenuWidget::SetAppearance(const std::string& texturePath, const std::string& animationPath, bool useIcons)
  {
    this->useIcons = useIcons;
    widgetTexture = Textures().LoadFromFile(texturePath);
    optionAnim = Animation(animationPath);

  }

  void RevolvingMenuWidget::SetBarrelFxParams(const parabolaFx& params)
  {
    parabolaParams = params;
  }

  size_t RevolvingMenuWidget::GetOptionsCount()
  {
    return optionsList.size();
  }

  void RevolvingMenuWidget::SetMaxOptionsVisible(size_t max)
  {
    menuItemsMax = std::min(optionsList.size(), max);
  }
}