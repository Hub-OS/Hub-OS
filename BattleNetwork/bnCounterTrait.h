#pragma once

#include <vector>

using namespace std;

template<typename T>
class CounterTrait {
protected:

  CounterTrait() {
    myCounterID = (long)IDs.size();
    IDs.push_back(myCounterID);
  };

  /**
 * @brief Used in states, if this entity is last in the list
 * @return
 */
  const bool IsLast() const {
    return IDs[0] == myCounterID;
  }


  void RemoveMeFromCounterList() {
    if (IDs.size() > 0) {
      vector<long>::iterator it = find(IDs.begin(), IDs.end(), myCounterID);

      if (it != IDs.end()) {
        // Remove me
        IDs.erase(it);
      }
    }
  }

  static const int GetCounterSize() {
    return (int)IDs.size();
  }

private:
  static vector<long> IDs; /*!< list of types spawned to take turns */
  static int currIndex; /*!< current active entity ID */
  long myCounterID; /*!< This entity's counter ID */
};

template<typename T> vector<long>
  CounterTrait<T>::IDs = vector<long>();

  template<typename T> int
    CounterTrait<T>::currIndex = 0;
