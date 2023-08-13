#!/bin/sh

g++ src/main.cpp src/Pokemon.cpp -Iinclude -o pokemonApp  -lcurl -lsqlite3 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network