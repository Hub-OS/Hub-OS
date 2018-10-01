#!/bin/bash

g++ -std=gnu++0x -c -I extern/includes BattleNetwork/*.cpp -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
g++ *.c -o BattleNetwork
rm *.o

