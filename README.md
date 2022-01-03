# ❓ ABOUT
Follow this project on :bird: [Twitter](https://twitter.com/OpenNetBattle)!

This project aims to be an accurate mmbn6 battle engine that can be used to program custom enemies, chips, navis, or used to make your own mmbn story.
This was originally started to kill some time one summer and I got a little carried away. It's been fun and I hope you have fun with it as I have had making it.

# 🕹️ GET STARTED PLAYING THE GAME


# 🧩 GET STARTED WITH THE CODE
This project uses `master` branch to stage releases  before tagging the final version.
In-progress features and bug fixes go into the `development` branch and can be found [here](https://github.com/TheMaverickProgrammer/OpenNetBattle/tree/development)

You will need:
* [CMAKE](https://cmake.org/download/) 3.19.6 or higher
* [VCPKG](https://vcpkg.io/en/index.html) 
* a C++ compiler and toolchain

Over time, it was decided that content would be made faster and easier if scripted with lua and so the engine itself contains no game-play content and must be provided with external mod files.

The codebase aims to achieve excellent coding standards, memory saftey, and re-usable components. However, due to the balancing act required to release interesting content, some areas of code could use some touch-up. I have never made a network game before, let alone lockstep, and the project of this scale is new to me as well. There are many good ideas and some not-so-good ideas. I welcome any and all contributions to improve the design. For the most part, I'm very satisfied with the stability and magnitude of this project.

## Build instructions from 7/17/2021
[Build Instructions On YouTube!](https://www.youtube.com/watch?v=5T_kS7DYbvw)

Note that you will need to also install `fluidsynth` with vcpkg and the install guide video is not yet updated.

# 💡 FEATURES
- Scriptable
- Custom players and forms (battle and overworld)
- Custom chips and attacks (timefreeze, etc.)
- Custom enemies and mob arrangements
- Standard battle, liberation missions, and network PVP via lockstep (or make your own battle scenes with the state graph!)
- Supports online play from custom [servers](https://github.com/ArthurCose/Scriptable-OpenNetBattle-Server).

# 🧑🏼‍🤝‍🧑🏼 COMMUNITY 
View the contributor list [here](). I could not have made it this far without these special and very talented people!
Join the project official discord [here](https://discord.gg/yAK9MG2)

# ⚙️ CONTROLS
These are the default bindings. The engine supports 1 joystick. You can change these bindings from within the game's `Config` screen.

```
ARROWS -> Move / UI options
Z      -> Shoot (hold to charge) / UI Confirm / Interact
X      -> Use chip / UI Cancel / Run
C      -> Special ability / Misc. Action
A      -> Scan Left / Misc. Action
S      -> Scan Right / Bring up chip select GUI / Talk to navi
M      -> Display map
RETURN -> Pause
```

# Contributions to the project
TODO: add contributors list
