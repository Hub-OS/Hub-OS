#include "bnBlockPackageManager.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include "stx/zip_utils.h"
#include <exception>
#include <atomic>
#include <thread>

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedBlock.h"
#endif

BlockMeta::BlockMeta() :
  PackageManager<BlockMeta>::Meta<PlayerCustScene::Piece>()
{ 
}

BlockMeta::~BlockMeta() {
}

void BlockMeta::SetName(const std::string& name)
{
  BlockMeta::name = name;
}

void BlockMeta::SetDescription(const std::string& desc)
{
  description = desc;
}

void BlockMeta::SetColor(size_t colorIdx)
{
  color = colorIdx;
}

void BlockMeta::SetShape(std::vector<uint8_t> values)
{
  std::swap(shape, values);
}

void BlockMeta::SetMutator(const std::function<void(Player& player)>& mutator)
{
  this->mutator = mutator;
}

void BlockMeta::AsProgram()
{
  isProgram = true;
}
