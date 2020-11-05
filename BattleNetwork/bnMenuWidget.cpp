#include "bnMenuWidget.h"
#include "bnTextureResourceManager.h"

MenuWidget::MenuWidget()
{
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthin_regular.ttf");
  areaLabel.setFont(*font);
}

MenuWidget::~MenuWidget()
{
}

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
void MenuWidget::Update(float elapsed)
{
  easeInTimer.update(elapsed);

  //optionAnim.Update(elapsed);

  switch (currState) {
  case state::closed: 
  {

  }
  break;
  case state::closing: 
  {

  }
  break;
  case state::opened:
  {

  }
  break;
  case state::opening: 
  {
    float seconds = easeInTimer.getElapsed().asSeconds();
    infoBoxAnim.Update(elapsed, infoBox.getSprite());
    float bgOpacity = swoosh::ease::linear(seconds, 14.0f / 60.0f, 1.0f);

    if (seconds > 8) {

    }
  }
  break;
  }


}

void MenuWidget::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
}

void MenuWidget::SetArea(const std::string& name)
{
  areaLabel.setString(sf::String(name));
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

void MenuWidget::ExecuteSelection()
{
  if (selectExit) {

  }
  else {
    switch (row) {
      
    }
  }
}

bool MenuWidget::SelectExit()
{
  if (!selectExit) {
    selectExit = true;
    return true;
  }

  return false;
}

bool MenuWidget::SelectOptions()
{
  if (selectExit) {
    selectExit = false;
    row = 0;
    return true;
  }

  return false;
}

bool MenuWidget::CursorMoveUp()
{
  if (!selectExit && --row >= 0) {
    return true;
  }

  row = std::max(row, 0);
  return false;
}

bool MenuWidget::CursorMoveDown()
{
  if (!selectExit && ++row < options.size()) {
    return true;
  }

  row = std::max(row, (int)options.size()-1);
  return false;
}

bool MenuWidget::Open()
{
  if (currState == state::closed) {
    currState = state::opening;
    easeInTimer.reset();
    easeInTimer.start();
    return true;
  }

  return false;
}

bool MenuWidget::Close()
{
  if (currState == state::opened) {
    currState = state::closing;
    easeInTimer.reset();
    easeInTimer.start();
    return true;
  }

  return false;
}

bool MenuWidget::IsOpen()
{
  return currState == state::opened;
}

bool MenuWidget::IsClosed()
{
  return currState == state::closed;
}
