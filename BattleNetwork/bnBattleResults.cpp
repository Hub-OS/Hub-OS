#include "bnBattleResults.h"
#include "bnWebClientMananger.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEngine.h"
#include "bnMob.h"
#include "bnBattleItem.h"
#include <numeric>
#include <algorithm>
#include <random>

/*
  Calculate score and rank
  Calculations are based off http ://megaman.wikia.com/wiki/Virus_Busting

  Delete Time
    0.00~5.00 = 7
    5.01~12.00 = 6
    12.01~36.00 = 5
    36.01 and up = 4

    Boss Delete Time
    0.00~30.00 = 10
    30.01~40.00 = 8
    40.01~50.00 = 6
    50.01 and up = 4

    Number of Hits(received by MegaMan)
    0 = +1
    1 = 0
    2 = -1
    3 = -2
    4 or more = -3

    Movement Number of Steps
    0~2 = +1
    3 or more = +0

    Multiple Deletion
    Double Delete = +2
    Triple Delete = +4

    Counter Hits
    0 = +0
    1 = +1
    2 = +2
    3 = +3
    */

BattleResults::BattleResults(sf::Time battleLength, int moveCount, int hitCount, int counterCount, bool doubleDelete, bool tripleDelete, Mob *mob) 
: cardMatrixIndex(0), isRevealed(false), playSoundOnce(false), rewardIsCard(false), item(nullptr), score(0), counterCount(0), totalElapsed(0)
{
  this->counterCount = std::min(3, counterCount);
  this->counterCount = std::max(0, this->counterCount);

  std::random_device rd;
  std::mt19937 g(rd());

  // begin counting index at 0
  std::iota(hideCardMatrix.begin(), hideCardMatrix.end(), 0);

  // shuffle twice - one shuffle looks too linear
  std::shuffle(hideCardMatrix.begin(), hideCardMatrix.end(), g);
  std::shuffle(hideCardMatrix.begin(), hideCardMatrix.end(), g);

  cardMatrixIndex = 0;

  if(!mob->IsBoss()) {
    if (battleLength.asSeconds() > 36.1) score += 4;
    else if (battleLength.asSeconds() > 12.01) score += 5;
    else if (battleLength.asSeconds() > 5.01) score += 6;
    else score += 7;
  }
  else {
    if (battleLength.asSeconds() > 50.01) score += 4;
    else if (battleLength.asSeconds() > 40.01) score += 6;
    else if (battleLength.asSeconds() > 30.01) score += 8;
    else score += 10;
  }

  switch (hitCount) {
    case 0: score += 1; break;
    case 1: score += 0; break;
    case 2: score -= 1; break;
    case 3: score -= 2; break;
    default: score -= 3; break;
  }

  if (moveCount >= 0 && moveCount <= 2) {
    score += 1;
  }

  if (doubleDelete) score += 2;
  if (tripleDelete) score += 4;

  score += counterCount;

  // No score of zero or below. Min score of 1
  score = std::max(1, score);

  // Get reward based on score
  item = mob->GetRankedReward(score);

  resultsSprite = sf::Sprite(*TEXTURES.GetTexture(TextureType::BATTLE_RESULTS_FRAME));
  resultsSprite.setScale(2.f, 2.f);
  resultsSprite.setPosition(-resultsSprite.getTextureRect().width*2.f, 20.f);

  pressA = sf::Sprite(*LOAD_TEXTURE(BATTLE_RESULTS_PRESS_A));
  pressA.setScale(2.f, 2.f);
  pressA.setPosition(2.f*42.f, 249.f);

  star = sf::Sprite(*LOAD_TEXTURE(BATTLE_RESULTS_STAR));
  star.setScale(2.f, 2.f);
  
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  if (item) {
    rewardCard = sf::Sprite(*WEBCLIENT.GetImageForCard(item->GetUUID()));

    rewardCard.setTextureRect(sf::IntRect(0,0,56,48));

    if (item->IsCard()) {
      rewardIsCard = true;

      cardCode.setFont(*font);
      cardCode.setPosition(2.f*114.f, 209.f);
      cardCode.setString(std::string() + item->GetCardCode());
    }
  }
  else {
    rewardCard = sf::Sprite(*TEXTURES.GetTexture(TextureType::BATTLE_RESULTS_NODATA));
  }

  rewardCard.setScale(2.f, 2.f);
  rewardCard.setPosition(274.0f, 180.f);

  time.setFont(*font);
  time.setPosition(2.f*192.f, 79.f);
  time.setString(FormatString(battleLength));
  time.setOrigin(time.getLocalBounds().width, 0);

  rank.setFont(*font);
  rank.setPosition(2.f*192.f, 111.f);

  reward.setFont(*font);
  reward.setPosition(2.f*42.f, 209.f);

  if (item) {
    reward.setString(item->GetName());
  }
  else {
    reward.setString("No Data");
  }

  if (score > 10) {
    rank.setString("S");
  }
  else {
    rank.setString(std::to_string(score));
  }

  rank.setOrigin(rank.getLocalBounds().width, 0);

  playSoundOnce = false;
}

BattleResults::~BattleResults() {

}

std::string BattleResults::FormatString(sf::Time time)
{
  double totalMS = time.asMilliseconds();
  
  int minutes = (int)totalMS / 1000 / 60;
  int seconds = (int)(totalMS / 1000.0) % 60;
  int ms = (int)(totalMS - (minutes*60*1000) - (seconds*1000)) % 99;

  if (minutes > 59) {
    minutes = 59;
  }

  std::string O = "0";
  std::string builder;

  if (minutes < 10) builder += O;
  builder += std::to_string(minutes) + ":";

  if (seconds < 10) builder += O;
  builder += std::to_string(seconds) + ":";

  if (ms < 10) builder += O;
  builder += std::to_string(ms);

  return builder;
}

// GUI ops
bool BattleResults::CursorAction() {
  bool prevStatus = isRevealed;

  if (!isRevealed) {
    isRevealed = true;
    totalElapsed = 0;
  } /*
    else if(extraItems.size() > 0) {
        item = extraItems.top(); extraItems.pop();
        totalElapsed =  0;
    }
    */

  return prevStatus;
}

bool BattleResults::IsOutOfView() {
  float bounds = -resultsSprite.getTextureRect().width*2.f;

  if (resultsSprite.getPosition().x <= bounds) {
    resultsSprite.setPosition(bounds, resultsSprite.getPosition().y);
  }

  return (resultsSprite.getPosition().x == bounds);
}

bool BattleResults::IsInView() {
  float bounds = 50.f;

  if (resultsSprite.getPosition().x >= bounds) {
    resultsSprite.setPosition(bounds, resultsSprite.getPosition().y);
  }

  return (resultsSprite.getPosition().x == bounds);
}

void BattleResults::Move(sf::Vector2f delta) {
  resultsSprite.setPosition(resultsSprite.getPosition() + delta);
  IsInView();
}

void BattleResults::Update(double elapsed)
{
  totalElapsed += elapsed;

  if ((int)totalElapsed % 2 == 0) {
    pressA.setScale(0, 0);
  }
  else {
    pressA.setScale(2.f, 2.f);
  }

  if (isRevealed) {
    if (cardMatrixIndex == hideCardMatrix.size() && !playSoundOnce) {
      playSoundOnce = true;
      AUDIO.Play(AudioType::ITEM_GET);
    }
    else {
      if (cardMatrixIndex < hideCardMatrix.size()) {
        hideCardMatrix[cardMatrixIndex++] = 0;
      }
    }

    if (!playSoundOnce) {
      AUDIO.Play(AudioType::TEXT, AudioPriority::lowest);
    }
  }
}

void BattleResults::Draw() {
  ENGINE.Draw(resultsSprite, false);

  // moves over when there's counter stars
  auto starSpacing = [](int index) -> float { return (19.f*index); };
  auto rankPos = sf::Vector2f((2.f*191.f) - starSpacing(counterCount), 110.f);

  if (IsInView()) {
    if (!isRevealed)
      ENGINE.Draw(pressA, false);

    // Draw shadow
    rank.setPosition(rankPos.x+1.f, rankPos.y+2.f);

    if (score > 10) {
      rank.setFillColor(sf::Color(56, 92, 25));
    }
    else {
      rank.setFillColor(sf::Color(80, 72, 88));
    }

    ENGINE.Draw(rank, false);

    // Draw overlay
    rank.setPosition(rankPos);

    if (score > 10) {
      rank.setFillColor(sf::Color(176, 228, 24));
    }
    else {
      rank.setFillColor(sf::Color(240, 248, 248));
    }
    ENGINE.Draw(rank, false);

    // Draw shadow
    time.setPosition(2.f*192.f, 80.f);
    time.setFillColor(sf::Color(80, 72, 88));
    ENGINE.Draw(time, false);

    // Draw overlay
    time.setPosition(2.f*191.f, 78.f);
    time.setFillColor(sf::Color(240, 248, 248));
    ENGINE.Draw(time, false);

    if (isRevealed) {
      ENGINE.Draw(rewardCard, false);

      sf::RectangleShape c(sf::Vector2f(8 * 2, 8 * 2));
      c.setFillColor(sf::Color::Black);
      c.setOutlineColor(sf::Color::Black);

      // obscure the card with a matrix
      for (auto cell : hideCardMatrix) {
        if (cell >= cardMatrixIndex) {
          // position based on cell's index from a 7x6 matrix
          sf::Vector2f offset = sf::Vector2f(float(cell % 7) * 8.0f, float(std::floor(cell / 7) * 8.0f));
          offset = 2.0f * offset;

          c.setPosition(rewardCard.getPosition() + offset);
          ENGINE.Draw(c, false);
        }
      }

      if (IsFinished()) {
        ENGINE.Draw(reward, false);

        if (rewardIsCard) {
          ENGINE.Draw(cardCode, false);
        }
      }
    }

    for (int i = 0; i < counterCount; i++) {
      star.setPosition(rankPos.x + starSpacing(i) + (starSpacing(1) / 2.0f), rankPos.y + 16.0f);
      ENGINE.Draw(star, false);
    }
  }
}

// Card ops
bool BattleResults::IsFinished() {
  return isRevealed && hideCardMatrix.size() == cardMatrixIndex;
}

BattleItem* BattleResults::GetReward()
{
  // pass ownership off
  BattleItem* reward = item;
  item = nullptr;
  return reward;
}
