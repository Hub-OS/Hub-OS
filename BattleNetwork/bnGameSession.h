class GameSession {
    using KeyItem = std::pair<std::string, std::string>;
    std::vector<std::string> cardPool;
    std::vector<KeyItem> keyItems; 
    uint32_t monies{}; //!< Monies on the account
    std::string nickname;
    
public:
    const bool LoadSession(const std::string& inpath);
    void SaveSession(const std::string& outpath);
    void SetKey(const std::string& key, const std::string& value);
    void SetNick(const std::string& nickname);
    const std::string GetValue(const std::string& key);
    const std::string GetNick();
}