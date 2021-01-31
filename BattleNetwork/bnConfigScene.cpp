#include <Swoosh/ActivityController.h>
#include <Segues/WhiteWashFade.h>
#include <Segues/BlackWashFade.h>

#include "bnConfigScene.h"
#include "bnRobotBackground.h"
#include "bnWebClientMananger.h"

ConfigScene::ConfigScene(swoosh::ActivityController& controller) :
  textbox(sf::Vector2f(4, 250)),
  label(Font::Style::thin),
  Scene(controller)
{
  textbox.SetTextSpeed(4.0);

  // Draws the scrolling background
  bg = new RobotBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  endBtnAnimator = Animation("resources/scenes/config/end_btn.animation");
  endBtnAnimator.Load();

  audioAnimator = Animation("resources/scenes/config/audio.animation");
  audioAnimator.Load();

  lightAnimator = Animation("resources/scenes/config/auth_widget.animation");
  lightAnimator.Load();

  // Audio() button
  audioBGM =  sf::Sprite(*LOAD_TEXTURE(AUDIO_ICO));
  audioBGM.setScale(2.f, 2.f);

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioBGM.setPosition(2*3, 2*140);

  audioSFX = audioBGM;
  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioSFX.setPosition(2 * 6 + 2 * 16, 2 * 140);

  // end button
  endBtn = sf::Sprite(*LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2*180, 2*10);

  // auth widget
  authWidget = sf::Sprite(*Textures().LoadTextureFromFile("resources/scenes/config/auth_widget.png"));
  light = authWidget; // copy

  configSettings = Input().GetConfigSettings();

  setView(sf::Vector2u(480, 320));

  auto imgBtn = std::make_shared<Button>(nullptr, "I have a picture");
  imgBtn->SetImage("resources/ui/zenny.png");


  auto layout = std::make_shared<VerticalLayout>(imgBtn, 200.0f);
  layout->AddWidget(std::make_shared<Button>(layout, "Click me"));
  layout->AddWidget(std::make_shared<Button>(layout, "No, me"));

  auto aaaa = std::make_shared<Button>(layout, "aaAAaaAAA");
  layout->AddWidget(aaaa);

  aaaa->AddSubmenu(Direction::right, std::make_shared<Button>(aaaa, ".... pick me!"));

  imgBtn->AddSubmenu(Direction::down, layout);
  
  menu = imgBtn;
}

void ConfigScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);
  bg->Update((float)elapsed);

  if (textbox.IsOpen()) {
    if (textbox.IsEndOfMessage()) {
      if (HasLeftButton()) {
        questionInterface->SelectYes();
      }
      else if (HasRightButton()) {
        questionInterface->SelectNo();
      }
      else if (HasCancelled()) {
        questionInterface->Cancel();
      }
      else if (HasConfirmed()) {
        questionInterface->ConfirmSelection();
      }
    }
    else if (HasConfirmed()) {
      if (!textbox.IsPlaying()) {
        questionInterface->Continue();
      }
      else {
        textbox.CompleteCurrentBlock();
      }
    }
  }

  if (!WEBCLIENT.IsLoggedIn()) {
    //uiList[0][static_cast<size_t>(MenuItems::account)].label = "LOGIN";
  }
  else {
    // uiList[0][static_cast<size_t>(MenuItems::account)].label = "LOGOUT " + WEBCLIENT.GetUserName();
  }

  if (HasUpButton()) {
    auto opened = menu->GetDeepestSubmenu();

    if (!opened) {
      opened = menu;
    }

    const auto& [valid, index] = opened->GetActiveSubmenuIndex();
    
    if (valid && index > 0) {
      opened->SelectSubmenu(index - 1);
    }
  }

  if (HasDownButton()) {
    auto opened = menu->GetDeepestSubmenu();

    if (!opened) {
      opened = menu;
    }

    const auto& [valid, index] = opened->GetActiveSubmenuIndex();

    if (valid && index < opened->CountSubmenus()) {
      opened->SelectSubmenu(index + 1);
    }
  }

  if (HasRightButton()) {
    auto opened = menu->GetDeepestSubmenu();
    if (!opened) {
      menu->Open();
    }
    else {
      opened->Open();
    }
  }

  if (HasLeftButton()) {
    auto opened = menu->GetDeepestSubmenu();
    if (opened) {
      opened->Close();
    }
  }

  if (HasConfirmed() && /*isSelectingTopMenu &&*/ !leave) {
    if (textbox.IsClosed()) {
      auto onYes = [this]() {
        // Save before leaving
        configSettings.SetKeyboardHash(keyHash);
        configSettings.SetGamepadHash(gamepadHash);
        ConfigWriter writer(configSettings);
        writer.Write("config.ini");
        ConfigReader reader("config.ini");
        Input().SupportConfigSettings(reader);
        textbox.Close();

        // transition to the next screen
        using namespace swoosh::types;
        using effect = segue<WhiteWashFade, milliseconds<300>>;
        getController().pop<effect>();

        Audio().Play(AudioType::NEW_GAME);
        leave = true;
      };

      auto onNo = [this]() {
        // Just close and leave
        using namespace swoosh::types;
        using effect = segue<BlackWashFade, milliseconds<300>>;
        getController().pop<effect>();
        leave = true;

        textbox.Close();
      };
      questionInterface = new Question("Overwrite your config settings?", onYes, onNo);
      textbox.EnqueMessage(sf::Sprite(), "", questionInterface);
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
  }

  // Make endBtn stick at the top of the form
  // endBtn.setPosition(endBtn.getPosition().x, uiList[0][0].position.y - 60);

  if (true /*isSelectingTopMenu*/) {
    endBtnAnimator.SetFrame(2, endBtn);
  }
  else {
    endBtnAnimator.SetFrame(1, endBtn);
  }
}

void ConfigScene::onDraw(sf::RenderTexture & surface)
{
  surface.draw(textbox);
  surface.draw(*menu);
}

const bool ConfigScene::HasConfirmed() const
{
  return Input().GetAnyKey() == sf::Keyboard::Return;
}

const bool ConfigScene::HasCancelled() const
{
  return (Input().GetAnyKey() == sf::Keyboard::BackSpace || Input().GetAnyKey() == sf::Keyboard::Escape);
}

const bool ConfigScene::HasUpButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Up;
}

const bool ConfigScene::HasDownButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Down;
}

const bool ConfigScene::HasLeftButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Left;
}

const bool ConfigScene::HasRightButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Right;
}

void ConfigScene::onStart()
{
  Audio().Stream("resources/loops/config.ogg", false);
}

void ConfigScene::onLeave()
{
  Audio().StopStream();
}

void ConfigScene::onExit()
{
}

void ConfigScene::onEnter()
{
  Audio().StopStream();
}

void ConfigScene::onResume()
{
}

void ConfigScene::onEnd()
{
}

ConfigScene::~ConfigScene()
{
}
