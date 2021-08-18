#include "bnPlayerEmotionUI.h"
#include "bnPlayer.h"
#include "battlescene/bnBattleSceneBase.h"

/** @todo This should probably be replace with a .animation file
 * A pair of Frame-count to Whether-or-not-to-show-the-new-or-old-emotion.
 */
static const struct {
  frame_time_t frames;
  bool showNewEmotion;
} flickerAnimation[] = {
  {  {30}, true },  // For frames [0-30), show new emotion
  {  {60}, false }, // For frames [30-60), show old emotion
  {  {90}, true },  // etc.
  { {120}, false },
  { {150}, true },  // End on new emotion so it's shown when there is no more animation
};

PlayerEmotionUI::PlayerEmotionUI(Player* player)
  : UIComponent(player)
  , player{ player }
  , texture{ nullptr }
  , emotionWindow()
  , emotion{ Emotion::normal }
  , previousEmotion{ Emotion::COUNT }
  , emotionChangeFrame{ 0 }
{
  emotionWindow.setPosition(3.f, 35.f);
  emotionWindow.setScale(2.f, 2.f);
  SetDrawOnUIPass(false);
}

PlayerEmotionUI::~PlayerEmotionUI()
{
  this->Eject();
}

void PlayerEmotionUI::SetTexture(std::shared_ptr<sf::Texture> texture)
{
  this->texture = texture;
  emotionWindow.setTexture(texture);

  OnUpdate(0);
}

void PlayerEmotionUI::Inject(BattleSceneBase& scene)
{
  scene.Inject(this);
}

void PlayerEmotionUI::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (this->IsHidden()) return;
  if ((player) && (!player->IsInForm()))
  {
    target.draw(emotionWindow, states);
  }
  UIComponent::draw(target, states);
}

void PlayerEmotionUI::OnUpdate(double elapsed)
{
  if (!player) return;
  if (!texture) return;

  // previousEmotion diff is used to determine if the animation needs to play
  if (previousEmotion == player->GetEmotion() && !isChangingEmotion) return;

  // If the player's emotion has changed, prepare the flicker animation
  if (emotion != player->GetEmotion())
  {
    isChangingEmotion = true;
    previousEmotion = emotion;
    emotion = player->GetEmotion();
    emotionChangeFrame = {0};
  }
  else
  {
    emotionChangeFrame += from_seconds(elapsed);
  }

  // Flicker between the previous emotion and current emotion
  auto it = std::find_if(std::begin(flickerAnimation), std::end(flickerAnimation),
                         [this](auto& animationData){ return emotionChangeFrame < animationData.frames; });
  if (it != std::end(flickerAnimation))
  {
    SetDisplayedEmotion(it->showNewEmotion ? emotion : previousEmotion);
  }
  else
  {
    // Once we have completed the animation, set previousEmotion to signal the animation is complete
    previousEmotion = emotion;
    isChangingEmotion = false;
  }
}

void PlayerEmotionUI::OnFormSelected(int index)
{
  if (index == -1)
  {
    Reveal();
  }
  else
  {
    Hide();
  }
}

void PlayerEmotionUI::SetDisplayedEmotion(Emotion emotion)
{
  if (emotion >= Emotion::COUNT) return;
  sf::Vector2i atlasSize = {(int)texture->getSize().x, (int)texture->getSize().y};

  if (atlasSize.y < ((int)Emotion::COUNT * AtlasCellHeight))
  {
    // Texture too small to be an atlas, must be a single emotion
    // Force use of full texture in case texture was hot swapped
    emotionWindow.setTextureRect({{0, 0}, {atlasSize.x, atlasSize.y}});
  }
  else
  {
    // Texture must be an atlas, select cell based on emotion offset
    int cellHeight = atlasSize.y / (int)Emotion::COUNT;
    int cellOffsetY = cellHeight * (int)emotion;
    emotionWindow.setTextureRect({
      {0, cellOffsetY},
      {atlasSize.x, cellHeight}
    });
  }
}
