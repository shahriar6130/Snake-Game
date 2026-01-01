
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <vector>
#include <deque>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <array>
#include <climits>
#include <iostream>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#endif


constexpr int   CELL_SIZE = 16;
constexpr int   WIDTH = 40;
constexpr int   HEIGHT = 30;
constexpr float INITIAL_DELAY = 0.15f;
constexpr float MIN_DELAY = 0.06f;
constexpr float DELAY_DECREMENT = 0.008f;
constexpr int   FOODS_PER_LEVEL = 5;
constexpr float BONUS_TIME = 5.0f;
constexpr int   BONUS_MAX_SCORE = 400;
constexpr int   MARGIN = 32;

constexpr int   MAX_LEVEL = 3;
const int LEVEL_UP_SCORES[MAX_LEVEL + 1] = { 0, 200, 500, INT_MAX };
constexpr int STARTING_SNAKE_LENGTH = 3;

// enemy animation constants (your sheet layout)
constexpr int   ENEMY_COLS = 7;
constexpr int   ENEMY_ROWS = 3;
constexpr float ENEMY_FRAME_DURATION = 0.1f;

enum Direction { Up, Down, Left, Right };
enum GameState { Playing, Paused, GameOver };
enum MenuState { MainMenu, InGame, PauseMenu, HighScoreMenu, MoodMenu, PickLevelMenu, SettingsMenu };
enum PlayMode { PickLevel, CycleLevel };

PlayMode playMode = PickLevel;

struct Enemy {
    sf::Vector2i pos;
    float moveTimer = 0.f;
    float moveDelay = 0.2f;
};

std::vector<Enemy> enemies;

// --- shrink arena / warnings ---
constexpr int MAX_SHRINK_TICKS = 5;
int shrinkTicks = 0;
const int SHRINK_FOOD_STEP = (FOODS_PER_LEVEL - 1) * 2;
int nextShrinkFood = SHRINK_FOOD_STEP;

bool warningActive = false;
int  warningCount = 0;
sf::Clock warningClock;

int minX, maxX, minY, maxY;

// --- enemy animation clock (FIX) ---
sf::Clock enemyAnimClock;

// --- screen shake (add-only) ---
float shakeTime = 0.f;
float shakeDuration = 0.20f;
float shakeMagnitude = 6.f;

static float frand(float a, float b) {
    return a + (b - a) * (float(rand()) / float(RAND_MAX));
}

// --- particles (add-only) ---
struct Particle {
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f vel{ 0.f, 0.f };
    float life = 0.f;
};

std::vector<Particle> particles;

void spawnParticles(sf::Vector2f center, int count) {
    particles.reserve(particles.size() + count);
    for (int i = 0; i < count; ++i) {
        Particle p{};
        p.pos = center;
        p.vel = { frand(-80.f, 80.f), frand(-120.f, -30.f) };
        p.life = frand(0.18f, 0.35f);
        particles.push_back(p);
    }
}

void updateParticles(float dt) {
    for (auto& p : particles) {
        p.life -= dt;
        p.vel.y += 260.f * dt;
        p.pos += p.vel * dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0.f; }), particles.end());
}

// Maintains aspect ratio by letterboxing the view into the window
void letterbox(sf::View& view, unsigned winW, unsigned winH) {
    float targetRatio = float(view.getSize().x) / view.getSize().y;
    float windowRatio = float(winW) / winH;
    float sizeX = 1.f, sizeY = 1.f, posX = 0.f, posY = 0.f;

    if (windowRatio > targetRatio) {
        sizeX = targetRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    }
    else {
        sizeY = windowRatio / targetRatio;
        posY = (1.f - sizeY) / 2.f;
    }
    view.setViewport({ posX, posY, sizeX, sizeY });
}

sf::Vector2f gridToPixel(sf::Vector2i cell) {
    return {
        float(cell.x * CELL_SIZE),
        float(cell.y * CELL_SIZE + MARGIN)
    };
}


// --- safer spawn helper (FIX) ---
template <typename BlockedFn>
sf::Vector2i generateFreeCell(
    int minx, int maxx, int miny, int maxy,
    BlockedFn isBlocked,
    int maxTries = 5000
) {
    for (int tries = 0; tries < maxTries; ++tries) {
        sf::Vector2i p;
        p.x = minx + rand() % (maxx - minx + 1);
        p.y = miny + rand() % (maxy - miny + 1);
        if (!isBlocked(p)) return p;
    }
    for (int y = miny; y <= maxy; ++y)
        for (int x = minx; x <= maxx; ++x) {
            sf::Vector2i p{ x, y };
            if (!isBlocked(p)) return p;
        }
    return { minx, miny };
}

// legacy helper (still used in some places; safe if only snake check needed)
sf::Vector2i generateFoodPosition(
    const std::deque<sf::Vector2i>& snake,
    int minx = 1, int maxx = WIDTH - 2,
    int miny = 1, int maxy = HEIGHT - 2
) {
    sf::Vector2i pos;
    do {
        pos.x = minx + rand() % (maxx - minx + 1);
        pos.y = miny + rand() % (maxy - miny + 1);
    } while (std::find(snake.begin(), snake.end(), pos) != snake.end());
    return pos;
}

int loadHighScore() {
    std::ifstream in("txt/highscore.txt");
    int high = 0;
    if (in >> high) return high;
    return 0;
}

std::vector<int> loadHighScores() {
    std::ifstream in("txt/highscores.txt");
    std::vector<int> scores;
    int s;
    while (in >> s) scores.push_back(s);
    std::sort(scores.rbegin(), scores.rend());
    if (scores.size() > 5) scores.resize(5);
    return scores;
}

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#endif

static void ensureTxtFolderExists() {
#if __has_include(<filesystem>)
    try {
        fs::create_directories("txt");
    }
    catch (...) {
        // don't crash if folder creation fails
    }
#endif
}

void saveHighScore(int score) {
    ensureTxtFolderExists();
    std::ofstream out("txt/highscore.txt");   // ✅ FIXED PATH
    out << score;
}

void saveHighScores(const std::vector<int>& scores) {
    ensureTxtFolderExists();
    std::ofstream out("txt/highscores.txt");  // ✅ FIXED PATH
    for (int s : scores) out << s << "\n";
}


void insertHighScore(std::vector<int>& scores, int score) {
    scores.push_back(score);
    std::sort(scores.rbegin(), scores.rend());
    if (scores.size() > 5) scores.resize(5);
}

void resetGame(std::deque<sf::Vector2i>& snake,
    Direction& dir,
    int& score,
    int& foodEaten,
    float& delay,
    sf::Vector2i& food,
    bool& bonusActive,
    float& bonusTimeLeft,
    int& level,
    bool& shrinkFoodActive,
    sf::Vector2i& shrinkFood) {
    snake = { {10, 15}, {9, 15}, {8, 15} };
    dir = Right;
    score = 0;
    foodEaten = 0;
    delay = INITIAL_DELAY;
    warningActive = false;
    warningCount = 0;
    warningClock.restart();
    bonusActive = false;
    bonusTimeLeft = 0.f;
    shrinkFood = { -1, -1 };
    food = generateFoodPosition(snake);
    (void)level;
    (void)shrinkFoodActive;
}

void generateObstacles(int level,
    const std::deque<sf::Vector2i>& snake,
    std::vector<sf::Vector2i>& obstacles) {
    obstacles.clear();
    int count = (level == 2 ? 5 : 10);
    for (int i = 0; i < count; ++i) {
        sf::Vector2i p;
        do {
            p.x = rand() % (WIDTH - 2) + 1;
            p.y = rand() % (HEIGHT - 2) + 1;
        } while (std::find(snake.begin(), snake.end(), p) != snake.end());
        obstacles.push_back(p);
    }
}

bool shrinkFoodActive = false;
sf::Vector2i shrinkFood{ -1, -1 };

void doLevelSetup(int lvl,
    const std::deque<sf::Vector2i>& snake,
    std::vector<sf::Vector2i>& obstacles) {
    shrinkFoodActive = (lvl == 2 || lvl == 3);

    if (lvl >= 2) {
        shrinkFood = generateFoodPosition(snake);
        generateObstacles(lvl, snake, obstacles);
    }
    else {
        obstacles.clear();
        shrinkFood = { -1, -1 };
    }

    if (lvl == 3) {
        enemies.clear();
        Enemy e;

        int ix0 = 1, ix1 = WIDTH - 2, iy0 = 1, iy1 = HEIGHT - 2;

        do {
            e.pos.x = ix0 + rand() % (ix1 - ix0 + 1);
            e.pos.y = iy0 + rand() % (iy1 - iy0 + 1);
        } while (std::find(obstacles.begin(), obstacles.end(), e.pos) != obstacles.end()
            || std::find(snake.begin(), snake.end(), e.pos) != snake.end());

        enemies.push_back(e);

        shrinkTicks = 0;
        nextShrinkFood = SHRINK_FOOD_STEP;
        warningActive = false;
        warningCount = 0;
        warningClock.restart();

        // enemy animation reset (FIX)
        enemyAnimClock.restart();
    }
}

void showFlashMessage(sf::RenderWindow& w, sf::Font& f, const std::string& txt, float seconds) {
    sf::Text t(txt, f, 48);
    t.setFillColor(sf::Color::Yellow);
    auto b = t.getLocalBounds();
    t.setPosition((WIDTH * CELL_SIZE - b.width) / 2, MARGIN + 20);
    w.draw(t);
    w.display();
    sf::sleep(sf::seconds(seconds));
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    static constexpr unsigned LOG_W = WIDTH * CELL_SIZE;
    static constexpr unsigned LOG_H = HEIGHT * CELL_SIZE + MARGIN;

    bool isFullscreen = false;

    sf::RenderWindow window(
        sf::VideoMode(LOG_W, LOG_H),
        "Snake Game",
        sf::Style::Titlebar | sf::Style::Close
    );

    sf::View view(sf::FloatRect(0, 0, LOG_W, LOG_H));
    letterbox(view, LOG_W, LOG_H);
    window.setView(view);
    sf::View baseView = view;

    window.setFramerateLimit(60);

    // Settings state
    bool vsyncEnabled = false;
    float musicVolume = 50.f;
    float sfxVolume = 50.f;
    bool draggingMusic = false;
    bool draggingSfx = false;

    window.setVerticalSyncEnabled(vsyncEnabled);

    // Music
    sf::Music menuMusic;
    if (!menuMusic.openFromFile("audios/music.ogg"))
        std::cerr << "Error loading audios/music.ogg\n";
    menuMusic.setLoop(true);
    menuMusic.setVolume(musicVolume);

    sf::Music gameMusic;
    if (!gameMusic.openFromFile("audios/gameplay.ogg"))
        std::cerr << "Error loading audios/gameplay.ogg\n";
    gameMusic.setLoop(true);
    gameMusic.setVolume(musicVolume);

    sf::Music gameOverMusic;
    if (!gameOverMusic.openFromFile("audios/gameover.ogg"))
        std::cerr << "Error loading audios/gameover.ogg\n";
    gameOverMusic.setLoop(false);
    gameOverMusic.setVolume(musicVolume);

    sf::Music CrashMusic;
    if (!CrashMusic.openFromFile("audios/crash.ogg"))
        std::cerr << "Error loading audios/crash.ogg\n";
    CrashMusic.setLoop(false);
    CrashMusic.setVolume(sfxVolume);

    menuMusic.play();

    sf::Font font;
    if (!font.loadFromFile("fonts/snake.ttf")) {
        std::cerr << "Failed to load fonts/snake.ttf\n";
        return -1;
    }

    sf::Texture wallTex;
    wallTex.loadFromFile("images/wall.png");

    sf::Texture foodTex;
    sf::Sprite foodSprite;
    sf::Texture ShrinkFoodTex;
    sf::Sprite ShrinkFoodSprite;
    sf::Texture enemySheet;
    sf::Texture bonusFoodTex;
    sf::Sprite bonusFoodSprite;

    if (!enemySheet.loadFromFile("images/enemy.png")) { std::cerr << "images/enemy.png load fail\n"; return -1; }
    if (!foodTex.loadFromFile("images/Apple.png")) { std::cerr << "images/Apple.png load fail\n"; return -1; }
    if (!ShrinkFoodTex.loadFromFile("images/bad.png")) { std::cerr << "images/bad.png load fail\n"; return -1; }
    if (!bonusFoodTex.loadFromFile("images/Bonus.png")) { std::cerr << "images/Bonus.png load fail\n"; return -1; }

    sf::Texture menuBgTex;
    sf::Sprite  menuBgSprite;
    if (!menuBgTex.loadFromFile("images/menu_bg.png")) { std::cerr << "images/menu_bg.png load fail\n"; return -1; }
    menuBgSprite.setTexture(menuBgTex);

    float windowWidth = float(WIDTH * CELL_SIZE);
    float windowHeight = float(HEIGHT * CELL_SIZE + MARGIN);
    float texWidth = float(menuBgTex.getSize().x);
    float texHeight = float(menuBgTex.getSize().y);
    menuBgSprite.setScale(windowWidth / texWidth, windowHeight / texHeight);
    menuBgSprite.setPosition(0.f, 0.f);

    std::vector<sf::Texture> levelBgTex;
    std::vector<sf::Sprite>  levelBgSprite;
    sf::Texture gameOverBgTex;
    sf::Sprite  gameOverBgSprite;

    int numLevels = 3;
    levelBgTex.resize(numLevels);
    levelBgSprite.resize(numLevels);

    for (int i = 0; i < numLevels; ++i) {
        std::string filename = "images/level" + std::to_string(i + 1) + "_bg.png";
        if (!levelBgTex[i].loadFromFile(filename)) {
            std::cerr << "Failed to load " << filename << "\n";
            return -1;
        }
        levelBgSprite[i].setTexture(levelBgTex[i]);
       
       float sx = float(WIDTH * CELL_SIZE) / float(levelBgTex[i].getSize().x);
        float sy = float(HEIGHT * CELL_SIZE + MARGIN) / float(levelBgTex[i].getSize().y);

        levelBgSprite[i].setScale(sx, sy);
        levelBgSprite[i].setPosition(0, 0);
    }

    if (!gameOverBgTex.loadFromFile("images/gameover_bg.png")) {
        std::cerr << "Failed to load images/gameover_bg.png\n";
        return -1;
    }
    gameOverBgSprite.setTexture(gameOverBgTex);
    {
        float sx2 = float(WIDTH * CELL_SIZE) / gameOverBgTex.getSize().x;
        float sy2 = float(HEIGHT * CELL_SIZE + MARGIN) / gameOverBgTex.getSize().y;
        gameOverBgSprite.setScale(sx2, sy2);
        gameOverBgSprite.setPosition(0, 0);
    }

    foodTex.setSmooth(true);
    ShrinkFoodTex.setSmooth(true);
    bonusFoodTex.setSmooth(true);

    foodSprite.setTexture(foodTex);
    ShrinkFoodSprite.setTexture(ShrinkFoodTex);
    bonusFoodSprite.setTexture(bonusFoodTex);

    foodSprite.setScale(float(CELL_SIZE - 4) / foodTex.getSize().x,
        float(CELL_SIZE - 4) / foodTex.getSize().y);
    foodSprite.setOrigin(foodTex.getSize().x / 2.f, foodTex.getSize().y / 2.f);

    ShrinkFoodSprite.setScale(float(CELL_SIZE - 4) / ShrinkFoodTex.getSize().x,
        float(CELL_SIZE - 4) / ShrinkFoodTex.getSize().y);
    ShrinkFoodSprite.setOrigin(ShrinkFoodTex.getSize().x / 2.f, ShrinkFoodTex.getSize().y / 2.f);

    bonusFoodSprite.setScale(float(CELL_SIZE - 4) / bonusFoodTex.getSize().x,
        float(CELL_SIZE - 4) / bonusFoodTex.getSize().y);
    bonusFoodSprite.setOrigin(bonusFoodTex.getSize().x / 2.f, bonusFoodTex.getSize().y / 2.f);

    enemySheet.setSmooth(false);

    int frameW = enemySheet.getSize().x / ENEMY_COLS;
    int frameH = enemySheet.getSize().y / ENEMY_ROWS;

    sf::Sprite enemySprite(enemySheet);
    enemySprite.setScale(float(CELL_SIZE) / float(frameW),
        float(CELL_SIZE) / float(frameH));

    sf::RectangleShape wall(sf::Vector2f(CELL_SIZE, CELL_SIZE));
    wall.setTexture(&wallTex);

    // Prebuild outer wall cells (perf)
    std::vector<sf::Vector2i> outerWalls;
    outerWalls.reserve(WIDTH * 2 + HEIGHT * 2);
    for (int x = 0; x < WIDTH; ++x) { outerWalls.push_back({ x, 0 }); outerWalls.push_back({ x, HEIGHT - 1 }); }
    for (int y = 1; y < HEIGHT - 1; ++y) { outerWalls.push_back({ 0, y }); outerWalls.push_back({ WIDTH - 1, y }); }

    // Pause menu texts
    sf::Text pauseContinue, pauseQuit, pauseToMenu;
    sf::Color normal = sf::Color::White, hover = sf::Color::Red;

    pauseContinue.setFont(font);
    pauseContinue.setString("1. Continue");
    pauseContinue.setCharacterSize(30);

    pauseQuit.setFont(font);
    pauseQuit.setString("2. Quit");
    pauseQuit.setCharacterSize(30);

    pauseToMenu.setFont(font);
    pauseToMenu.setString("3. Quit to Main Menu");
    pauseToMenu.setCharacterSize(30);

    float baseX = (WIDTH * CELL_SIZE - pauseContinue.getLocalBounds().width) / 2;
    float baseY = 220;
    pauseContinue.setPosition(baseX, baseY);
    pauseQuit.setPosition(baseX, baseY + 40);
    pauseToMenu.setPosition(baseX, baseY + 80);

    sf::Text bonusTimerText("", font, 16);
    bonusTimerText.setFillColor(sf::Color::Blue);
    bonusTimerText.setPosition(WIDTH * CELL_SIZE - 140, 5);

    sf::Text pausedText("GAME PAUSED", font, 48);
    pausedText.setFillColor(sf::Color::White);

    sf::Text finalScoreText("", font, 28);
    finalScoreText.setFillColor(sf::Color::Yellow);

    sf::Text highScoreText("", font, 28);
    highScoreText.setFillColor(sf::Color::Cyan);

    sf::Text restartText("Restart (R)", font, 20);
    sf::Text exitText("Exit (Esc)", font, 20);
    sf::Text menuText("Main Menu (M)", font, 20);
    restartText.setFillColor(sf::Color::White);
    exitText.setFillColor(sf::Color::White);
    menuText.setFillColor(sf::Color::White);

    sf::RectangleShape restartBtn(sf::Vector2f(160, 48));
    sf::RectangleShape exitBtn(sf::Vector2f(160, 48));
    sf::RectangleShape menuBtn(sf::Vector2f(160, 48));
    restartBtn.setFillColor(sf::Color(30, 30, 30));
    exitBtn.setFillColor(sf::Color(30, 30, 30));
    menuBtn.setFillColor(sf::Color(30, 30, 30));
    restartBtn.setOutlineThickness(2);
    exitBtn.setOutlineThickness(2);
    menuBtn.setOutlineThickness(2);

    sf::Clock clock, flashClock;

    sf::RectangleShape bonusShape(sf::Vector2f(CELL_SIZE, CELL_SIZE));
    bonusShape.setFillColor(sf::Color::Blue);

    std::vector<sf::Vector2i> obstacles;

    std::deque<sf::Vector2i> snake;
    Direction dir;
    int score, foodEaten;
    float delay;
    bool bonusActive;
    float bonusTimeLeft;
    sf::Vector2i food, bonusFood;
    int level = 1;

    resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);

    GameState state = Paused;
    MenuState menu = MainMenu;

    std::vector<int> highScores = loadHighScores();

    // Mood menu buttons
    sf::RectangleShape cycleBtn({ 200, 48 });
    cycleBtn.setFillColor({ 80, 80, 80 });
    cycleBtn.setOutlineThickness(2);
    cycleBtn.setOutlineColor(sf::Color::White);
    cycleBtn.setPosition(60, 150);

    sf::Text cycleText("1. Cycle Level", font, 28);
    cycleText.setFillColor(sf::Color::White);
    cycleText.setPosition(cycleBtn.getPosition().x + 10,
        cycleBtn.getPosition().y + (cycleBtn.getSize().y - cycleText.getCharacterSize()) / 2 - 5);

    sf::RectangleShape pickBtn({ 200, 48 });
    pickBtn.setFillColor({ 80, 80, 80 });
    pickBtn.setOutlineThickness(2);
    pickBtn.setOutlineColor(sf::Color::White);
    pickBtn.setPosition(60, 210);

    sf::Text pickText("2. Pick Level", font, 28);
    pickText.setFillColor(sf::Color::White);
    pickText.setPosition(pickBtn.getPosition().x + 10,
        pickBtn.getPosition().y + (pickBtn.getSize().y - pickText.getCharacterSize()) / 2 - 5);

    // PickLevel menu buttons
    std::array<sf::RectangleShape, 3> levelBtns;
    std::array<sf::Text, 3> levelLabels;
    for (int i = 0; i < 3; ++i) {
        levelBtns[i].setSize({ 80, 48 });
        levelBtns[i].setFillColor({ 100, 100, 100 });
        levelBtns[i].setOutlineThickness(2);
        levelBtns[i].setOutlineColor(sf::Color::White);
        levelBtns[i].setPosition(
            60.f + float(i) * 100.f,
            150.f
        );

        levelLabels[i].setString(std::to_string(i + 1));
        levelLabels[i].setFont(font);
        levelLabels[i].setCharacterSize(28);
        levelLabels[i].setFillColor(sf::Color::White);
        levelLabels[i].setPosition(
            levelBtns[i].getPosition().x +
            (levelBtns[i].getSize().x - float(levelLabels[i].getCharacterSize())) * 0.5f,
            levelBtns[i].getPosition().y +
            (levelBtns[i].getSize().y - float(levelLabels[i].getCharacterSize())) * 0.5f - 5.f
        );

    }

    // Main menu entries (added Settings = 7)
    const std::array<std::string, 7> labels = {
        "1. Resume",
        "2. New Game",
        "3. High Score",
        "4. Level",
        "5. Mood",
        "6. Quit",
        "7. Settings"
    };
    std::array<sf::Text, 7> menuTexts;
    for (int i = 0; i < 7; ++i) {
        menuTexts[i].setFont(font);
        menuTexts[i].setString(labels[i]);
        menuTexts[i].setCharacterSize(28);
        menuTexts[i].setFillColor(sf::Color(255, 215, 0));
        menuTexts[i].setPosition(
            50.f,
            70.f + float(i) * (float(menuTexts[i].getCharacterSize()) + 6.f)
        );
    }

    // Cached score text (FIX)
    sf::Text scoreText("", font, 16);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(5.f, 5.f);

    sf::Text infoText("", font, 16);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(5.f, 22.f);

    int lastScoreShown = INT_MIN;
    int lastLevelShown = -1;
    PlayMode lastModeShown = PickLevel;

    // --- Settings menu UI (add-only) ---
    sf::Text settingsTitle("SETTINGS", font, 48);
    settingsTitle.setFillColor(sf::Color::White);
    settingsTitle.setPosition(
        (float(WIDTH * CELL_SIZE) - settingsTitle.getLocalBounds().width) * 0.5f,
        40.f
    );


    sf::Text musicLabel("Music Volume", font, 24);
    musicLabel.setFillColor(sf::Color::White);
    musicLabel.setPosition(60.f, 125.f);

    sf::RectangleShape musicBar({ 300.f, 10.f });
    musicBar.setFillColor(sf::Color(200, 200, 200));
    musicBar.setPosition(60.f, 160.f);

    sf::CircleShape musicKnob(10.f);
    musicKnob.setFillColor(sf::Color::Yellow);

    sf::Text sfxLabel("SFX Volume", font, 24);
    sfxLabel.setFillColor(sf::Color::White);
    sfxLabel.setPosition(60.f, 225.f);

    sf::RectangleShape sfxBar({ 300.f, 10.f });
    sfxBar.setFillColor(sf::Color(200, 200, 200));
    sfxBar.setPosition(60.f, 260.f);

    sf::CircleShape sfxKnob(10.f);
    sfxKnob.setFillColor(sf::Color::Yellow);

    sf::RectangleShape vsyncBtn({ 260.f, 44.f });
    vsyncBtn.setFillColor(sf::Color(80, 80, 80));
    vsyncBtn.setOutlineThickness(2);
    vsyncBtn.setOutlineColor(sf::Color::White);
    vsyncBtn.setPosition(60.f, 330.f);

    sf::Text vsyncText("", font, 22);
    vsyncText.setFillColor(sf::Color::White);

    sf::RectangleShape fsBtn({ 260.f, 44.f });
    fsBtn.setFillColor(sf::Color(80, 80, 80));
    fsBtn.setOutlineThickness(2);
    fsBtn.setOutlineColor(sf::Color::White);
    fsBtn.setPosition(60.f, 390.f);

    sf::Text fsText("", font, 22);
    fsText.setFillColor(sf::Color::White);

    sf::Text binds(
        "Keybinds:\n"
        "- Arrow keys: move\n"
        "- P: pause\n"
        "- R: restart (game over)\n"
        "- M: main menu (game over)\n"
        "- F11: fullscreen\n"
        "- ESC/0: back\n"
        "- V: toggle VSync (settings)\n",
        font, 18
    );
    binds.setFillColor(sf::Color::White);
    binds.setPosition(420.f, 140.f);

    sf::Text backHint("Press ESC or 0 to return", font, 18);
    backHint.setFillColor(sf::Color::White);
    backHint.setPosition(60.f, HEIGHT * CELL_SIZE + MARGIN - 40);

    // --- game loop ---
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // update particles always
        updateParticles(dt);

        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();

            // Fullscreen toggle (F11) stays available everywhere
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::F11) {
                window.close();

                if (!isFullscreen) {
                    window.create(sf::VideoMode::getDesktopMode(), "Snake Game", sf::Style::Fullscreen);
                }
                else {
                    window.create(sf::VideoMode(LOG_W, LOG_H), "Snake Game", sf::Style::Titlebar | sf::Style::Close);
                }
                isFullscreen = !isFullscreen;

                window.setFramerateLimit(60);
                window.setVerticalSyncEnabled(vsyncEnabled);

                letterbox(view, window.getSize().x, window.getSize().y);
                baseView = view;
                window.setView(view);
                continue;
            }

            if (e.type == sf::Event::Resized) {
                letterbox(view, e.size.width, e.size.height);
                baseView = view;
                window.setView(view);
            }

            // Hover handling
            if (e.type == sf::Event::MouseMoved) {
                sf::Vector2f mp = window.mapPixelToCoords({ e.mouseMove.x, e.mouseMove.y });

                if (menu == PauseMenu) {
                    pauseContinue.setFillColor(pauseContinue.getGlobalBounds().contains(mp) ? hover : normal);
                    pauseQuit.setFillColor(pauseQuit.getGlobalBounds().contains(mp) ? hover : normal);
                    pauseToMenu.setFillColor(pauseToMenu.getGlobalBounds().contains(mp) ? hover : normal);
                }
                else if (menu == MainMenu) {
                    for (int i = 0; i < 7; ++i) {
                        menuTexts[i].setFillColor(menuTexts[i].getGlobalBounds().contains(mp) ? hover : sf::Color(255, 215, 0));
                    }
                }
                else if (menu == MoodMenu) {
                    cycleBtn.setFillColor(cycleBtn.getGlobalBounds().contains(mp) ? hover : sf::Color(80, 80, 80));
                    pickBtn.setFillColor(pickBtn.getGlobalBounds().contains(mp) ? hover : sf::Color(80, 80, 80));
                }
                else if (menu == PickLevelMenu) {
                    for (int i = 0; i < MAX_LEVEL; ++i) {
                        levelBtns[i].setFillColor(levelBtns[i].getGlobalBounds().contains(mp) ? hover : sf::Color(100, 100, 100));
                    }
                }
                else if (menu == InGame && state == GameOver) {
                    restartBtn.setFillColor(restartBtn.getGlobalBounds().contains(mp) ? hover : sf::Color(30, 30, 30));
                    exitBtn.setFillColor(exitBtn.getGlobalBounds().contains(mp) ? hover : sf::Color(30, 30, 30));
                    menuBtn.setFillColor(menuBtn.getGlobalBounds().contains(mp) ? hover : sf::Color(30, 30, 30));
                }
                else if (menu == SettingsMenu) {
                    // Dragging sliders in Settings
                    auto setVolFromBar = [&](sf::RectangleShape& bar, float& vol) {
                        float x = mp.x;
                        float left = bar.getPosition().x;
                        float right = left + bar.getSize().x;
                        float t = (x - left) / (right - left);
                        t = std::max(0.f, std::min(1.f, t));
                        vol = t * 100.f;
                        };

                    if (draggingMusic) setVolFromBar(musicBar, musicVolume);
                    if (draggingSfx)   setVolFromBar(sfxBar, sfxVolume);

                    // Apply live
                    menuMusic.setVolume(musicVolume);
                    gameMusic.setVolume(musicVolume);
                    gameOverMusic.setVolume(musicVolume);
                    CrashMusic.setVolume(sfxVolume);
                }
            }

            // Mouse pressed
            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mp = window.mapPixelToCoords({ e.mouseButton.x, e.mouseButton.y });

                if (menu == MainMenu) {
                    if (menuTexts[0].getGlobalBounds().contains(mp)) {
                        if (state == Playing || state == Paused) {
                            menu = InGame;
                            menuMusic.stop();
                            gameMusic.play();
                        }
                    }
                    else if (menuTexts[1].getGlobalBounds().contains(mp)) {
                        // New Game
                        resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                        if (playMode == CycleLevel) level = 1;
                        nextShrinkFood = SHRINK_FOOD_STEP;
                        doLevelSetup(level, snake, obstacles);

                        state = Playing;
                        menu = InGame;
                        menuMusic.stop();
                        gameMusic.play();
                    }
                    else if (menuTexts[2].getGlobalBounds().contains(mp)) {
                        menu = HighScoreMenu;
                    }
                    else if (menuTexts[3].getGlobalBounds().contains(mp)) {
                        menu = PickLevelMenu;
                    }
                    else if (menuTexts[4].getGlobalBounds().contains(mp)) {
                        menu = MoodMenu;
                    }
                    else if (menuTexts[5].getGlobalBounds().contains(mp)) {
                        window.close();
                    }
                    else if (menuTexts[6].getGlobalBounds().contains(mp)) {
                        menu = SettingsMenu;
                    }
                }

                else if (menu == MoodMenu) {
                    if (cycleBtn.getGlobalBounds().contains(mp)) {
                        playMode = CycleLevel;
                        level = 1;
                        shrinkFoodActive = false;
                        obstacles.clear();
                        menu = MainMenu;
                        gameMusic.stop();
                        menuMusic.play();
                    }
                    else if (pickBtn.getGlobalBounds().contains(mp)) {
                        menu = PickLevelMenu;
                    }
                    // (Mouse union bug FIX: no e.key usage here)
                }

                else if (menu == PickLevelMenu) {
                    for (int i = 0; i < MAX_LEVEL; ++i) {
                        if (levelBtns[i].getGlobalBounds().contains(mp)) {
                            level = i + 1;
                            doLevelSetup(level, snake, obstacles);
                            menu = MainMenu;
                            gameMusic.stop();
                            menuMusic.play();
                        }
                    }
                }

                else if (menu == PauseMenu) {
                    if (pauseContinue.getGlobalBounds().contains(mp)) {
                        state = Playing;
                        menu = InGame;
                        menuMusic.stop();
                        gameMusic.play();
                    }
                    else if (pauseQuit.getGlobalBounds().contains(mp)) {
                        window.close();
                    }
                    else if (pauseToMenu.getGlobalBounds().contains(mp)) {
                        resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                        menu = MainMenu;
                        gameMusic.stop();
                        menuMusic.play();
                        state = Paused;
                    }
                }

                else if (menu == InGame && state == GameOver) {
                    if (restartBtn.getGlobalBounds().contains(mp)) {
                        resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                        if (playMode == CycleLevel) level = 1;
                        nextShrinkFood = SHRINK_FOOD_STEP;
                        doLevelSetup(level, snake, obstacles);

                        warningActive = false;
                        warningCount = 0;
                        warningClock.restart();

                        state = Playing;
                        menu = InGame;
                        gameOverMusic.stop();
                        menuMusic.stop();
                        gameMusic.play();
                    }
                    else if (exitBtn.getGlobalBounds().contains(mp)) {
                        window.close();
                    }
                    else if (menuBtn.getGlobalBounds().contains(mp)) {
                        menu = MainMenu;
                        gameOverMusic.stop();
                        gameMusic.stop();
                        menuMusic.play();
                        state = Paused;
                    }
                }

                else if (menu == SettingsMenu) {
                    // slider dragging
                    if (musicBar.getGlobalBounds().contains(mp) || musicKnob.getGlobalBounds().contains(mp))
                        draggingMusic = true;
                    if (sfxBar.getGlobalBounds().contains(mp) || sfxKnob.getGlobalBounds().contains(mp))
                        draggingSfx = true;

                    if (vsyncBtn.getGlobalBounds().contains(mp)) {
                        vsyncEnabled = !vsyncEnabled;
                        window.setVerticalSyncEnabled(vsyncEnabled);
                    }

                    if (fsBtn.getGlobalBounds().contains(mp)) {
                        // same as F11 toggle
                        window.close();

                        if (!isFullscreen) {
                            window.create(sf::VideoMode::getDesktopMode(), "Snake Game", sf::Style::Fullscreen);
                        }
                        else {
                            window.create(sf::VideoMode(LOG_W, LOG_H), "Snake Game", sf::Style::Titlebar | sf::Style::Close);
                        }
                        isFullscreen = !isFullscreen;

                        window.setFramerateLimit(60);
                        window.setVerticalSyncEnabled(vsyncEnabled);
                        letterbox(view, window.getSize().x, window.getSize().y);
                        baseView = view;
                        window.setView(view);
                    }
                }
            }

            if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
                draggingMusic = false;
                draggingSfx = false;
            }

            // Keyboard
            if (e.type == sf::Event::KeyPressed) {
                if (menu == MainMenu) {
                    switch (e.key.code) {
                    case sf::Keyboard::Num1:
                    case sf::Keyboard::Numpad1:
                        if (state == Playing || state == Paused) {
                            menu = InGame;
                            menuMusic.stop();
                            gameMusic.play();
                        }
                        break;
                    case sf::Keyboard::Num2:
                    case sf::Keyboard::Numpad2:
                        resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                        if (playMode == CycleLevel) level = 1;
                        nextShrinkFood = SHRINK_FOOD_STEP;
                        doLevelSetup(level, snake, obstacles);
                        state = Playing;
                        menu = InGame;
                        menuMusic.stop();
                        gameMusic.play();
                        break;
                    case sf::Keyboard::Num3:
                    case sf::Keyboard::Numpad3:
                        menu = HighScoreMenu;
                        break;
                    case sf::Keyboard::Num4:
                    case sf::Keyboard::Numpad4:
                        menu = PickLevelMenu;
                        break;
                    case sf::Keyboard::Num5:
                    case sf::Keyboard::Numpad5:
                        menu = MoodMenu;
                        break;
                    case sf::Keyboard::Num6:
                    case sf::Keyboard::Numpad6:
                        window.close();
                        break;
                    case sf::Keyboard::Num7:
                    case sf::Keyboard::Numpad7:
                        menu = SettingsMenu;
                        break;
                    default:
                        break;
                    }
                }

                else if (menu == HighScoreMenu) {
                    if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Num0 || e.key.code == sf::Keyboard::Numpad0) {
                        menu = MainMenu;
                    }
                }

                else if (menu == MoodMenu) {
                    if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Num0 || e.key.code == sf::Keyboard::Numpad0) {
                        menu = MainMenu;
                    }
                    else if (e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) {
                        playMode = CycleLevel;
                        level = 1;
                        shrinkFoodActive = false;
                        obstacles.clear();
                        menu = MainMenu;
                    }
                    else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) {
                        menu = PickLevelMenu;
                    }
                }

                else if (menu == PickLevelMenu) {
                    if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Num0 || e.key.code == sf::Keyboard::Numpad0) {
                        menu = MainMenu;
                    }
                    else if (e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) {
                        level = 1; doLevelSetup(level, snake, obstacles); menu = MainMenu;
                    }
                    else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) {
                        level = 2; doLevelSetup(level, snake, obstacles); menu = MainMenu;
                    }
                    else if (e.key.code == sf::Keyboard::Num3 || e.key.code == sf::Keyboard::Numpad3) {
                        level = 3; doLevelSetup(level, snake, obstacles); menu = MainMenu;
                    }
                }

                else if (menu == SettingsMenu) {
                    if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Num0 || e.key.code == sf::Keyboard::Numpad0) {
                        menu = MainMenu;
                    }
                    if (e.key.code == sf::Keyboard::V) {
                        vsyncEnabled = !vsyncEnabled;
                        window.setVerticalSyncEnabled(vsyncEnabled);
                    }
                }

                else if (menu == InGame) {
                    if (state == Playing) {
                        if (e.key.code == sf::Keyboard::Up && dir != Down) dir = Up;
                        else if (e.key.code == sf::Keyboard::Down && dir != Up) dir = Down;
                        else if (e.key.code == sf::Keyboard::Left && dir != Right) dir = Left;
                        else if (e.key.code == sf::Keyboard::Right && dir != Left) dir = Right;
                        else if (e.key.code == sf::Keyboard::P) {
                            state = Paused;
                            gameMusic.pause();
                            menu = PauseMenu;
                        }
                    }
                    else if (state == Paused) {
                        if (e.key.code == sf::Keyboard::P) {
                            state = Playing;
                            gameMusic.play();
                            menu = InGame;
                        }
                    }
                    else if (state == GameOver) {
                        if (e.key.code == sf::Keyboard::R) {
                            resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                            if (playMode == CycleLevel) level = 1;
                            nextShrinkFood = SHRINK_FOOD_STEP;
                            doLevelSetup(level, snake, obstacles);
                            warningActive = false;
                            warningCount = 0;
                            warningClock.restart();
                            state = Playing;
                            menu = InGame;
                            gameOverMusic.stop();
                            gameMusic.play();
                        }
                        else if (e.key.code == sf::Keyboard::Escape) {
                            window.close();
                        }
                        else if (e.key.code == sf::Keyboard::M) {
                            menu = MainMenu;
                            state = Paused;
                            gameOverMusic.stop();
                            gameMusic.stop();
                            menuMusic.play();
                        }
                    }
                }

                else if (menu == PauseMenu) {
                    if (e.key.code == sf::Keyboard::P || e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) {
                        state = Playing;
                        gameMusic.play();
                        menu = InGame;
                    }
                    else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) {
                        window.close();
                    }
                    else if (e.key.code == sf::Keyboard::Num3 || e.key.code == sf::Keyboard::Numpad3) {
                        resetGame(snake, dir, score, foodEaten, delay, food, bonusActive, bonusTimeLeft, level, shrinkFoodActive, shrinkFood);
                        gameMusic.stop();
                        menuMusic.play();
                        menu = MainMenu;
                        state = Paused;
                    }
                }
            }
        }

        // --- apply screen shake to view before drawing ---
        sf::View shaken = baseView;
        if (shakeTime > 0.f) {
            shakeTime -= dt;
            float strength = std::max(0.f, shakeTime / shakeDuration);
            float dx = frand(-shakeMagnitude, shakeMagnitude) * strength;
            float dy = frand(-shakeMagnitude, shakeMagnitude) * strength;
            shaken.move(dx, dy);
        }
        window.setView(shaken);

        // --- game update ---
        if (menu == InGame && state == Playing) {
            static float timer = 0.f;
            timer += dt;

            if (timer >= delay) {
                timer = 0.f;

                sf::Vector2i head = snake.front();
                if (dir == Up) head.y--;
                else if (dir == Down) head.y++;
                else if (dir == Left) head.x--;
                else if (dir == Right) head.x++;

                // level 3 enemy movement + collision vs NEW head (FIX)
                if (level == 3) {
                    minX = shrinkTicks + 1; maxX = WIDTH - 2 - shrinkTicks;
                    minY = shrinkTicks + 1; maxY = HEIGHT - 2 - shrinkTicks;

                    for (auto& en : enemies) {
                        en.moveTimer += dt;
                        if (en.moveTimer >= en.moveDelay) {
                            en.moveTimer = 0.f;

                            std::vector<sf::Vector2i> nbs;
                            static const sf::Vector2i dirs4[4] = { {1,0},{-1,0},{0,1},{0,-1} };
                            for (auto& d4 : dirs4) {
                                sf::Vector2i np = en.pos + d4;

                                if (np.x < 1 || np.x > WIDTH - 2 || np.y < 1 || np.y > HEIGHT - 2) continue;
                                if (np.x <= minX || np.x >= maxX || np.y <= minY || np.y >= maxY) continue;

                                if (std::find(obstacles.begin(), obstacles.end(), np) != obstacles.end()) continue;
                                if (std::find(snake.begin(), snake.end(), np) != snake.end()) continue;

                                nbs.push_back(np);
                            }
                            if (!nbs.empty()) en.pos = nbs[rand() % nbs.size()];
                        }

                        if (head == en.pos) {
                            CrashMusic.stop();
                            CrashMusic.setVolume(sfxVolume);
                            CrashMusic.play();
                            shakeTime = shakeDuration;

                            state = GameOver;
                            gameOverMusic.play();
                            gameMusic.stop();
                            break;
                        }
                    }
                    if (state == GameOver) {
                        // record score
                        insertHighScore(highScores, score);
                        saveHighScores(highScores);
                        if (score > loadHighScore()) saveHighScore(score);
                    }
                    if (state == GameOver) goto DRAW_FRAME;
                }

                // bounds for inner walls
                minX = shrinkTicks + 1; maxX = WIDTH - 2 - shrinkTicks;
                minY = shrinkTicks + 1; maxY = HEIGHT - 2 - shrinkTicks;

                bool hitInnerWall = (level == 3 && shrinkTicks > 0) &&
                    ((head.x == minX) || (head.x == maxX) || (head.y == minY) || (head.y == maxY));

                bool hitOuterWall = (head.x == 0 || head.x == WIDTH - 1 || head.y == 0 || head.y == HEIGHT - 1);
                bool hitObstacle = std::find(obstacles.begin(), obstacles.end(), head) != obstacles.end();
                bool hitSelf = std::find(snake.begin(), snake.end(), head) != snake.end();

                if (hitInnerWall || hitOuterWall || hitSelf || hitObstacle) {
                    insertHighScore(highScores, score);
                    saveHighScores(highScores);
                    if (score > loadHighScore()) saveHighScore(score);

                    CrashMusic.stop();
                    CrashMusic.setVolume(sfxVolume);
                    CrashMusic.play();
                    shakeTime = shakeDuration;

                    state = GameOver;
                    gameOverMusic.play();
                    gameMusic.stop();
                    menu = InGame;
                    goto DRAW_FRAME;
                }

                // warning countdown -> shrink
                if (warningActive) {
                    if (warningClock.getElapsedTime().asSeconds() >= 1.f) {
                        warningClock.restart();
                        --warningCount;
                        if (warningCount == 0) {
                            shrinkTicks++;
                            nextShrinkFood += SHRINK_FOOD_STEP;
                            warningActive = false;

                            int newMinX = shrinkTicks + 1;
                            int newMaxX = WIDTH - 2 - shrinkTicks;
                            int newMinY = shrinkTicks + 1;
                            int newMaxY = HEIGHT - 2 - shrinkTicks;

                            for (auto& en : enemies) {
                                if (en.pos.x < newMinX || en.pos.x > newMaxX || en.pos.y < newMinY || en.pos.y > newMaxY) {
                                    sf::Vector2i dest;
                                    do {
                                        dest.x = newMinX + rand() % (newMaxX - newMinX + 1);
                                        dest.y = newMinY + rand() % (newMaxY - newMinY + 1);
                                    } while (std::find(snake.begin(), snake.end(), dest) != snake.end()
                                        || std::find(obstacles.begin(), obstacles.end(), dest) != obstacles.end());
                                    en.pos = dest;
                                }
                            }

                            auto count = obstacles.size();
                            obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                                [&](const sf::Vector2i& o) {
                                    return o.x < newMinX || o.x > newMaxX || o.y < newMinY || o.y > newMaxY;
                                }), obstacles.end());

                            while (obstacles.size() < count) {
                                obstacles.push_back(generateFoodPosition(snake, newMinX, newMaxX, newMinY, newMaxY));
                            }
                        }
                    }
                }

                snake.push_front(head);

                // compute spawn bounds
                int fx0 = (level == 3 ? minX + 1 : 1);
                int fx1 = (level == 3 ? maxX - 1 : WIDTH - 2);
                int fy0 = (level == 3 ? minY + 1 : 1);
                int fy1 = (level == 3 ? maxY - 1 : HEIGHT - 2);

                auto blocked = [&](sf::Vector2i p) {
                    if (std::find(snake.begin(), snake.end(), p) != snake.end()) return true;
                    if (std::find(obstacles.begin(), obstacles.end(), p) != obstacles.end()) return true;
                    if (level == 3) {
                        for (auto& en : enemies) if (en.pos == p) return true;
                        if (shrinkTicks > 0) {
                            int minX2 = shrinkTicks + 1, maxX2 = WIDTH - 2 - shrinkTicks;
                            int minY2 = shrinkTicks + 1, maxY2 = HEIGHT - 2 - shrinkTicks;
                            if (p.x == minX2 || p.x == maxX2 || p.y == minY2 || p.y == maxY2) return true;
                        }
                    }
                    return false;
                    };

                if (head == food) {
                    score += 10;
                    foodEaten++;

                    // particles on eat
                    spawnParticles(gridToPixel(food) + sf::Vector2f(CELL_SIZE / 2.f, CELL_SIZE / 2.f), 18);

                    if (level < MAX_LEVEL && score >= LEVEL_UP_SCORES[level]) {
                        level++;
                        doLevelSetup(level, snake, obstacles);
                        showFlashMessage(window, font, "LEVEL UP!", 1.0f);
                    }

                    food = generateFreeCell(fx0, fx1, fy0, fy1, blocked);

                    if (level == 2 || level == 3) {
                        shrinkFood = generateFreeCell(fx0, fx1, fy0, fy1, blocked);
                    }

                    if (level == 3 && !warningActive && shrinkTicks < MAX_SHRINK_TICKS && foodEaten >= nextShrinkFood) {
                        warningActive = true;
                        warningCount = 3;
                        warningClock.restart();
                    }

                    if (foodEaten % FOODS_PER_LEVEL == 0 && !bonusActive) {
                        bonusActive = true;
                        bonusTimeLeft = BONUS_TIME;
                        bonusFood = generateFreeCell(fx0, fx1, fy0, fy1, blocked);
                    }

                    delay = std::max(MIN_DELAY, delay - DELAY_DECREMENT);
                }
                else if (bonusActive && head == bonusFood) {
                    score += static_cast<int>(BONUS_MAX_SCORE * (bonusTimeLeft / BONUS_TIME));
                    bonusActive = false;

                    // particles on bonus
                    spawnParticles(gridToPixel(bonusFood) + sf::Vector2f(CELL_SIZE / 2.f, CELL_SIZE / 2.f), 28);

                    if (level < MAX_LEVEL && score >= LEVEL_UP_SCORES[level]) {
                        level++;
                        doLevelSetup(level, snake, obstacles);
                        showFlashMessage(window, font, "LEVEL UP!", 1.0f);
                    }

                    if (level == 3 && shrinkTicks < MAX_SHRINK_TICKS && foodEaten >= nextShrinkFood) {
                        warningActive = true;
                        warningCount = 3;
                        warningClock.restart();
                    }
                }
                else if (shrinkFoodActive && head == shrinkFood) {
                    if (snake.size() <= STARTING_SNAKE_LENGTH + 1) {
                        insertHighScore(highScores, score);
                        saveHighScores(highScores);
                        if (score > loadHighScore()) saveHighScore(score);

                        state = GameOver;
                        gameOverMusic.play();
                        gameMusic.stop();
                        menu = InGame;
                        goto DRAW_FRAME;
                    }
                    else {
                        snake.pop_back();
                        snake.pop_back();
                        score -= 5;
                    }

                    food = generateFreeCell(fx0, fx1, fy0, fy1, blocked);
                    if (level == 2 || level == 3) {
                        shrinkFood = generateFreeCell(fx0, fx1, fy0, fy1, blocked);
                    }
                }
                else {
                    snake.pop_back();
                }
            }

            if (bonusActive) {
                bonusTimeLeft -= dt;
                if (bonusTimeLeft <= 0) bonusActive = false;
            }
        }

    DRAW_FRAME:
        window.clear();

        if (menu == MainMenu) {
            window.draw(menuBgSprite);
            for (auto& t : menuTexts) window.draw(t);
            window.display();
            continue;
        }

        if (menu == HighScoreMenu) {
            sf::RectangleShape bg(sf::Vector2f(WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE + MARGIN));
            bg.setFillColor(sf::Color(20, 20, 60));
            window.draw(bg);

            sf::Text title("HIGH SCORES", font, 48);
            title.setFillColor(sf::Color::Yellow);
            title.setPosition((WIDTH * CELL_SIZE - title.getLocalBounds().width) / 2, 50);
            window.draw(title);

            for (size_t i = 0; i < highScores.size(); ++i) {
                sf::Text hsItem(std::to_string(i + 1) + ". " + std::to_string(highScores[i]), font, 36);
                hsItem.setFillColor(sf::Color::White);
                hsItem.setPosition(100, 140 + float(i) * 50.f);
                window.draw(hsItem);
            }

            sf::Text info("Press ESC or 0 to return", font, 20);
            info.setFillColor(sf::Color::White);
            info.setPosition(60, HEIGHT * CELL_SIZE + MARGIN - 40);
            window.draw(info);

            window.display();
            continue;
        }

        if (menu == MoodMenu) {
            window.draw(menuBgSprite);
            window.draw(cycleBtn);
            window.draw(cycleText);
            window.draw(pickBtn);
            window.draw(pickText);
            window.display();
            continue;
        }

        if (menu == PickLevelMenu) {
            window.draw(menuBgSprite);
            for (int i = 0; i < 3; ++i) {
                window.draw(levelBtns[i]);
                window.draw(levelLabels[i]);
            }
            window.display();
            continue;
        }

        if (menu == SettingsMenu) {
            window.draw(menuBgSprite);

            auto knobX = [&](sf::RectangleShape& bar, float vol) {
                float t = std::max(0.f, std::min(1.f, vol / 100.f));
                return bar.getPosition().x + t * bar.getSize().x;
                };

            musicKnob.setPosition(knobX(musicBar, musicVolume) - musicKnob.getRadius(),
                musicBar.getPosition().y - musicKnob.getRadius() + 5.f);

            sfxKnob.setPosition(knobX(sfxBar, sfxVolume) - sfxKnob.getRadius(),
                sfxBar.getPosition().y - sfxKnob.getRadius() + 5.f);

            vsyncText.setString(std::string("VSync: ") + (vsyncEnabled ? "ON" : "OFF"));
            vsyncText.setPosition(vsyncBtn.getPosition().x + 10.f, vsyncBtn.getPosition().y + 8.f);

            fsText.setString(std::string("Fullscreen: ") + (isFullscreen ? "ON" : "OFF"));
            fsText.setPosition(fsBtn.getPosition().x + 10.f, fsBtn.getPosition().y + 8.f);

            window.draw(settingsTitle);

            window.draw(musicLabel);
            window.draw(musicBar);
            window.draw(musicKnob);

            window.draw(sfxLabel);
            window.draw(sfxBar);
            window.draw(sfxKnob);

            window.draw(vsyncBtn);
            window.draw(vsyncText);

            window.draw(fsBtn);
            window.draw(fsText);

            window.draw(binds);
            window.draw(backHint);

            window.display();
            continue;
        }

        if (menu == PauseMenu) {
            window.draw(levelBgSprite[level - 1]);

            // fake blur overlay (stacked translucent layers)
            sf::RectangleShape overlay(sf::Vector2f(WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE + MARGIN));
            overlay.setFillColor(sf::Color(0, 0, 0, 120));
            overlay.setPosition(0.f, 0.f);
            window.draw(overlay);

            overlay.setFillColor(sf::Color(0, 0, 0, 70));
            overlay.setPosition(1.f, 1.f);
            window.draw(overlay);

            overlay.setFillColor(sf::Color(0, 150, 180, 110));
            overlay.setPosition(0.f, 0.f);
            window.draw(overlay);

            sf::Text pauseTitle("GAME PAUSED", font, 48);
            pauseTitle.setFillColor(sf::Color::White);
            pauseTitle.setPosition((WIDTH * CELL_SIZE - pauseTitle.getLocalBounds().width) / 2, 100);
            window.draw(pauseTitle);

            window.draw(pauseContinue);
            window.draw(pauseQuit);
            window.draw(pauseToMenu);

            window.display();
            continue;
        }

        // InGame (Playing / Paused / GameOver)
        if (menu == InGame) {
            window.draw(levelBgSprite[level - 1]);

            // outer walls
            for (auto& c : outerWalls) {
                wall.setPosition(gridToPixel(c));
                window.draw(wall);
            }

            // inner wall (level 3 shrink)
            if (level == 3 && shrinkTicks > 0) {
                wall.setFillColor(sf::Color(100, 100, 100));
                for (int x = minX; x <= maxX; ++x) {
                    wall.setPosition(gridToPixel({ x, minY })); window.draw(wall);
                    wall.setPosition(gridToPixel({ x, maxY })); window.draw(wall);
                }
                for (int y = minY; y <= maxY; ++y) {
                    wall.setPosition(gridToPixel({ minX, y })); window.draw(wall);
                    wall.setPosition(gridToPixel({ maxX, y })); window.draw(wall);
                }
                wall.setFillColor(sf::Color::White);
            }

            // draw enemies (animation clock FIX)
            if (level == 3) {
                float elapsed = enemyAnimClock.getElapsedTime().asSeconds();
                int frameInRow = int(elapsed / ENEMY_FRAME_DURATION) % ENEMY_COLS;
                int rowIndex = std::min(shrinkTicks, 2);

                enemySprite.setTextureRect({
                    frameInRow * frameW,
                    rowIndex * frameH,
                    frameW,
                    frameH
                    });

                for (auto& en : enemies) {
                    enemySprite.setPosition(gridToPixel(en.pos));
                    window.draw(enemySprite);
                }

                if (warningActive) {
                    sf::Text warningText(std::to_string(warningCount), font, 96);
                    warningText.setFillColor(sf::Color::Red);
                    auto b = warningText.getLocalBounds();
                    warningText.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
                    warningText.setPosition(WIDTH * CELL_SIZE / 2.f, (HEIGHT * CELL_SIZE + MARGIN) / 2.f);
                    window.draw(warningText);
                }
            }

            // food
            {
                sf::Vector2f pixel = gridToPixel(food) + sf::Vector2f(CELL_SIZE / 2.f, CELL_SIZE / 2.f);
                foodSprite.setPosition(pixel);
                window.draw(foodSprite);
            }

            // bonus
            if (bonusActive) {
                sf::Vector2f pixel = gridToPixel(bonusFood) + sf::Vector2f(CELL_SIZE / 2.f, CELL_SIZE / 2.f);
                bonusFoodSprite.setPosition(pixel);
                window.draw(bonusFoodSprite);
            }

            // obstacles
            sf::RectangleShape obsShape(sf::Vector2f(CELL_SIZE, CELL_SIZE));
            obsShape.setFillColor(sf::Color(128, 64, 0));
            for (auto& o : obstacles) {
                obsShape.setPosition(gridToPixel(o));
                window.draw(obsShape);
            }

            // snake
            sf::RectangleShape segment(sf::Vector2f(CELL_SIZE, CELL_SIZE));
            segment.setFillColor(sf::Color::Green);
            for (const auto& s : snake) {
                segment.setPosition(gridToPixel(s));
                window.draw(segment);
            }

            // shrink food
            if (shrinkFoodActive && shrinkFood != sf::Vector2i{ -1, -1 }) {
                sf::Vector2f pixel = gridToPixel(shrinkFood) + sf::Vector2f(CELL_SIZE / 2.f, CELL_SIZE / 2.f);
                ShrinkFoodSprite.setPosition(pixel);
                window.draw(ShrinkFoodSprite);
            }

            // particles
            sf::CircleShape dot(2.f);
            for (auto& p : particles) {
                float a = std::max(0.f, std::min(1.f, p.life / 0.35f));
                dot.setFillColor(sf::Color(255, 255, 255, sf::Uint8(255 * a)));
                dot.setPosition(p.pos);
                window.draw(dot);
            }

            // cached score text (FIX)
            if (score != lastScoreShown) {
                scoreText.setString("Score: " + std::to_string(score));
                lastScoreShown = score;
            }
            window.draw(scoreText);

            if (level != lastLevelShown || playMode != lastModeShown) {
                std::string mode = (playMode == CycleLevel ? "Cycle" : "Pick");
                infoText.setString("Level: " + std::to_string(level) + "  Mode: " + mode);
                lastLevelShown = level;
                lastModeShown = playMode;
            }
            window.draw(infoText);

            // bonus timer
            if (bonusActive) {
                std::ostringstream oss;
                oss << "Bonus: " << std::fixed << std::setprecision(1) << bonusTimeLeft;
                bonusTimerText.setString(oss.str());
                window.draw(bonusTimerText);
            }

            if (state == GameOver) {
                window.draw(gameOverBgSprite);

                finalScoreText.setString("Score: " + std::to_string(score));
                finalScoreText.setPosition((WIDTH * CELL_SIZE - finalScoreText.getLocalBounds().width) / 2,
                    HEIGHT * CELL_SIZE / 2 - 30);
                window.draw(finalScoreText);

                int topHighScore = highScores.empty() ? 0 : highScores[0];
                highScoreText.setString("High Score: " + std::to_string(topHighScore));
                highScoreText.setPosition((WIDTH * CELL_SIZE - highScoreText.getLocalBounds().width) / 2,
                    HEIGHT * CELL_SIZE / 2 + 10);
                window.draw(highScoreText);

                float btnY = HEIGHT * CELL_SIZE / 2 + 60;
                float spacing = 20.f;
                float totalWidth = restartBtn.getSize().x + exitBtn.getSize().x + spacing;
                float btnY_dash = HEIGHT * CELL_SIZE / 2 + 140;

                restartBtn.setPosition((WIDTH * CELL_SIZE - totalWidth) / 2, btnY);
                exitBtn.setPosition((WIDTH * CELL_SIZE - totalWidth) / 2 + restartBtn.getSize().x + spacing, btnY);
                menuBtn.setPosition((WIDTH * CELL_SIZE - menuBtn.getSize().x) / 2, btnY_dash);

                bool flash = ((int)(flashClock.getElapsedTime().asSeconds() * 2)) % 2 == 0;
                sf::Color borderColor = flash ? sf::Color::White : sf::Color::Transparent;
                restartBtn.setOutlineColor(borderColor);
                exitBtn.setOutlineColor(borderColor);

                window.draw(restartBtn);
                window.draw(exitBtn);
                window.draw(menuBtn);

                // center texts
                sf::FloatRect rt = restartText.getLocalBounds();
                restartText.setOrigin(rt.left + rt.width / 2.f, rt.top + rt.height / 2.f);
                restartText.setPosition(restartBtn.getPosition().x + restartBtn.getSize().x / 2.f,
                    restartBtn.getPosition().y + restartBtn.getSize().y / 2.f);

                sf::FloatRect et = exitText.getLocalBounds();
                exitText.setOrigin(et.left + et.width / 2.f, et.top + et.height / 2.f);
                exitText.setPosition(exitBtn.getPosition().x + exitBtn.getSize().x / 2.f,
                    exitBtn.getPosition().y + exitBtn.getSize().y / 2.f);

                menuText.setPosition(menuBtn.getPosition().x + 10,
                    menuBtn.getPosition().y + (menuBtn.getSize().y - menuText.getCharacterSize()) / 2 - 5);

                window.draw(restartText);
                window.draw(exitText);
                window.draw(menuText);
            }

            window.display();
            continue;
        }

        window.display();
    }

    return 0;
}
