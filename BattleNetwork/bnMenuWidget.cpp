#include "bnMenuWidget.h"
#include "bnDrawWindow.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

MenuWidget::MenuWidget(const std::string& area, const MenuWidget::OptionsList& options) :
  optionsList(options),
  font(Font::Style::small),
  infoText(font),
  areaLabel(font)
{
  // Load resources
  areaLabel.SetFont(font);
  areaLabel.setPosition(127, 119);
  areaLabel.setScale(sf::Vector2f(0.5f, 0.5f));
  infoText = areaLabel;
  
  widgetTexture = Textures().LoadTextureFromFile("resources/ui/main_menu_ui.png");

  banner.setTexture(Textures().LoadTextureFromFile("resources/ui/menu_overlay.png"));
  symbol.setTexture(widgetTexture);
  icon.setTexture(widgetTexture);
  exit.setTexture(widgetTexture);
  infoBox.setTexture(widgetTexture);
  selectTextSpr.setTexture(widgetTexture);
  placeTextSpr.setTexture(widgetTexture);

  AddNode(&banner);

  infoBoxAnim = Animation("resources/ui/main_menu_ui.animation");
  infoBoxAnim << "INFO";
  infoBoxAnim.SetFrame(1, infoBox.getSprite());
  AddNode(&infoBox);
  infoBox.setPosition(180, 52);

  optionAnim = Animation("resources/ui/main_menu_ui.animation");
  optionAnim << "SYMBOL";
  optionAnim.SetFrame(1, symbol.getSprite());
  AddNode(&symbol);
  symbol.setPosition(20, 1);

  optionAnim << "SELECT";
  optionAnim.SetFrame(1, selectTextSpr.getSprite());
  AddNode(&selectTextSpr);
  selectTextSpr.setPosition(4, 18);

  optionAnim << "PLACE";
  optionAnim.SetFrame(1, placeTextSpr.getSprite());
  AddNode(&placeTextSpr);
  placeTextSpr.setPosition(120, 111);

  optionAnim << "EXIT";
  optionAnim.SetFrame(1, exit.getSprite());
  AddNode(&exit);
  exit.setPosition(240, 144);
  exit.Hide();

  optionAnim << "PET";
  optionAnim.SetFrame(1, icon.getSprite());
  icon.setPosition(2, 3);

  exitAnim = Animation("resources/ui/main_menu_ui.animation") << Animator::Mode::Loop;

  //
  // Load options
  //

  CreateOptions();

  // Set name
  SetArea(area);
}

MenuWidget::~MenuWidget()
{
}

using namespace swoosh;

/**
8 frames to slide in
- frame 8 first folder appears
- "PLACE" name starts to appear one letter at a time
- Exit begins to slide in

@ 10 frames
"place" name scrolls out

@ 14 frames
- exit is in place
- the screen is fully dark
- right-hand window with HP and zenny begins to open up

@ 16 frames
- right-hand info window is fully open

@ 20 frames
- all the folder options have expanded
- ease in animation is complete
*/
void MenuWidget::QueueAnimTasks(const MenuWidget::state& state)
{
  easeInTimer.clear();

  if (state == MenuWidget::state::opening) {
    easeInTimer.reverse(false);
    easeInTimer.set(frames(0));
  }
  else {
    easeInTimer.reverse(true);
    easeInTimer.set(frames(21));
  }

  //
  // Start these task at the beginning
  //

  auto& t0f = easeInTimer.at(frames(0));
  
  t0f.doTask([=](double elapsed) {
      this->opacity = ease::linear(static_cast<float>(elapsed), seconds_cast<float>(frames(14)), 1.0f);
  }).withDuration(frames(14));

  t0f.doTask([=](double elapsed) {
    float x = ease::linear(static_cast<float>(elapsed), milli_cast<float>(frames(8)), 1.0f);
    this->banner.setPosition((1.0f-x)*-this->banner.getSprite().getLocalBounds().width, 0);
  }).withDuration(frames(8));

  if (state == MenuWidget::state::closing) {
    t0f.doTask([=](double elapsed) {
      currState = state::closed;
    });

    t0f.doTask([=](double elapsed) {
      for (size_t i = 0; i < options.size(); i++) {

        //
        // labels (menu options)
        //

        float x = ease::linear(static_cast<float>(elapsed), seconds_cast<float>(frames(20)), 1.0f);
        float start = 36;
        float dest = -(options[i]->getLocalBounds().width + start); // our destination

        // lerp to our hiding spot
        options[i]->setPosition(dest * (1.0f-x) + (x * start), options[i]->getPosition().y);

        //
        // icons
        //
        start = 16;
        dest = dest - start; // our destination is calculated from the previous label's pos

        // lerp to our hiding spot
        optionIcons[i]->setPosition(dest * (1.0f - x) + (x * start), optionIcons[i]->getPosition().y);
      }
    }).withDuration(frames(20));
  }

  //
  // These tasks begin at the 8th frame
  //

  auto& t8f = easeInTimer.at(frames(8));

  if (state == MenuWidget::state::opening) {
    t8f.doTask([=](double elapsed) {
      placeTextSpr.Reveal();
      selectTextSpr.Reveal();
      exit.Reveal();

      for (auto&& opts : options) {
        opts->Reveal();
      }

      for (auto&& opts : optionIcons) {
        opts->Reveal();
      }
    });

    t8f.doTask([=](double elapsed) {
      for (size_t i = 0; i < options.size(); i++) {
        float y = static_cast<float>(ease::linear(elapsed, milli_cast<double>(frames(12)), 1.0));
        options[i]->setPosition(36, 26 + (y * (i * 16)));
        optionIcons[i]->setPosition(16, 26 + (y * (i * 16)));
      }
    }).withDuration(frames(12));
  }
  else {
    t8f.doTask([=](double elapsed) {
      placeTextSpr.Hide();
      selectTextSpr.Hide();
      exit.Hide();

      //infobox task handles showing, but we need to hide if closing
      infoBox.Hide(); 

      for (auto&& opts : options) {
        opts->Hide();
      }


      for (auto&& opts : optionIcons) {
        opts->Hide();
      }
    });
  }

  t8f.doTask([=](double elapsed) {
    float x = 1.0f-ease::linear(static_cast<float>(elapsed), milli_cast<float>(frames(6)), 1.0f);
    exit.setPosition(130 + (x * 200), exit.getPosition().y);
  }).withDuration(frames(6));

  t8f.doTask([=](double elapsed) {
    size_t offset = static_cast<size_t>(12 * ease::linear(elapsed, milli_cast<double>(frames(2)), 1.0));
    std::string substr = areaName.substr(0, offset);
    areaLabel.SetString(substr);
  }).withDuration(frames(2));

  //
  // These tasks begin on the 14th frame
  //

  easeInTimer
    .at(time_cast<sf::Time>(frames(14)))
    .doTask([=](double elapsed) {
    infoBox.Reveal();
    infoBoxAnim.SyncTime(static_cast<float>(elapsed)/1000.0f);
    infoBoxAnim.Refresh(infoBox.getSprite());
  }).withDuration(frames(4));

  //
  // on frame 20 change state flag
  //
  if (state == MenuWidget::state::opening) {
    easeInTimer
      .at(frames(20))
      .doTask([=](double elapsed) {
      currState = state::opened;
    });
  }
}

void MenuWidget::CreateOptions()
{
  options.reserve(optionsList.size() * 2);
  optionIcons.reserve(optionsList.size() * 2);

  for (auto&& L : optionsList) {
    // label
    auto sprite = std::make_shared<SpriteProxyNode>();
    sprite->setTexture(Textures().LoadTextureFromFile("resources/ui/main_menu_ui.png"));
    sprite->setPosition(36, 26);
    optionAnim << (L.name + "_LABEL");
    optionAnim.SetFrame(1, sprite->getSprite());
    options.push_back(sprite);
    sprite->Hide();
    AddNode(sprite.get());

    // icon
    auto iconSpr = std::make_shared<SpriteProxyNode>();
    iconSpr->setTexture(Textures().LoadTextureFromFile("resources/ui/main_menu_ui.png"));
    iconSpr->setPosition(36, 26);
    optionAnim << L.name;
    optionAnim.SetFrame(1, iconSpr->getSprite());
    optionIcons.push_back(iconSpr);
    iconSpr->Hide();
    AddNode(iconSpr.get());
  }
}


void MenuWidget::Update(double elapsed)
{
  easeInTimer.update(sf::seconds(static_cast<float>(elapsed)));

  if (!IsOpen()) return;

  // loop over options
  for (size_t i = 0; i < optionsList.size(); i++) {
    if (i == row && selectExit == false) {
      optionAnim << (optionsList[i].name + "_LABEL");
      optionAnim.SetFrame(2, options[i]->getSprite());

      // move the icon inwards to the label
      optionAnim << optionsList[i].name;
      optionAnim.SetFrame(2, optionIcons[i]->getSprite());

      auto pos = optionIcons[i]->getPosition();
      float delta = ease::interpolate(0.5f, pos.x, 20.0f + 5.0f);
      optionIcons[i]->setPosition(delta, pos.y);
    }
    else {
      optionAnim << (optionsList[i].name + "_LABEL");
      optionAnim.SetFrame(1, options[i]->getSprite());

      // move the icon away from the label
      optionAnim << optionsList[i].name;
      optionAnim.SetFrame(1, optionIcons[i]->getSprite());

      auto pos = optionIcons[i]->getPosition();
      float delta = ease::interpolate(0.5f, pos.x, 16.0f);
      optionIcons[i]->setPosition(delta, pos.y);
    }
  }

  if (selectExit) {
    exitAnim.Update(elapsed, exit.getSprite());
  }
}

void MenuWidget::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (IsHidden()) return;

  states.transform *= getTransform();

  if (IsClosed()) {
    target.draw(icon, states);
  }
  else {
    // draw black square to darken bg
    auto view = target.getView();
    sf::RectangleShape screen(view.getSize());
    screen.setFillColor(sf::Color(0, 0, 0, int(opacity * 255.f * 0.5f)));
    target.draw(screen, sf::RenderStates::Default);

    // draw all child nodes
    SceneNode::draw(target, states);

    if (IsOpen()) {
      auto shadowColor = sf::Color(16, 82, 107, 255);

      // area text
      auto pos = areaLabel.getPosition();
      auto copyAreaLabel = areaLabel;
      copyAreaLabel.setPosition(pos.x + 1, pos.y + 1);
      copyAreaLabel.SetColor(shadowColor);
      target.draw(copyAreaLabel, states);
      target.draw(areaLabel, states);

      // hp shadow
      infoText.SetString(std::to_string(health));
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setPosition(174 + 1, 34 + 1);
      target.draw(infoText, states);

      // hp text
      infoText.setPosition(174, 34);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // "/" shadow
      infoText.SetString("/");
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setPosition(182 + 1, 34 + 1);
      target.draw(infoText, states);

      // "/"
      infoText.setPosition(182, 34);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // max hp shadow
      infoText.SetString(std::to_string(maxHealth));
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.SetColor(shadowColor);
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.setPosition(214 + 1, 34 + 1);
      target.draw(infoText, states);

      // max hp 
      infoText.setPosition(214, 34);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);

      // coins shadow
      infoText.SetColor(shadowColor);
      infoText.SetString(std::to_string(coins) + "z");
      infoText.setOrigin(infoText.GetLocalBounds().width, 0);
      infoText.setPosition(214 + 1, 58 + 1);
      target.draw(infoText, states);

      // coins
      infoText.setPosition(214, 58);
      infoText.SetColor(sf::Color::White);
      target.draw(infoText, states);
    }
  }
}

void MenuWidget::SetArea(const std::string& name)
{
  areaName = name;
}

void MenuWidget::SetCoins(int coins)
{
  MenuWidget::coins = coins;
}

void MenuWidget::SetHealth(int health)
{
  MenuWidget::health = health;
}

void MenuWidget::SetMaxHealth(int maxHealth)
{
  MenuWidget::maxHealth = maxHealth;
}

void MenuWidget::UseIconTexture(const std::shared_ptr<sf::Texture> iconTexture)
{
  this->iconTexture = iconTexture;
  this->icon.setTexture(iconTexture, true);
}

void MenuWidget::ResetIconTexture()
{
  iconTexture.reset();

  optionAnim << "PET";
  icon.setTexture(widgetTexture);
  optionAnim.SetFrame(1, icon.getSprite());
}

bool MenuWidget::ExecuteSelection()
{
  if (selectExit) {
    return Close();
  }
  else {
    auto func = optionsList[row].onSelectFunc;

    if (func) {
      func();
      return true;
    }
  }

  return false;
}

bool MenuWidget::SelectExit()
{
  if (!selectExit) {
    selectExit = true;
    exitAnim << "EXIT_SELECTED" << Animator::Mode::Loop;
    exitAnim.SetFrame(1, exit.getSprite());

    for (size_t i = 0; i < optionsList.size(); i++) {
      optionAnim << optionsList[i].name;
      optionAnim.SetFrame(1, optionIcons[i]->getSprite());

      optionAnim << (optionsList[i].name + "_LABEL");
      optionAnim.SetFrame(1, options[i]->getSprite());
    }
    return true;
  }

  return false;
}

bool MenuWidget::SelectOptions()
{
  if (selectExit) {
    selectExit = false;
    row = 0;
    exitAnim << "EXIT";
    exitAnim.SetFrame(1, exit.getSprite());
    return true;
  }

  return false;
}

bool MenuWidget::CursorMoveUp()
{
  if (!selectExit ) {
    if (--row < 0) {
      row = static_cast<int>(optionsList.size() - 1);
    }
    
    return true;
  }

  row = std::max(row, 0);

  return false;
}

bool MenuWidget::CursorMoveDown()
{
  if (!selectExit) {
    row = (row+1u) % (int)optionsList.size();

    return true;
  }

  return false;
}

bool MenuWidget::Open()
{
  if (currState == state::closed) {
    currState = state::opening;
    QueueAnimTasks(currState);
    easeInTimer.start();
    return true;
  }

  return false;
}

bool MenuWidget::Close()
{
  if (currState == state::opened) {
    currState = state::closing;
    QueueAnimTasks(currState);
    easeInTimer.start();
    return true;
  }

  return false;
}

bool MenuWidget::IsOpen() const
{
  return currState == state::opened;
}

bool MenuWidget::IsClosed() const
{
  return currState == state::closed;
}
