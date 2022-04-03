#include "bnGameUtils.h"
#include "battlescene/bnMobBattleScene.h"
#include <Swoosh/Segue.h>
#include <Segues/WhiteWashFade.h>

using namespace swoosh::types;

GameUtils::GameUtils(Game& game) : game(game) {}

void GameUtils::LaunchMobBattle(PlayerMeta& playerMeta, MobMeta& mobMeta, std::shared_ptr<Background> background, PA& pa, CardFolder* folder)
{
  // Play the pre battle rumble sound
  Audio().Play(AudioType::PRE_BATTLE, AudioPriority::high);

  // Stop music and go to battle screen
  Audio().StopStream();

  // Get the navi we selected
  std::filesystem::path mugshotAnim = playerMeta.GetMugshotAnimationPath();
  std::shared_ptr<sf::Texture> mugshot = Textures().LoadFromFile(playerMeta.GetMugshotTexturePath());
  std::shared_ptr<sf::Texture> emotions = Textures().LoadFromFile(playerMeta.GetEmotionsTexturePath());
  std::shared_ptr<Player> player = std::shared_ptr<Player>(playerMeta.GetData());

  Mob* mob = mobMeta.GetData()->Build(std::make_shared<Field>(6, 3));

  std::unique_ptr<CardFolder> clonedFolder;

  if (folder) {
    clonedFolder = folder->Clone();
    clonedFolder->Shuffle();
  }
  else {
    // use a new blank folder if we dont have a proper folder selected
    clonedFolder = std::make_unique<CardFolder>();
  }

  // Queue screen transition to Battle Scene with a white fade effect
  // just like the game
  if (!mob->GetBackground()) {
    mob->SetBackground(background);
  }

  // Queue screen transition to Battle Scene with a white fade effect
  // just like the game
  MobBattleProperties props{
      { player, pa, std::move(clonedFolder), mob->GetField(), mob->GetBackground() },
      MobBattleProperties::RewardBehavior::take,
      { mob },
      sf::Sprite(*mugshot),
      mugshotAnim,
      emotions,
      {}
  };

  using effect = segue<WhiteWashFade>;
  game.push<effect::to<MobBattleScene>>(std::move(props));
}