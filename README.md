Snake Game (C++ & SFML)
Project Overview

This project is a custom-developed Snake Game implemented in C++ using the SFML (Simple and Fast Multimedia Library) framework. The game features a modern user interface, sound effects, multiple levels, enemy mechanics, bonus food, and interactive menu systems.

The project demonstrates core concepts of structured programming, including game loops, event handling, file input/output, collision detection, and multimedia integration in C++.

Course: Structured Programming
Semester: 1st Year, 2nd Semester
Submitted by: Shahriar Islam

Project Structure and Assets
SnakeGame/
│
├── SnakeGame.cpp                 Main game source code
├── SnakeGame.slnx                Visual Studio solution file
├── SnakeGame.vcxproj             Visual Studio project file
├── SnakeGame.vcxproj.filters     Visual Studio filters
├── README.md                     Project documentation
├── .gitignore                    Git ignore rules
│
├── audios/                       Game audio assets
│   ├── music.ogg                 Menu music
│   ├── gameplay.ogg              In-game music
│   ├── gameover.ogg              Game over sound
│   └── crash.ogg                 Crash sound effect
│
├── images/                       Game images and textures
│   ├── menu_bg.png               Main menu background
│   ├── level1_bg.png             Level 1 background
│   ├── level2_bg.png             Level 2 background
│   ├── level3_bg.png             Level 3 background
│   ├── wall.png                  Wall texture
│   ├── Apple.png                 Normal food
│   ├── Bonus.png                 Bonus food
│   ├── bad.png                   Shrink / penalty food
│   ├── enemy.png                 Enemy sprite sheet
│   └── gameover_bg.png           Game over background
│
├── fonts/                        Fonts used in the game
│   └── snake.ttf                 Game font
│
├── txt/                          Text-based data files
│   ├── highscore.txt             Best score
│   └── highscores.txt            Top score list
│
├── x64/                          Build output (ignored by Git)
│   └── Debug/                    Debug binaries and copied assets
│
└── .vs/                          Visual Studio cache (ignored by Git)

Build and Run Instructions:
Requirements-
1.C++17 or later
2.SFML 2.5 or 2.6
3.Supported platforms: Windows and Linux

Linux (Ubuntu / Debian)

Install SFML:
sudo apt update
sudo apt install libsfml-dev

Compile:
g++ SnakeGame.cpp -o SnakeGame \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio


Run:
./SnakeGame

Windows (Visual Studio)
1.Install SFML and configure it in Visual Studio
2.Link required SFML libraries
3.Set the Working Directory to $(TargetDir)
4.Asset files are copied automatically using post-build events

Controls
 1.Arrow Keys: Move the snake

   P: Pause / Resume
   R: Restart after Game Over
   M: Return to Main Menu
   F11: Toggle Fullscreen
   V: Toggle VSync (Settings menu)
   ESC / 0: Back or Exit

Important Notes
Do not rename or move asset files.
The following directories must remain in the project root:
audios/, images/, fonts/, txt/
File names are case-sensitive on Linux.
Ensure the working directory is correctly set before running the game.

Features

Multiple levels with unique backgrounds
Enemy AI with animated sprites
Bonus food and score multipliers
Screen shake and particle effects
Pause menu with blur effect
Settings menu (Volume, VSync, Fullscreen)
High score saving system

Contact Information

Name: Shahriar Islam
University: University of Dhaka
Email: shahriar-2023216004@cs.du.ac.bd