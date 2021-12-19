/*! \brief Same as SelectedCardsUI but cleans up memory
 */
#pragma once
#include "bnSelectedCardsUI.h"

class Card;
class OwnedCardsUI : public SelectedCardsUI {
public:
  /**
   * \param character Character to attach to
   */
  OwnedCardsUI(std::shared_ptr<Character> owner, CardPackagePartition* partition);

  /**
   * @brief destructor
   */
  virtual ~OwnedCardsUI();

  void AddCards(const std::vector<Battle::Card>& cards);
private:
  std::vector<Battle::Card> ownedCards; /*!< list of provided cards. */
};
