#pragma once
#include "bnScriptType.h"
#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>

using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::string;

class ScriptResourceManager {
private:
  std::map<ScriptType, std::string> scripts;

public:
  void AddToPaths();

  static ScriptResourceManager& GetInstance() {
    static ScriptResourceManager* instance = new ScriptResourceManager();

    return *instance;
  };