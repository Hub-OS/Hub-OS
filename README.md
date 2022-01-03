# WARNING - THIS BRANCH IS OUTDATED
Please `git checkout development` for the latest, greatest, and (maybe) unstable features.

This main branch is over a year old!

New `development` branch can be found [here](https://github.com/TheMaverickProgrammer/OpenNetBattle/tree/development)

# About This Project
Follow this project on :bird: [Twitter](https://twitter.com/OpenNetBattle)!

This aims to be an accurate mmbn6 battle engine that can be used to program custom enemies, chips, navis, or used to make your own mmbn story. It includes a fully playable battle-rush game.

This was originally started to kill some time one summer and I got a little carried away. It's been fun and I hope you have fun with it as I have had making it.

## Update 7/17/2021
[New Build Instructions On YouTube!](https://www.youtube.com/watch?v=5T_kS7DYbvw)

## Update 12/27/2019
[![forte-thumbnail.png](https://i.postimg.cc/bNvt3xP3/forte-thumbnail.png)](https://streamable.com/cxp7l#)
[![thumbnail.png](https://i.postimg.cc/dVNffmg6/thumbnail.png)](https://streamable.com/pmy2d)
[![thumbnail.png](https://i.postimg.cc/pLmCmN1Q/thumbnail.png)](https://twitter.com/i/status/1175981132912975872)
[![thumbnail.png](https://i.postimg.cc/8cnWGcG2/thumbnail.png)](https://twitter.com/i/status/1185687172868956161)

_some videos may be a little old_

Lots of updates:
- New playable characters
- Customizable forms and form menu
- New enemies
- New chips and `ChipAction` system
- New lifecycle callbacks: `OnSpawn`, `OnDelete`, `OnUpdate` allow more control for custom content
- Battle-step routines will have more expected outcomes even with customization
- Less fickle movement system
- Entity & Tile queries
- Hitbox types for sharing damage across multiple targets on the field
- A single entity can also share life cycle events across other entities to create big enemies with multiple parts (like Alpha or Stone man)
- Support chips
- No more 3rd party configuration -> we have an options/ joystick config screen now
- Folder naming, editing, and deleting screens are complete
- Different AI pattern templates
- Bunch of other things

# Wiki
TODO

# Controls
These are the default bindings. The engine supports 1 joystick.

```
ARROWS -> Move / UI options
Z      -> Shoot (hold to charge) / UI Cancel
X      -> Use chip / UI Confirm
A      -> Bring up chip select GUI
D      -> Scan Left
F      -> Scan Right
RETURN -> Pause
SPACE  -> Quick Option
```

# Contributions to the project
TODO: add contributors list
