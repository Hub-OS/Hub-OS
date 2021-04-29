#pragma once 
#include <Poco/Net/IPAddress.h>
#include <Poco/Buffer.h>

class IPacketProcessor {
private:
  Poco::Net::IPAddress watch;

public:
  IPacketProcessor(const Poco::Net::IPAddress& watchFor) : watch(watchFor)
  { }

  virtual ~IPacketProcessor() { }
  virtual void OnPacket(const Poco::Buffer<char>& buffer) = 0;
  virtual void Update(double elapsed) = 0;

  Poco::Net::IPAddress& Watching() { return watch; }
};