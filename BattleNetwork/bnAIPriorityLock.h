#pragma once
#include <type_traits>

/*! \brief Metatemplate object that deduces if an AI<Any> type object has a function to unlock or lock
*
* Used by AIStates that may be shared by BossAI<> types who do not have PriorityLock() and PriorityUnlock() states
*/
namespace AIPriorityDetail {
  // Locker
  template<typename Dummy, class Enabled = void>
  struct AIPriorityLock_t {
    AIPriorityLock_t(Dummy& dummy) { 
      dummy.LockState();
    }
  };

  template<typename Dummy> 
  struct AIPriorityLock_t<Dummy, 
    typename std::enable_if<!std::is_same<decltype(&Dummy::PriorityLock), void(Dummy::*)()>::value>::type> {
    AIPriorityLock_t(Dummy& dummy) {
      dummy.PriorityLock();
    }
  };

  // Unlocker
  template<typename Dummy, class Enabled = void>
  struct AIPriorityUnlock_t {
    AIPriorityUnlock_t(Dummy& dummy) { 
      dummy.UnlockState();
    }
  };

  template<typename Dummy>
  struct AIPriorityUnlock_t<Dummy,
    typename std::enable_if<!std::is_same<decltype(&Dummy::PriorityUnlock), void(Dummy::*)()>::value>::type> {
    AIPriorityUnlock_t(Dummy& dummy) {
      dummy.PriorityUnlock();
    }
  };
}

template<typename Type>
using AIPriorityLock = AIPriorityDetail::AIPriorityLock_t<Type>;

template<typename Type>
using AIPriorityUnlock = AIPriorityDetail::AIPriorityUnlock_t<Type>;