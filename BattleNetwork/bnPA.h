/*! \file bnPA.h */

/*! \brief PA loads the recipes for PA combos and
 *         provides interface to swap cards with PA card
 *
 * Program Advanced class parses a PA input into a lookup table.
 * Then the class accepts cards as input during game play and replaces
 * matching PA sets with a new unlisted card.
 *
 * This takes place during the transition from card custom select screen
 * and battle. The names of each card in the PA is listed one at a time,
 * then the PA name is displayed and the battle continues.
 */

#pragma once

#include <string>
#include <vector>
#include "bnCard.h"
#include "bnHitProperties.h"

class PA
{
public:
  enum class RuleType : char {
    ordered = 0, // only order matters: any code can be replaced by wildcard
    dupes        // set of duplicate cards matching the code combination with a limit of 1 wildcard
  };

  struct StepData {
    std::string id;
    std::string name;
    char code;
  }; /*!< Name of card and code */

  typedef std::vector<StepData> Steps; /*!< List of steps for a PA*/

  /*! \class PACard
   *  \desc Describes the PA card and what steps it needs */
  struct PACard {
    std::string name; /*!< name of PA*/
    std::string uuid; /*!< UUID of PA*/
    std::string action{ "IDLE" }; /*!< Action this PA invokes*/
    unsigned damage{ 0 };  /*!< damage of the PA*/
    Element primaryElement{ Element::none };/*!< element of the PA*/
    Element secondElement{ Element::none }; /*!< Secondary (hidden) element of PA*/
    Hit::Flags hitFlags{ 0 };
    bool canBoost{ false }; /*!< true if damage > 0*/
    bool timeFreeze{ false }; /*!< Triggers time freeze if true */

    std::vector<std::string> metaClasses; /*!< User-created class types*/
    PA::Steps steps; /*!< list of steps for PA */
    RuleType ruleType;
  };

  /**
   * @brief sets advanceCardRef to null
   */
  PA();

  /**
   * @brief If advanceCardRef is non null, deletes it. Clears PA lookup.
   */
  ~PA();

  /**
   * @brief Registers a new combo
   */
  void RegisterPA(PACard entry);

  /**
   * @brief Given a list of cards, generates a matching PA.
   * @param input list of cards
   * @param size size of card list
   * @return -1 if no match. Otherwise returns the start position of the PA
   */
  const int FindPA(std::vector<Battle::Card>& input);

  /**
   * @brief Returns the list of matching steps in the PA
   * @return const PASteps
   */
  const PA::Steps GetMatchingSteps();

  /**
   * @brief Fetch the generated PA as a card
   * @return Card*
   * @warning do not delete this pointer!
   * This is deleted by the PA
   */
  Battle::Card& GetAdvanceCard();

  std::vector<PACard> advances; /*!< list of all PAs */
  std::vector<PACard>::iterator iter; /*!< iterator */
  Battle::Card* advanceCardRef; /*!< Allocated PA needs to be deleted */
};

