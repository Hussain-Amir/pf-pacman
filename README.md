# Pac-Man (C++ and SFML 3)

A Pac-Man game written in C++ with SFML 3. The maze is loaded from a text file. Eat all the pellets, avoid four ghosts, and use power pellets to turn the ghosts blue and eat them.

Semester project for CS1002 Programming Fundamentals, FAST-NUCES Lahore, Fall 2026.

<img width="1329" height="686" alt="gameimage" src="https://github.com/user-attachments/assets/4fb14e10-e2f9-4808-921f-03b9cd63a0d7" />

## Team Members

| Name | Roll Number |
| Hussain Amir | 26L-0004 |
| Hussain Ahmed Durrani | 26L-0024 |



## Controls

| Key | Action |
|---|---|
| Arrow keys or W / A / S / D | Move the player (in the menu: choose an item) |
| Enter | Select a menu item, or go back to the menu from the end screen |
| Esc | Save the game and go back to the menu |
| M | Show or hide the distance map (debug view) |

## Features

- Maze loaded from `maze.txt`, with an error message if the file is missing
- Player movement, pellets and power pellets, score and lives
- Four ghosts: two chasers and two wanderers
- Blue mode: ghosts turn blue for 5 seconds and can be eaten for 200 points
- Main menu with New Game, Continue and Exit
- Save and continue with `savegame.txt`
- Bonus: ghost 0 uses a distance map to find the shortest path to the player

## How to Run

1. Install Visual Studio and SFML 3.0.2, and set up SFML in the project (see the official SFML and Visual Studio tutorial).
2. Open `pf_pacman.slnx` in Visual Studio.
3. Build and run the project.
4. `maze.txt` and `arial.ttf` must be in the project folder, next to `pf_pacman.vcxproj`.
