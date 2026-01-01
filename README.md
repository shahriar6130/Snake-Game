# 🐍 Snake Game

Snake Game is a **C++ and SFML–based arcade game** developed as part of an academic project.  
The project demonstrates the practical application of **structured programming**, **game logic design**, and **multimedia handling** using the **SFML graphics and audio library**.

---

## 📌 Project Overview

Snake Game is designed to provide a modern and interactive version of the classic Snake game.  
It includes multiple levels, animated enemies, background music, sound effects, and a complete menu-driven interface.

The game focuses on:
- Structured and modular C++ programming
- SFML-based graphics and audio handling
- Event-driven input processing
- File handling for score persistence
- Performance-friendly rendering techniques

## 🎮 Game Features

- 🧭 Multiple playable levels with unique backgrounds  
- 🐍 Smooth snake movement and growth mechanics  
- 🍎 Normal food, bonus food, and negative food items  
- 👾 Animated enemy with collision detection  
- 💥 Particle effects and screen shake on events  
- 🔊 Background music and sound effects  
- ⏸️ Pause, resume, and settings menu  
- 🏆 High score saving using text files  
- 🖥️ Fullscreen and VSync support  

---

## 🛠️ Technologies Used

- **Programming Language:** C++  
- **Graphics & Audio Library:** SFML (2.5 / 2.6)  
- **IDE:** Visual Studio / g++  
- **Platform:** Windows & Linux  

---

## 📂 Project Structure

SnakeGame/
│
├── SnakeGame.cpp  
    └──(Main game source code)               
├── SnakeGame.slnx 
    └──(Visual Studio solution file)
├── SnakeGame.vcxproj 
    └──(Visual Studio project file)
├── SnakeGame.vcxproj.filters  
    └──(Visual Studio filters)

├── README.md 
    └── (Project documentation)
├── .gitignore 
     └── (Git ignore rules)
│
├── audios/
    └──(Game audio assets)
│   ├── music.ogg
           └── (Menu background music)
│   ├── gameplay.ogg 
           └──(In-game music)
│   ├── gameover.ogg 
           └──(Game over sound)
│   └── crash.ogg
           └──(Crash sound effect)
│
├── images/ 
        └──(Game images and textures)
│   ├── menu_bg.png 
        └──(Main menu background)
│   ├── level1_bg.png            
         └──(Level 1 background)
│   ├── level2_bg.png 
         └──(Level 2 background)
│   ├── level3_bg.png
         └──(Level 3 background)
│   ├── wall.png
         └──(Wall texture)
│   ├── Apple.png 
         └──(Normal food sprite)
│   ├── Bonus.png
         └──(Bonus food sprite)
│   ├── bad.png 
         └──(Shrink / penalty food)
│   ├── enemy.png 
         └──(Enemy sprite sheet)
│   └── gameover_bg.png           
         └──(Game over background)
│
├── fonts/                       
│   └── snake.ttf 
         └──(Game font)
│
├── txt/  
                        
│   ├── highscore.txt  
          └──(Best score)
│   └── highscores.txt            
           └──(Top scores list)
│
├── x64/                         
│   └── Debug/ 
           └──(Debug binaries and auto-copied assets)                 
│
└── .vs/                          
      └──(Visual Studio cache (ignored by Git))


## Install SFML:
sudo apt update
sudo apt install libsfml-dev

## Compile:
g++ SnakeGame.cpp -o SnakeGame \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

## Run:
./SnakeGame

Windows (Visual Studio)
1.Install SFML and configure it in Visual Studio
2.Link required SFML libraries
3.Set the Working Directory to $(TargetDir)
4.Asset files are copied automatically using post-build events

## 🎮 Controls

Arrow Keys → Move the snake

P → Pause / Resume

R → Restart after Game Over

M → Return to Main Menu

F11 → Toggle Fullscreen

V → Toggle VSync (Settings Menu)

ESC / 0 → Back or Exit

## Important Notes
Do not rename or move asset files.
The following directories must remain in the project root:
audios/, images/, fonts/, txt/
File names are case-sensitive on Linux.
Ensure the working directory is correctly set before running the game.

## Contact Information
Name: Shahriar Islam
University: University of Dhaka
Email: shahriar-2023216004@cs.du.ac.bd