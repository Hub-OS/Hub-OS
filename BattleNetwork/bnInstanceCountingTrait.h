#pragma once

#include <vector>

using namespace std;

template<typename T>
class InstanceCountingTrait {
protected:

  InstanceCountingTrait() {
    myCounterID = (long)IDs.size();
    IDs.push_back(myCounterID);
  };

  virtual ~InstanceCountingTrait() {
    RemoveInstanceFromCountedList();
  }

  /**
 * @brief Used in states, if this entity is last in the list
 * @return
 */
  const bool IsLast() const {
    return IDs[0] == myCounterID;
  }

  static const int GetCounterSize() {
    return (int)IDs.size();
  }

private:
  static vector<long> IDs; /*!< list of types spawned to take turns */
  static int currIndex; /*!< current active entity ID */
  long myCounterID; /*!< This entity's counter ID */

  void RemoveInstanceFromCountedList() {
    if (IDs.size() > 0) {
      vector<long>::iterator it = find(IDs.begin(), IDs.end(), myCounterID);

      if (it != IDs.end()) {
        // Remove me
        IDs.erase(it);
      }
    }
  }
};

template<typename T> vector<long>
 InstanceCountingTrait<T>::IDs = vector<long>();

 template<typename T> int
 InstanceCountingTrait<T>::currIndex = 0;
