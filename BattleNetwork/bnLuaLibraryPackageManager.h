/*! \file bnLuaLibraryPackageManager.h */

#pragma once

#include "bnPackageManager.h"

class LuaLibraryImpl
{
    public:
    virtual ~LuaLibraryImpl() {};
};

struct LuaLibraryMeta final : public PackageManager<LuaLibraryMeta>::Meta<LuaLibraryImpl> {};
class LuaLibraryPackageManager : public PackageManager<LuaLibraryMeta> {
public:
  LuaLibraryPackageManager(const std::string& ns) : PackageManager<LuaLibraryMeta>(ns) {}
};
class LuaLibraryPackagePartition : public PartitionedPackageManager<LuaLibraryPackageManager> {};