#pragma once
#include "bnChip.h"

class ChipUsePublisher;
class Character;

/**
 * @class ChipUseListener
 * @author mav
 * @date 05/05/19
 * @brief Listens for chip use events emitted by ChipUsePublishers
 */
class ChipUseListener {
public:
  ChipUseListener() = default;
  ~ChipUseListener() = default;

  ChipUseListener(const ChipUseListener& rhs) = delete;
  ChipUseListener(ChipUseListener&& rhs) = delete;

<<<<<<< HEAD
  virtual void OnChipUse(Chip& chip, Character& user) = 0;
=======
  /**
   * @brief What happens when we recieve the chip event
   * @param chip
   * @param user
   */
  virtual void OnChipUse(Chip& chip, Character& user) = 0;
  
  /**
   * @brief Subscribe this listener to a publisher objects
   * @param publisher
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Subscribe(ChipUsePublisher& publisher);
};