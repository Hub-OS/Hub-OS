/*! \file bnPA.h */

/*! \brief PA loads the recipes for PA combos and 
 *         provides interface to swap chips with PA chip
 * 
 * Program Advanced class parses a PA input file into a lookup table.
 * Then the class accepts chips as input during game play and replaces
 * matching PA sets with a new unlisted chip.
 *  
 * This takes place during the transition from chip custom select screen
 * and battle. The names of each chip in the PA is listed one at a time,
 * then the PA name is displayed and the battle continues.
 */
   
#pragma once

#include <string>
#include <vector>
#include "bnChip.h"

typedef std::pair<std::string, char> PAStep; /*!< Name of chip and code */
typedef std::vector<PAStep> PASteps;         /*!< List of steps for a PA*/

class PA
{
  /*! \class PAData
   *  \desc Describes the PA chip and what steps it needs */
  struct PAData {
    std::string name; /*!< name of PA*/
    unsigned icon;    /*!< icon of the PA*/
    unsigned damage;  /*!< damage of the PA*/
    Element type;     /*!< element of the PA*/

    /*! \class Required
     *  \desc The structure for matching name and code */
    struct Required {
      std::string chipShortName; /*!< name of chip */
      char code;                 /*!< code of chip */
    };

    std::vector<Required> steps; /*!< list of steps for PA */
  };

  std::vector<PAData> advances; /*!< list of all PAs */
  std::vector<PAData>::iterator iter; /*!< iterator */
  Chip* advanceChipRef; /*!< Allocated PA needs to be deleted */
public:
  /**
   * @brief sets advanceChipRef to null
   */
  PA();
  
  /**
   * @brief If advanceChipRef is non null, deletes it. Clears PA lookup.
   */
  ~PA();
  
  /**
   * @brief Interpets and loads data from PA file at resources/database/PA.txt
   */
  void LoadPA();
  
  /**
   * @brief Extracts the value for a key given a line
   * @param _key to look for
   * @param _line input string
   * @return value of key
   */
  std::string valueOf(std::string _key, std::string _line);
  
  /**
   * @brief Given a list of chips, generates a matching PA. 
   * @param input list of chips
   * @param size size of chip list
   * @return -1 if no match. Otherwise returns the start position of the PA
   */
  const int FindPA(Chip** input, unsigned size);
  
  /**
   * @brief Returns the list of matching steps in the PA
   * @return const PASteps
   */
  const PASteps GetMatchingSteps();
  
  /**
   * @brief Fetch the generated PA as a chip 
   * @return Chip*
   * @warning do not delete this pointer!
   * This is deleted by the PA 
   */
  Chip* GetAdvanceChip();
};

