#pragma once
#include <Swoosh/Timer.h>

/*! \brief Represents a cached resource of type T. It knows how frequently it is used.
*/
template<typename T>
class CachedResource {
  long long lastRequestTime; //!< Used to determine if we should remove this resource
  unsigned int useCount; //!< Higher useCount means this is frequently used

  using SharedPtrType = std::shared_ptr<std::remove_pointer_t<T>>;
  SharedPtrType resource; //!< shared ptr to the actual resource
public:

  /*! \brief Constructor creates a shared resouce, pauses the timer, and sets count to 0*/
  CachedResource(const T& item) : resource(std::make_shared(item)) {
    lastRequestTime = CurrentTime::AsMilli();
    useCount = 0;
  }

  CachedResource(const SharedPtrType& ref) : resource(ref) {
    lastRequestTime = CurrentTime::AsMilli();
    useCount = 0;
  }

  /*! \brief returns use count*/
  const unsigned int GetUseCount() const {
    return useCount;
  }

  /*! \brief checks if this resource is still in use*/
  const bool IsInUse() const {
    return resource.use_count() >= 1;
  }

  /*! \brief returns the resource 
    This function also increases the use count and resets
    the "last requested" timer.
  */
  SharedPtrType GetResource() {
    useCount++;
    lastRequestTime = CurrentTime::AsMilli();
    return resource;
  }

  /*! \brief return the seconds since this item was last requested */
  const float GetSecondsSinceLastRequest() {
    return (CurrentTime::AsMilli() - lastRequestTime)/1000.0f;
  }
};