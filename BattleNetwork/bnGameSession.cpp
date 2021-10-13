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

const std::string WebClientManager::GetNick() const
{
  return nickname;
}