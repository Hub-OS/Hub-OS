# Architecture

The goal of this document is to provide a quick overview of the project for contributors get started. This concept is taken from [here](https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html).

## The Event Loop

The heart of the server is the event loop. When calling Server.start(), the server waits for events to dispatch to plugins.

The server tries to handle the necessities and gives plugins control over game logic. The server may handle some functionality of packets such as updating player position during movement or entirely handling packets such as responding to a ping and storing streamed data of a player asset to use later. The server won't handle game logic such as what should happen if the player steps on a tile, talks to another player, or is somehow floating in the void.

### Diagram of data and events flowing through the server:

```
Server.start():

          (client)
             |
           Packet
             |
    _________v__________       __________________
   /                    \     /                  \
   |  Listening Thread  |     |   Clock Thread   |
   \____________________/     \__________________/
             |                          |
     Processed Packet                  Tick
             |                          |
         ____V__________________________v____
        /                                    \
        |             Event Loop             |
        \____________________________________/
                          |
                        Event
                          |
                    ______v______
                   /             \
                   |   Plugins   |
                   \_____________/
                          |
                       Response
                          |
                     _____v_____
                    /           \
                    |    Net    |
                    \___________/

```

# Net

That end bit in the diagram is the Net. It stores data for the whole world: maps, areas, assets, characters. When a plugin wants to make a change, it can do so by making an update to the Net.
