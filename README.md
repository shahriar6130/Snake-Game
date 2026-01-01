
🎮 Snake Game (C++ & SFML)

🔹 Project Overview

This is a custom-developed **Snake Game** built using **C++** and the
**SFML (Simple and Fast Multimedia Library)** framework.  
The game features a modern UI, sound effects, multiple levels, enemy
mechanics, bonus food, and interactive menus.

The project demonstrates core concepts of **structured programming**,
game loops, event handling, file I/O, and multimedia integration in C++.

🔹 Course       : Structured Programming  
🔹 Semester     : 1st Year, 2nd Semester  
🔹 Submitted by : Shahriar Islam  
🔹 Roll         : 61  

📁 Project Structure & Assets

Source Code:
- SnakeGame.cpp          → Main game logic and mechanics

Images:
- menu_bg.png            → Main menu background
- level1_bg.png          → Level 1 background
- level2_bg.png          → Level 2 background
- level3_bg.png          → Level 3 background
- gameover_bg.png        → Game Over background
- Apple.png              → Food sprite
- Bonus.png              → Bonus food sprite
- bad.png                → Negative / shrink food
- wall.png               → Wall texture
- enemy.png              → Enemy sprite sheet

Audio:
- music.ogg              → Menu background music
- gameplay.ogg           → In-game music
- gameover.ogg           → Game over sound
- crash.ogg              → Crash sound effect

Fonts:
- snake.ttf              → Game font

Data Files:
- txt/highscore.txt      → Stores highest score
- txt/highscores.txt     → Stores top scores list


🛠️ How to Build & Run

🔹 Requirements
- C++17 or later
- SFML 2.5 or 2.6
- Windows / Linux supported

🔹 Linux (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install libsfml-dev

🔹 Compile (Linux):
g++ SnakeGame.cpp -o SnakeGame \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

🔹 Run:
./SnakeGame

🔹 Windows (Visual Studio):

   Install SFML

   Link SFML libraries

   Set Working Directory to $(TargetDir)

   Assets are copied automatically via Post-Build events

🎮 Controls

   Arrow Keys → Move snake

   P → Pause / Resume

   R → Restart after Game Over

   M → Return to Main Menu

   F11 → Toggle Fullscreen

   V → Toggle VSync (Settings menu)

   ESC / 0 → Back / Exit

⚠️ Important Notes

   Do NOT rename or move asset files.

   Folder structure must remain intact:
   audios/, images/, fonts/, txt/

   File names are case-sensitive on Linux.

   Ensure the working directory is set correctly when running the game.


✨ Features

✔ Multiple levels with unique backgrounds
✔ Enemy AI with animation
✔ Bonus food & score multipliers
✔ Screen shake & particle effects
✔ Pause menu with blur effect
✔ Settings menu (Volume, VSync, Fullscreen)
✔ High score saving system

====================================================
📩 Contact

Name : Shahriar Islam
University : University of Dhaka
Email : shahriar-2023216004@cs.du.ac.bd