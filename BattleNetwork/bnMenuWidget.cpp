#include "bnMenuWidget.h"
#include "bnEngine.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

MenuWidget::MenuWidget(const std::string& area)
{
  // Load resources
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthin_regular.ttf");
  areaLabel.setFont(*font);
  areaLabel.setPosition(127, 119);
  areaLabel.setScale(sf::Vector2f(0.5f, 0.5f));
  
  banner.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/menu_overlay.png"));
  symbol.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
  icon.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
  exit.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
  infoBox.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
  selectText.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
  placeText.setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));

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
  optionAnim.SetFrame(1, selectText.getSprite());
  AddNode(&selectText);
  selectText.setPosition(4, 18);

  optionAnim << "PLACE";
  optionAnim.SetFrame(1, placeText.getSprite());
  AddNode(&placeText);
  placeText.setPosition(120, 111);

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
      this->opacity = ease::linear(elapsed, (double)frames(14), 1.0);
  }).withDuration(frames(14));

  t0f.doTask([=](double elapsed) {
    double x = ease::linear(elapsed, (double)frames(8), 1.0);
    this->banner.setPosition((1.0-x)*-this->banner.getSprite().getLocalBounds().width, 0);
  }).withDuration(frames(8));

  if (state == MenuWidget::state::closing) {
    t0f.doTask([=](double elapsed) {
      currState = state::closed;
    });

    t0f.doTask([=](double elapsed) {
      for (size_t i = 0; i < options.size(); i++) {
        float x = ease::linear(elapsed, (double)frames(20), 1.0);
        options[i]->setPosition(x * (36+options[i]->getLocalBounds().width), options[i]->getPosition().y);
      }
    }).withDuration(frames(20));
  }

  //
  // These tasks begin at the 8th frame
  //

  auto& t8f = easeInTimer.at(frames(8));

  if (state == MenuWidget::state::opening) {
    t8f.doTask([=](double elapsed) {
      placeText.Reveal();
      selectText.Reveal();
      exit.Reveal();

      for (auto&& opts : options) {
        opts->Reveal();
      }
    });

    t8f.doTask([=](double elapsed) {
      for (size_t i = 0; i < options.size(); i++) {
        float y = ease::linear(elapsed, (double)frames(12), 1.0);
        options[i]->setPosition(36, 26 + (y * (i * 16)));
      }
    }).withDuration(frames(12));
  }
  else {
    t8f.doTask([=](double elapsed) {
      placeText.Hide();
      selectText.Hide();
      exit.Hide();

      //infobox task handles showing, but we need to hide if closing
      infoBox.Hide(); 

      for (auto&& opts : options) {
        opts->Hide();
      }
    });
  }

  t8f.doTask([=](double elapsed) {
    float x = 1.0f-ease::linear(elapsed, (double)frames(6), 1.0);
    exit.setPosition(130 + (x * 200), exit.getPosition().y);
  }).withDuration(frames(6));

  t8f.doTask([=](double elapsed) {
    size_t offset = 12 * ease::linear(elapsed, (double)frames(2), 1.0);
    std::string substr = areaName.substr(0, offset);
    areaLabel.setString(sf::String(substr));
  }).withDuration(frames(2));

  //
  // These tasks begin on the 14th frame
  //

  easeInTimer
    .at(frames(14))
    .doTask([=](double elapsed) {
    infoBox.Reveal();
    infoBoxAnim.SyncTime(elapsed/1000.0);
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
  optionsList = {
    "CHIP_FOLDER_LABEL",
    "NAVI_LABEL",
    "CONFIG_LABEL",
    "MOB_SELECT_LABEL",
    "SYNC_LABEL"
  };

  options.reserve(optionsList.size() * 2);

  for (auto&& L : optionsList) {
    auto sprite = std::make_shared<SpriteProxyNode>();
    sprite->setTexture(TEXTURES.LoadTextureFromFile("resources/ui/main_menu_ui.png"));
    sprite->setPosition(36, 26);
    optionAnim << L;
    optionAnim.SetFrame(1, sprite->getSprite());
    options.push_back(sprite);
    sprite->Hide();
    AddNode(sprite.get());
  }
}


void MenuWidget::Update(float elapsed)
{
  easeInTimer.update(elapsed);

  // loop over options
  if (selectExit == false) {
    for (size_t i = 0; i < optionsList.size(); i++) {
      if (i == row) {
        optionAnim << optionsList[i];
        optionAnim.SetFrame(2, options[i]->getSprite());
      }
      else {
        optionAnim << optionsList[i];
        optionAnim.SetFrame(1, options[i]->getSprite());
      }
    }
  }
  else {
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
    auto view = ENGINE.GetView();
    sf::RectangleShape screen(view.getSize());
    screen.setFillColor(sf::Color(0, 0, 0, int(opacity * 255.f * 0.5f)));
    target.draw(screen, sf::RenderStates::Default);

    // draw all child nodes
    SceneNode::draw(target, states);

    auto pos = areaLabel.getPosition();
    auto copyAreaLabel = areaLabel;
    copyAreaLabel.setPosition(pos.x + 1, pos.y + 1);
    copyAreaLabel.setColor(sf::Color(100, 100, 101, 255));
    target.draw(copyAreaLabel, states);

    target.draw(areaLabel, states);
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

bool MenuWidget::ExecuteSelection()
{
  if (selectExit) {
    // close
    QueueAnimTasks(MenuWidget::state::closing);
    return true;
  }
  else {
    switch (row) {
      // TODO: actions
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
      optionAnim << optionsList[i];
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
      row = optionsList.size() - 1;
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
