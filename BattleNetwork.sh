#!/bin/bash

g++ -std=c++17 -c -I extern/includes BattleNetwork/*.cpp -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
g++ *.c -o BattleNetwork
rm *.o

