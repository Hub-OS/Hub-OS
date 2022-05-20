#include "bnRevolvingMenuWidget.h"
#include <Swoosh/Ease.h>
#include "../bnDrawWindow.h"
#include "../bnTextureResourceManager.h"
#include "../bnAudioResourceManager.h"

namespace RealPET {
  RevolvingMenuWidget::RevolvingMenuWidget(const std::shared_ptr<PlayerSession>& session, const Menu::OptionsList& options) :
    session(session),
    optionsList(options),
    infoText(Font::Style::thin),
    time(Font::Style::thick)
  {
    // Load resources
    infoText.setPosition(127, 119);

    // clock
    time.setPosition(480 - 4.f, 6.f);
    time.setScale(2.f, 2.f);

    widgetTexture = Textures().LoadFromFile(TexturePaths::PET_MENU);
    optionAnim = Animation(AnimationPaths::PET_MENU);

    //
    // Load options
    //

    CreateOptions();
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
      sprite->setTexture(Textures().LoadFromFile(TexturePaths::PET_MENU));
      sprite->setPosition(36, 26);
      optionAnim << (L.name + "_LABEL");
      optionAnim.SetFrame(1, sprite->getSprite());
      options.push_back(sprite);
      AddNode(sprite);

      // icon
      auto iconSpr = std::make_shared<SpriteProxyNode>();
      iconSpr->setTexture(Textures().LoadFromFile(TexturePaths::PET_MENU));
      iconSpr->setPosition(36, 26);
      optionAnim << L.name;
      optionAnim.SetFrame(1, iconSpr->getSprite());
      optionIcons.push_back(iconSpr);
      AddNode(iconSpr);
    }
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

    size_t menuItemsMax = optionsList.size();

    barrel b { 
      0.0,        // start angle
      phaseWidth, // phase width
      50.0        //radius
    };

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add)) {
      phaseWidth += 0.1;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract)) {
      phaseWidth -= 0.1;
    }

    // loop over options
    for (size_t i = 0; i < optionsList.size(); i++) {      
      if (i == row) {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(2, options[i]->getSprite());

        // move the icon inwards to the label
        optionAnim << optionsList[i].name;
        optionAnim.SetFrame(2, optionIcons[i]->getSprite());
      }
      else {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(1, options[i]->getSprite());

        // move the icon away from the label
        optionAnim << optionsList[i].name;
        optionAnim.SetFrame(1, optionIcons[i]->getSprite());
      }

      int circleIdx = static_cast<int>(row) - (optionsList.size() >> 1) - i;
      if (circleIdx > 0) {
        circleIdx = static_cast<int>(optionsList.size()) - circleIdx;
      }
      else if (circleIdx < 0) {
        circleIdx = std::abs(circleIdx) % optionsList.size();
      }

      sf::Vector2f pointOnB = b.calculate_point(circleIdx, optionsList.size());
      pointOnB.x *= 0.7;

      float s = wideParabola(static_cast<float>(circleIdx+1), static_cast<float>(menuItemsMax), 1.0f);
      sf::Color c = sf::Color::White;
      
      if (i != row) {
        float s = wideParabola(static_cast<float>(circleIdx + 1), static_cast<float>(menuItemsMax), 0.5f);
        c = sf::Color(255, 255, 255, static_cast<unsigned>(255.0f * s));
      }

      sf::Vector2f pos = optionIcons[i]->getPosition();
      float dx = interpolate(0.5f, pos.x, pointOnB.x);
      float dy = interpolate(0.5f, pos.y, pointOnB.y + getPosition().y);
      optionIcons[i]->setPosition(dx, dy);
      optionIcons[i]->setScale(s, s);
      optionIcons[i]->setColor(c);

      pos = options[i]->getPosition();
      dx = interpolate(0.5f, pos.x, pointOnB.x + 20.f);
      dy = interpolate(0.5f, pos.y, pointOnB.y + getPosition().y);
      options[i]->setPosition(dx, dy);
      options[i]->setScale(s, s);
      options[i]->setColor(c);
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

      sf::Color shadowColor = sf::Color(16, 82, 107, 255);

      // hp shadow
      infoText.SetString(std::to_string(session->health));
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setPosition(174 + 1, 33 + 1);
      target.draw(infoText, states);

      // hp text
      infoText.setPosition(174, 33);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // "/" shadow
      infoText.SetString("/");
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setPosition(182 + 1, 33 + 1);
      target.draw(infoText, states);

      // "/"
      infoText.setPosition(182, 33);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // max hp shadow
      infoText.SetString(std::to_string(session->maxHealth));
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.setPosition(214 + 1, 33 + 1);
      target.draw(infoText, states);

      // max hp 
      infoText.setPosition(214, 33);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // coins shadow
      infoText.SetColor(shadowColor);
      infoText.SetString(std::to_string(session->money) + "$");
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.setPosition(214 + 1, 57 + 1);
      target.draw(infoText, states);

      // coins
      infoText.setPosition(214, 57);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);
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
}