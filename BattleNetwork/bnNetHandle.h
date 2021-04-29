#pragma once
#include <memory>
#include <assert.h>

class NetManager;

class NetHandle {
  friend class Game;

private:
  static NetManager* net;

public:
  NetManager& Net() { 
    assert(net != nullptr && "net manager was nullptr!");  
    return *net; 
  }

  // const-qualified functions

  NetManager& Net() const {
    assert(net != nullptr && "net manager was nullptr!");
    return *net;
  }
};