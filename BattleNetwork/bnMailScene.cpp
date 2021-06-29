#include "bnMailScene.h"
#include "bnTextureResourceManager.h"
#include <Segues/BlackWashFade.h>

MailScene::MailScene(swoosh::ActivityController& controller, Mail& inbox) :
  inbox(inbox),
  label(Font::Style::thin),
  textbox(360,40, Font::Style::thin),
  Scene(controller)
{
  label.setScale(2.f, 2.f);

  moreText.setTexture(*LOAD_TEXTURE(TEXT_BOX_NEXT_CURSOR));
  moreText.setScale(2.f, 2.f);

  scroll.setTexture(*LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scroll.setScale(2.f, 2.f);

  cursor.setTexture(*LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);

  bg.setTexture(*Textures().LoadTextureFromFile("resources/scenes/items/bg.png"), true);
  bg.setScale(2.f, 2.f);

  // Text box navigator
  textbox.setPosition(100, 205);
  textbox.SetTextColor(sf::Color::Black);
  textbox.Mute();

  // greetings
  std::string msg = "You have no mail";

  if (inbox.Size()) {
    msg = "Select mail to read it!";
  }

  textbox.SetText(msg);
  textbox.CompleteAll();

  scroll.setPosition(450.f, 40.f);
  setView(sf::Vector2u(480, 320));
}

MailScene::~MailScene()
{
}

void MailScene::onLeave()
{
}

void MailScene::onExit()
{
}

void MailScene::onEnter()
{
}

void MailScene::onResume()
{
}

void MailScene::onStart()
{
}

void MailScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);

  unsigned lastRow = row;

  if (Input().Has(InputEvents::pressed_ui_up)) {
    row--;
  }
  else if (Input().Has(InputEvents::pressed_ui_down)) {
    row++;
  }
  else if (Input().Has(InputEvents::pressed_confirm)) {
    if (row < inbox.Size()) {

      if (textbox.IsEndOfMessage()) {
        // TODO: use on-read callback...
        textbox.SetText(inbox.GetAt(row).body);
        textbox.CompleteAll();
        Audio().Play(AudioType::CHIP_SELECT);
      }else if (textbox.HasMore()) {
        for (size_t i = 0; i < textbox.GetNumberOfFittingLines(); i++) {
          textbox.ShowNextLine();
        }
        Audio().Play(AudioType::CHIP_CHOOSE);
      }
    }
  }
  else if (Input().Has(InputEvents::pressed_cancel)) {
    using namespace swoosh::types;
    getController().pop<segue<BlackWashFade, milli<500>>>();
    Audio().Play(AudioType::CHIP_DESC_CLOSE);
  }

  signed lastRowOffset = rowOffset;

  if (row >= maxRows) {
    rowOffset++;
    row--;
  }

  if (row < 0) {
    rowOffset--;
    row++;
  }

  signed maxRowOffset = inbox.Size();

  row = std::max(0, row);

  if (maxRowOffset > 1) {
    row = std::min(maxRowOffset - 1, row);
  }

  size_t index = row;

  rowOffset = std::max(0, rowOffset);
  rowOffset = std::min(maxRowOffset, rowOffset);
  index += rowOffset;

  // Now that we've forced the change of values into a restricted range,
  // check to see if we actually selected anything new
  if (!(index < inbox.Size() && (rowOffset != lastRowOffset || row != lastRow))) {
    row = lastRow;
    rowOffset = lastRowOffset;
  }

  float startPos = 40.f;
  float endPos = 160.f;
  float rowsPerView = std::max(1.f, static_cast<float>(maxRowOffset - maxRows));
  float delta = static_cast<float>(rowOffset) / rowsPerView;
  float y = (endPos * delta) + (startPos * (1.f - delta));

  scroll.setPosition(450.f, y);

  cursor.setPosition(30 + 1.f, 50 + (row * 30) + 1.f);

  auto bounce = std::sin((float)totalElapsed * 20.0f);
  moreText.setPosition(sf::Vector2f(225, 140+bounce) * 2.0f);
  totalElapsed += static_cast<float>(elapsed);
}

void MailScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);

  for (size_t i = 0; i < inbox.Size(); i++) {
    size_t offset = static_cast<size_t>(rowOffset);
    size_t index = offset + i;

    auto& msg = inbox.GetAt(index);
    label.SetString(msg.title);
    label.setPosition(12.f, 42 + (i * 30.f) + 4.f);

    const auto pos = label.getPosition();
    label.setPosition(pos.x - 2.f, pos.y - 2.f);

    sf::Color read = sf::Color(165, 214, 255);
    sf::Color unread = sf::Color::White;
    sf::Color from = sf::Color(115, 255, 189);

    // TITLE
    if (msg.read) {
      label.SetColor(read);
    }
    else {
      label.SetColor(unread);
    }

    surface.draw(label);

    // FROM
    label.SetString(msg.from);
    label.setPosition(label.getPosition().x + 10.f, label.getPosition().y);
    label.SetColor(from);
    surface.draw(label);

    // NUMBER
    label.SetString(std::to_string(offset + i));
    label.setPosition(label.getPosition().x + 10.f, label.getPosition().y);
    label.SetColor(unread); 
    surface.draw(label);
  }
 
  surface.draw(scroll);
  surface.draw(textbox);

  if (textbox.HasMore()) {
    surface.draw(moreText);
  }

  surface.draw(cursor);
}

void MailScene::onEnd()
{
}
