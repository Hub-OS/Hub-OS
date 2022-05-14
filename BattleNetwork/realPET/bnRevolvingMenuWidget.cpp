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

    widgetTexture = Textures().LoadFromFile("resources/ui/main_menu_ui.png");

    banner = std::make_shared<SpriteProxyNode>();
    symbol = std::make_shared<SpriteProxyNode>();
    infoBox = std::make_shared<SpriteProxyNode>();
    selectTextSpr = std::make_shared<SpriteProxyNode>();

    banner->setTexture(Textures().LoadFromFile("resources/ui/menu_overlay.png"));
    symbol->setTexture(widgetTexture);
    infoBox->setTexture(widgetTexture);
    selectTextSpr->setTexture(widgetTexture);

    AddNode(banner);

    infoBoxAnim = Animation("resources/ui/main_menu_ui.animation");
    infoBoxAnim << "INFO";
    infoBoxAnim.SetFrame(1, infoBox->getSprite());
    AddNode(infoBox);
    infoBox->setPosition(180, 52);

    optionAnim = Animation("resources/ui/main_menu_ui.animation");
    optionAnim << "SYMBOL";
    optionAnim.SetFrame(1, symbol->getSprite());
    AddNode(symbol);
    symbol->setPosition(20, 1);

    optionAnim << "SELECT";
    optionAnim.SetFrame(1, selectTextSpr->getSprite());
    AddNode(selectTextSpr);
    selectTextSpr->setPosition(4, 18);
    //
    // Load options
    //

    CreateOptions();
  }

  RevolvingMenuWidget::~RevolvingMenuWidget()
  {
  }

  using namespace swoosh;

  void RevolvingMenuWidget::QueueAnimTasks(const RevolvingMenuWidget::state& state)
  {

  }

  void RevolvingMenuWidget::CreateOptions()
  {
    options.reserve(optionsList.size() * 2);
    optionIcons.reserve(optionsList.size() * 2);

    for (auto&& L : optionsList) {
      // label
      auto sprite = std::make_shared<SpriteProxyNode>();
      sprite->setTexture(Textures().LoadFromFile("resources/ui/main_menu_ui.png"));
      sprite->setPosition(36, 26);
      optionAnim << (L.name + "_LABEL");
      optionAnim.SetFrame(1, sprite->getSprite());
      options.push_back(sprite);
      sprite->Hide();
      AddNode(sprite);

      // icon
      auto iconSpr = std::make_shared<SpriteProxyNode>();
      iconSpr->setTexture(Textures().LoadFromFile("resources/ui/main_menu_ui.png"));
      iconSpr->setPosition(36, 26);
      optionAnim << L.name;
      optionAnim.SetFrame(1, iconSpr->getSprite());
      optionIcons.push_back(iconSpr);
      iconSpr->Hide();
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

    // loop over options
    for (size_t i = 0; i < optionsList.size(); i++) {
      if (i == row && selectExit == false) {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(2, options[i]->getSprite());

        // move the icon inwards to the label
        optionAnim << optionsList[i].name;
        optionAnim.SetFrame(2, optionIcons[i]->getSprite());

        const auto& pos = optionIcons[i]->getPosition();
        float delta = ease::interpolate(0.5f, pos.x, 20.0f + 5.0f);
        optionIcons[i]->setPosition(delta, pos.y);
      }
      else {
        optionAnim << (optionsList[i].name + "_LABEL");
        optionAnim.SetFrame(1, options[i]->getSprite());

        // move the icon away from the label
        optionAnim << optionsList[i].name;
        optionAnim.SetFrame(1, optionIcons[i]->getSprite());

        const auto& pos = optionIcons[i]->getPosition();
        float delta = ease::interpolate(0.5f, pos.x, 16.0f);
        optionIcons[i]->setPosition(delta, pos.y);
      }
    }
  }

  void RevolvingMenuWidget::HandleInput(InputManager& input, sf::Vector2f mousePos) {
    if (currState != state::opened) {
      // ignore input unless the menu is fully open
      return;
    }

    if (input.Has(InputEvents::pressed_pause) && !input.Has(InputEvents::pressed_cancel)) {
      Close();
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      return;
    }
  }


  void RevolvingMenuWidget::draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    if (IsHidden()) return;

    states.transform *= getTransform();

    if (!IsClosed()) {
      // draw black square to darken bg
      const sf::View view = target.getView();
      sf::RectangleShape screen(view.getSize());
      screen.setFillColor(sf::Color(0, 0, 0, int(opacity * 255.f * 0.5f)));
      target.draw(screen, sf::RenderStates::Default);

      // draw all child nodes
      SceneNode::draw(target, states);

      auto shadowColor = sf::Color(16, 82, 107, 255);

      if (currState == state::opened) {
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
    }

    DrawTime(target);
  }

  void RevolvingMenuWidget::DrawTime(sf::RenderTarget& target) const
  {
    auto shadowColor = sf::Color(105, 105, 105);
    std::string format = (frameTick.count() < 30) ? "%OI:%OM %p" : "%OI %OM %p";
    std::string timeStr = CurrentTime::AsFormattedString(format);
    time.SetString(timeStr);
    time.setOrigin(time.GetLocalBounds().width, 0.f);
    auto origin = time.GetLocalBounds().width;

    auto pos = time.getPosition();
    time.SetColor(shadowColor);
    time.setPosition(pos.x + 2.f, pos.y + 2.f);
    target.draw(time);

    time.SetString(timeStr.substr(0, 5));
    time.setPosition(pos);
    time.SetColor(sf::Color::White);
    target.draw(time);

    auto pColor = sf::Color::Red;
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
    if (selectExit) {
      if (currState == state::opened) {
        Close();
        Audio().Play(AudioType::CHIP_DESC_CLOSE);
        return true;
      }
    }
    else {
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
    if (!selectExit) {
      if (--row < 0) {
        row = static_cast<int>(optionsList.size() - 1);
      }

      return true;
    }

    // else if exit is selected
    selectExit = false;

    return false;
  }

  bool RevolvingMenuWidget::CursorMoveDown()
  {
    if (!selectExit) {
      row = (row + 1u) % (int)optionsList.size();

      return true;
    }

    // else if exit is selected

    selectExit = false;

    return false;
  }

  void RevolvingMenuWidget::Open()
  {
    if (currState == state::closed) {
      Audio().Play(AudioType::CHIP_DESC);
      currState = state::opening;
      QueueAnimTasks(currState);
      easeInTimer.start();
      Menu::Open();
    }
  }

  void RevolvingMenuWidget::Close()
  {
    if (currState == state::opened) {
      currState = state::closing;
      QueueAnimTasks(currState);
      easeInTimer.start();
    }
  }
}