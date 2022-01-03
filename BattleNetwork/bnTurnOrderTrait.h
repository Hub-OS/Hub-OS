#pragma once

#include <vector>

using namespace std;

template<typename T>
class TurnOrderTrait {
public:
  virtual ~TurnOrderTrait() {
    RemoveMeFromTurnOrder();
  }

protected:

  TurnOrderTrait() {
    myTurnID = (long)IDs.size();
    IDs.push_back(myTurnID);
  };

  /**
 * @brief Used in states, if this entity is allowed to move
 * @return
 */
  const bool IsMyTurn() const {
    // this could be the first frame, all mettaurs are loaded, so set the index to start
    if (currIndex >= IDs.size()) { currIndex = 0; }

    if (IDs.size() > 0)
      return (IDs.at(currIndex) == myTurnID);

    return false;
  }

  /**
   * @brief Passes control to the next entity of same type
   */
  void EndMyTurn() {
    currIndex++;

    if (currIndex >= IDs.size() && IDs.size() > 0) {
      currIndex = 0;
    }
  }

  void RemoveMeFromTurnOrder() {
    if (IDs.size() > 0) {
      vector<long>::iterator it = find(IDs.begin(), IDs.end(), myTurnID);

      if (it != IDs.end()) {
        // Remove me out of rotation...
        IDs.erase(it);
        EndMyTurn();
      }
    }
  }

private:
  static vector<long> IDs; /*!< list of types spawned to take turns */
  static int currIndex; /*!< current active entity ID */
  long myTurnID; /*!< This entity's turn ID */
};

template<typename T> vector<long>
TurnOrderTrait<T>::IDs = vector<long>();

template<typename T> int
TurnOrderTrait<T>::currIndex = 0;