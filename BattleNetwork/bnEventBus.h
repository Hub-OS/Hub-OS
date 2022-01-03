#pragma once

#include <list>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>

#ifndef __APPLE__
#include <any>
#define any_cast std::any_cast
using any = std::any;
#else
#include "stx/any.h"
#define any_cast stx::any_cast
using any = stx::any;
#endif

class Scene; // forward decl

namespace {
  // utility function to compare the input type with std::any
  auto any_compare = [](auto&& i) {
    return [i](auto&& val) {
      return typeid(i) == val.type() && any_cast<decltype(i)>(val) == i;
    };
  };
}

/*! \file  bnEventBus.h
 *  \brief Emit functions to registered objects anywhere in the game
 *
 * How It Is Organized:
 * This event bus is a little barbaric but functional and fufills its purpose.
 * EventBus delcares a hash of recievers in the global memory space.
 * The recievers themselves are another hash of typename strings and a list of std::any objects.
 * When a scene registers an object will be placed into the list identified by the typename hash.
 * Each scene can make a unique channel that divides the recievers by Scene addresses in memory.
 * 
 * How It Works:
 * When an event is emitted, the reciever hash will be identified by the current scene address
 * represented by the Channel class. If one exists, the typename of the function-owning class
 * is inspected for a list. If the list is found, an iterative lookup will try and find the
 * correct std::any object and cast it before invoking the requested member function with the 
 * supplied arguments.
 * 
 * Caveats:
 * This event bus is in the global memory space and is not threadsafe nor is it safe from
 * invalid pointers and segfaults that come with them. 
 * 
 * If a registered object is deleted or goes out of scope, and not dropped beforehand, the event bus will crash
 * 
 * A wrapper class of some type e.g. Reciever<T> could be used as CRTP to remove itself from the 
 * recievers in the deconstructor and only those types could be registered to prevent nullptrs
 */

class EventBus final {
  //!< Recievers represents all channels and objects lists that may recieve events
  static inline std::map<const Scene*, std::map<std::string, std::vector<any>>> receivers;

public:
  class Channel {
    //!< The Scene address this channel represents
    // It is assumed the lifetime will live on beyond the channel and
    // the address represents a unique scene ptr
    const Scene* scene{ nullptr }; 

    /**
    * @brief Attempts to insert an item, obj of type T*, into the typename hash
    * @param map typename hash
    * @param typeStr the typid of obj as std::string
    * @param obj to be added into the list
    * 
    * If the typename hash does not exist, it will create one
    * If the item is discovered in the list of objects, it will not be re-added
    */
    template<typename T>
    void try_insert_item(std::map<std::string, std::vector<any>>& map,
      std::string typeStr, T* obj) {
      if (obj == nullptr) return;

      auto& vec = map[typeStr];

      auto vecIter = std::find_if(vec.begin(), vec.end(), ::any_compare(obj));

      if (vecIter == vec.end()) {
        vec.push_back(obj);
      }
    }

    /**
    * @brief Attempts to remove an item, obj of type T*, out of the typename hash
    * @param map typename hash
    * @param typeStr the typid of obj as std::string
    * @param obj to be removed into the list
    *
    * If the typename hash does not exist, it will create one
    * If the item is discovered in the list of objects, it is erased
    */
    template<typename T>
    void try_remove_item(std::map<std::string, std::vector<any>>& map,
      std::string typeStr, T* obj) {
      if (obj == nullptr) return;

      auto& vec = map[typeStr];

      auto vecIter = std::find_if(vec.begin(), vec.end(), ::any_compare(obj));

      if (vecIter != vec.end()) {
        vec.erase(vecIter);
      }
    }

  public:
    Channel() = delete;

    /**
    * @brief constructs a new channel using the scene pointer to differentiate it from other channels
    * @param s of type Scene*
    */
    Channel(const Scene* s) : scene(s) {
      if (scene == nullptr) return;

      auto mapIter = receivers.find(scene);

      if (mapIter == receivers.end()) {
        receivers.insert(std::make_pair(s, std::map<std::string, std::vector<any>>()));
      }
    }

    /**
    * @brief copies a channel
    */
    Channel(const Channel& rhs) : scene(rhs.scene) { }

    /**
    * @brief Registers a varadic list of object pointers subscribing for events
    * @param args of type typename... Args
    * @warning if an object is deleted or goes out of scope, it needs to be Drop()-ed beforehand
    */
    template<typename... Args>
    void Register(Args*... args) {
      if (scene == nullptr) return;

      auto mapIter = receivers.find(scene);

      if (mapIter != receivers.end()) {
        auto& typenameMap = mapIter->second;
        (try_insert_item(typenameMap, typeid(args).name(), args), ...);
      }
    }

    /**
    * @brief Drops a varadic list of object pointers
    * @param args of type typename... Args
    * 
    * if no such object exists in the channel, nothing will happen for that object
    */
    template<typename... Args>
    void Drop(Args*... args) {
      if (scene == nullptr) return;

      auto mapIter = receivers.find(scene);

      if (mapIter != receivers.end()) {
        auto& typenameMap = mapIter->second;
        (try_remove_item(typenameMap, typeid(args).name(), args), ...);
      }
    }

    /**
    * @brief Clears the typename hash which in turn forgets about all subscribed object pointers
    * 
    * Useful for scenes that are about to be destroyed in memory
    */
    void Clear() {
      auto mapIter = receivers.find(scene);

      if (mapIter != receivers.end()) {
        mapIter->second.clear();
      }
    }

    /**
    * @brief For all object pointers that match the function signature, invokes the function with arguments
    * @param Func, member-function pointer
    * @param args of type typename... Args
    * @warning If there is a dangling invalid obj pointer in the channel, this will crash
    * 
    * Because Channels are separated by their respective Scene addresses, only those objects registered
    * to that channel will invoke their matching functions.
    */
    template<typename R, typename Class, typename... FuncArgs, typename... Args>
    void Emit(R(Class::* Func)(FuncArgs...), Args&&... args) const {
      auto mapIter = receivers.find(scene);

      if (mapIter != receivers.end()) {
        std::string typeStr = typeid(Class*).name();
        auto typenameIter = mapIter->second.find(typeStr);

        if (typenameIter != mapIter->second.end()) {
          for (auto any : typenameIter->second) {
            (any_cast<Class*>(any)->*Func)(std::forward<decltype(args)>(args)...);
          }
        }
      }
    }
  };
};

#undef any_cast