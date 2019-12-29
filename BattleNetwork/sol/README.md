# About
Sol2 is used as the Lua/C++ wrapper in the engine

# Dependencies
Downloaded from [Sol v2.20.1](https://github.com/ThePhD/sol2/releases/tag/v2.20.1)
Uses **Lua 5.3.5** as a git submodule

# Compilation
## Visual Studio
* Add lua to include directories.
* Include all lua's `.c` files as part of the project except for the main-entry file `luac.c`
* Use C++17 language support
