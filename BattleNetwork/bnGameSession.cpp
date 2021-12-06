#include "bnGameSession.h"
#include "netplay/bnBufferWriter.h"
#include "netplay/bnBufferReader.h"

const bool GameSession::LoadSession(const std::string& inpath)
{
  monies = 0;
  cardPool.clear();
  keys.clear();
  keyItems.clear();

  BufferReader reader;
  std::fstream file;
  file.open(inpath, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return false;

  size_t file_begin = file.tellg();
  file.seekg(0, std::ios::end);
  size_t file_end = file.tellg();
  size_t file_len = file_end - file_begin;
  Poco::Buffer<char> buffer(file_len);
  file.seekg(0, std::ios::beg);
  file.read(buffer.begin(), file_len);

  nickname = reader.ReadTerminatedString(buffer);

  size_t keys_len = reader.Read<size_t>(buffer);

  while (keys_len-- > 0) {
    std::string key = reader.ReadString<char>(buffer);
    std::string val = reader.ReadString<char>(buffer);
    SetKey(key, val);
  }

  size_t pool_len = reader.Read<size_t>(buffer);

  while (pool_len-- > 0) {
    std::string card = reader.ReadTerminatedString(buffer);
    cardPool.push_back(card);
  }

  file.close();

  return true;
}

void GameSession::SaveSession(const std::string& outpath)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer(0);
  std::fstream file;
  file.open(outpath, std::ios::out | std::ios::binary);

  if (!file.is_open())
    return;

  writer.WriteTerminatedString(buffer, nickname);

  size_t num_of_keys = keys.size();
  writer.Write(buffer, num_of_keys);

  for (auto& [k, v] : keys) {
    writer.WriteString<char>(buffer, k);
    writer.WriteString<char>(buffer, v);
  }

  size_t pool_len = cardPool.size();
  writer.Write(buffer, pool_len);

  for (auto& card : cardPool) {
    writer.WriteTerminatedString(buffer, card);
  }

  char* raw = new char [buffer.size()];
  std::memcpy(raw, buffer.begin(), buffer.size());
  std::string as_string = std::string(raw, buffer.size());
  file << as_string;
  file.close();
  delete[] raw;
}

void GameSession::SetKey(const std::string& key, const std::string& value)
{
  auto iter = keys.find(key);

  if (iter != keys.end()) {
    keys[key] = value;
  }
  else {
    keys.insert(std::make_pair(key, value));
  }
}

void GameSession::SetNick(const std::string& nickname) {
  this->nickname = nickname;
}

const std::string GameSession::GetValue(const std::string& key)
{
  std::string res;

  auto iter = keys.find(key);
  if (iter != keys.end()) {
    res = iter->second;
  }

  return res;
}

const std::string GameSession::GetNick() const
{
  return nickname;
}