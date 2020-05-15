#!/bin/bash

gcc game_main.c game_system.c game_window.c game_system.h -o game_main -L/usr/lib -lGL -lm -lSDL -lSDL_mixer -lSDL_image -lSDL_ttf `pkg-config --cflags --libs opencv`
