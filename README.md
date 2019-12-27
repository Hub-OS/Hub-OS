
# Author TheMaverickProgrammer
This aims to be an accurate mmbn6 engine that can be used to program custom enemies, chips, navis, or used to make your own mmbn story. It includes a fully playable battle-rush game.

This was originally started to kill some time one summer and I got a little carried away. It's been fun and I hope you have fun with it as I have had making it.

## Update 12/27/2019
[![forte-thumbnail.png](https://i.postimg.cc/bNvt3xP3/forte-thumbnail.png)](https://streamable.com/cxp7l#)

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

