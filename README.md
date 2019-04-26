
# Author TheMaverickProgrammer

## Update 3/2/2019
[![Boss Battle](https://media.giphy.com/media/1eEv6U6IzFpFFfncDT/giphy.gif)](https://youtu.be/8EIgjohRZ_c?t=375)

(Click the gif to see a video)

Lots of updates: A full fledged scene node system, defense checks, an aggregate component system, components-as-an-entity, components that modify battle steps, multiple chip implementations, multiple enemy implementations, folders, ... the video says more than I can.

# Videos
#### Branch: master
Click the thumbnail to watch on youtube. 

[![Video of engine 3/2/2019](http://i3.ytimg.com/vi/8EIgjohRZ_c/hqdefault.jpg)](https://youtu.be/8EIgjohRZ_c)

# Screenshots



[![starman.png](https://s8.postimg.cc/qjbob7h2d/starman.png)](https://postimg.cc/image/3umhbmzoh/)

[![image.png](https://s8.postimg.cc/sjtcjqrol/folder.png)](https://s8.postimg.cc/sjtcjqrol/folder.png)

[![preview.png](https://s15.postimg.cc/6cpgwlocr/preview.png)](https://postimg.cc/image/phsq6d30n/)

[![menu.png](https://s15.postimg.cc/k819ndp6z/Untitled.png)](https://postimg.cc/image/hdy49xn0n/)

# Wiki
How to [compile](https://github.com/TheMaverickProgrammer/battlenetwork/Begin-Here)
Care to [contribute](https://github.com/TheMaverickProgrammer/battlenetwork/wiki)? 

# Features

**Keyboard support for MMBN Chrono X Config .ini files**
[![tool.png](https://s15.postimg.cc/hdqmp92i3/tool.png)](https://postimg.cc/image/wmgk30w6f/)

Just copy and paste your `options.ini` file to the same folder as the executable and the engine will read it. Plug in your controller. You'll know if everything is good because the GamePad icon will show up on the title screen:

[![gamepad_support.png](https://s15.postimg.cc/nmm2cu7ij/gamepad_support.png)](https://postimg.cc/image/ib75s4lfr/)

There is joystick support but the tricky thing about joysticks are that each vendor has different configurations. If you have a problem with your joystick, file an issue on the project page [here](https://github.com/TheMaverickProgrammer/battlenetwork/issues). 

--------

In this demo, you can choose your Navi, choose which mob to battle, move navi around, shoot, charge, and delete enemies on the grid. When the chip cust is full, you can bring up the chip select menu. 

The player can select chips and deselect them in the order they were added.  Return to battle and you can use the chips by pressing Right-Control. 

There is 1 Program Advance: XtremeCannon. Can be activated by selecting `Cannon A` + `Cannon B` + `Cannon C` in order. It deals a whopping 600 points of damage, shaking the screen, and attacks the first 3 enemies vertically.
There other other PAs that can be triggered through system but are not implemented and do not do any damage. 
You can write your own PA's and add your own chips by editting the `/database` textfiles.

Mega can also be deleted, ending the demo. If mega wins, the battle results will show up with your time, ranking, and a random chip based on score.

# Controls
If not using [MMBN Chrono X Config Utility](http://www.mmbnchronox.com/download.php), these are the default bindings

```
ARROWS -> Move
SPACE  -> Shoot (hold to charge)
P      -> Pause / Chip description 
Return -> Bring up chip select GUI / Continue / Spread chips preview
R CTRL -> Use a chip
```

## Contributions to the project
Pheelbert wrote the original tile-based movement code where the entity kept a pointer reference to his current and next tile. This was a fitting design choice for a mmbn clone. Since then I have gone on to add multiple systems, components, routines, and so much more. It's unrecognizable as anything else but fresh and new.
