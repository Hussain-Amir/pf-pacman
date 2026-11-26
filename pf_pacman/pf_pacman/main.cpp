#include <SFML/Graphics.hpp>
#include <optional>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

const int WIDTH = 570;
const int HEIGHT = 680;
const float TILE = 30.f;
const int ROWS = 21;
const int COLS = 19;
const float HUD_HEIGHT = 50.f;
const int NUM_GHOSTS = 4;
const int PLAYER_SPEED = 8;
const int GHOST_SPEED = 10;
const int GHOST_FRIGHT_SPEED = 16;
const int GHOST_POINTS = 200;
const int START_LIVES = 3;
const int PELLET_POINTS = 10;
const int POWER_POINTS = 50;
const int FRIGHT_FRAMES = 5 * 60;
const string SAVE_FILE = "savegame.txt";

const int MENU = 0, PLAYING = 1, GAME_OVER = 2;

const int MENU_NEW = 0, MENU_CONTINUE = 1, MENU_EXIT = 2;
const int MENU_COUNT = 3;

char maze[ROWS][COLS];
int pelletCount = 0;
int playerStartRow = 0, playerStartCol = 0;
int ghostStartRow[NUM_GHOSTS], ghostStartCol[NUM_GHOSTS];
bool mazeLoaded = false;
sf::Font font;
bool fontLoaded = false;

int gameState = MENU;
bool playerWon = false;
int menuChoice = MENU_NEW;
bool noSaveMessage = false;

int keyDir = 0;          // 0 none, 1 up, 2 down, 3 left, 4 right
int playerRow = 0, playerCol = 0;
int playerDir = 0;
int playerTimer = 0;

int score = 0;
int lives = START_LIVES;
int frightenedTimer = 0;

int ghostRow[NUM_GHOSTS], ghostCol[NUM_GHOSTS];
int ghostDir[NUM_GHOSTS];
int ghostTimer[NUM_GHOSTS];
bool ghostBlue[NUM_GHOSTS];

// positions at the start of the frame, needed to catch swaps
int playerPrevRow = 0, playerPrevCol = 0;
int ghostPrevRow[NUM_GHOSTS], ghostPrevCol[NUM_GHOSTS];

const int dRow[5] = { 0, -1, 1, 0, 0 };
const int dCol[5] = { 0, 0, 0, -1, 1 };

// tie order: up, left, down, right
const int TIE_ORDER[4] = { 1, 3, 2, 4 };

const sf::Color GHOST_COLORS[NUM_GHOSTS] = {
    sf::Color(255, 0, 0),
    sf::Color(255, 150, 200),
    sf::Color(0, 255, 255),
    sf::Color(255, 160, 0)
};

// reads maze.txt, false if file is missing or wrong
bool loadMaze(const string& filename)
{
    ifstream file(filename);
    if (!file.is_open())
        return false;

    int rows, cols;
    file >> rows >> cols;
    if (rows != ROWS || cols != COLS)
        return false;

    pelletCount = 0;
    int ghostsFound = 0;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            file >> maze[r][c];

            if (maze[r][c] == '.' || maze[r][c] == 'o')
            {
                pelletCount++;
            }
            else if (maze[r][c] == 'P')
            {
                playerStartRow = r;
                playerStartCol = c;
                maze[r][c] = '-';
            }
            else if (maze[r][c] == 'G')
            {
                if (ghostsFound < NUM_GHOSTS)
                {
                    ghostStartRow[ghostsFound] = r;
                    ghostStartCol[ghostsFound] = c;
                    ghostsFound++;
                }
                maze[r][c] = '-';
            }
        }
    }
    return ghostsFound == NUM_GHOSTS;
}

// true for wall or outside the maze
bool isWall(int r, int c)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
        return true;
    return maze[r][c] == '#';
}

// puts player and ghosts on their start cells, ends blue mode
void resetPositions()
{
    playerRow = playerStartRow;
    playerCol = playerStartCol;
    playerDir = 0;
    keyDir = 0;
    playerTimer = 0;
    frightenedTimer = 0;

    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghostRow[i] = ghostStartRow[i];
        ghostCol[i] = ghostStartCol[i];
        ghostDir[i] = 0;
        ghostTimer[i] = 0;
        ghostBlue[i] = false;
    }
}

// moves one cell every PLAYER_SPEED frames
void movePlayer()
{
    playerTimer++;
    if (playerTimer < PLAYER_SPEED)
        return;
    playerTimer = 0;

    // turn only if that way is open, else key is ignored
    if (keyDir != 0 && !isWall(playerRow + dRow[keyDir], playerCol + dCol[keyDir]))
        playerDir = keyDir;

    int newRow = playerRow + dRow[playerDir];
    int newCol = playerCol + dCol[playerDir];

    if (!isWall(newRow, newCol))
    {
        playerRow = newRow;
        playerCol = newCol;
    }
    else
    {
        playerDir = 0;
    }
}

// all ghosts turn blue, eating another power pellet restarts the timer
void makeGhostsBlue()
{
    for (int i = 0; i < NUM_GHOSTS; i++)
        ghostBlue[i] = true;
    frightenedTimer = FRIGHT_FRAMES;
}

// eats the pellet under the player
void eatPellet()
{
    char cell = maze[playerRow][playerCol];

    if (cell == '.')
    {
        score += PELLET_POINTS;
        maze[playerRow][playerCol] = '-';
        pelletCount--;
    }
    else if (cell == 'o')
    {
        score += POWER_POINTS;
        maze[playerRow][playerCol] = '-';
        pelletCount--;
        makeGhostsBlue();
    }
}

// up <-> down, left <-> right
int oppositeDir(int d)
{
    if (d == 0)
        return 0;
    return (d % 2 == 1) ? d + 1 : d - 1;
}

// ghosts 0 and 2 chase, 1 and 3 wander
bool isChaser(int i)
{
    return i == 0 || i == 2;
}

// moves ghost i one cell, blue ghosts are slower
void moveGhost(int i)
{
    int speed = ghostBlue[i] ? GHOST_FRIGHT_SPEED : GHOST_SPEED;
    ghostTimer[i]++;
    if (ghostTimer[i] < speed)
        return;
    ghostTimer[i] = 0;

    // open directions in tie order, never turning back
    int options[4];
    int count = 0;
    int back = oppositeDir(ghostDir[i]);

    for (int k = 0; k < 4; k++)
    {
        int d = TIE_ORDER[k];
        if (d == back)
            continue;
        if (!isWall(ghostRow[i] + dRow[d], ghostCol[i] + dCol[d]))
            options[count++] = d;
    }

    // turn back only in a dead end
    if (count == 0 && back != 0 && !isWall(ghostRow[i] + dRow[back], ghostCol[i] + dCol[back]))
        options[count++] = back;

    if (count == 0)
        return;

    int chosen = options[0];

    if (ghostBlue[i] || !isChaser(i))
    {
        chosen = options[rand() % count];
    }
    else
    {
        // closest cell to player, "<" keeps first one on a tie
        int best = 9999;
        for (int k = 0; k < count; k++)
        {
            int d = options[k];
            int r = ghostRow[i] + dRow[d];
            int c = ghostCol[i] + dCol[d];
            int dist = abs(r - playerRow) + abs(c - playerCol);
            if (dist < best)
            {
                best = dist;
                chosen = d;
            }
        }
    }

    ghostDir[i] = chosen;
    ghostRow[i] += dRow[chosen];
    ghostCol[i] += dCol[chosen];
}

void moveGhosts()
{
    for (int i = 0; i < NUM_GHOSTS; i++)
        moveGhost(i);
}

// shows the win or game over screen
void endGame(bool won)
{
    playerWon = won;
    gameState = GAME_OVER;
}

// one life less, everyone back to start, pellets stay
void loseLife()
{
    lives--;
    if (lives <= 0)
    {
        endGame(false);
        return;
    }
    resetPositions();
}

// two ways to meet: same cell, or swapped cells in this frame
void checkCollisions()
{
    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        bool sameCell = (playerRow == ghostRow[i] && playerCol == ghostCol[i]);
        bool swapped = (playerRow == ghostPrevRow[i] && playerCol == ghostPrevCol[i] &&
            ghostRow[i] == playerPrevRow && ghostCol[i] == playerPrevCol);

        if (!sameCell && !swapped)
            continue;

        if (ghostBlue[i])
        {
            // eat the blue ghost, it goes back to start
            score += GHOST_POINTS;
            ghostRow[i] = ghostStartRow[i];
            ghostCol[i] = ghostStartCol[i];
            ghostDir[i] = 0;
            ghostTimer[i] = 0;
            ghostBlue[i] = false;
        }
        else
        {
            loseLife();
            return;
        }
    }
}

// fresh game: score 0, full lives, maze loaded again
void startNewGame()
{
    score = 0;
    lives = START_LIVES;
    playerWon = false;
    mazeLoaded = loadMaze("maze.txt");
    resetPositions();
    gameState = PLAYING;
}

// true if savegame.txt exists and is not empty
bool hasSavedGame()
{
    ifstream file(SAVE_FILE);
    if (!file.is_open())
        return false;

    int firstNumber;
    if (file >> firstNumber)
        return true;
    else
        return false;
}

// loading the save comes in the next part
void continueGame()
{
}

void goToMenu()
{
    gameState = MENU;
    menuChoice = MENU_NEW;
    noSaveMessage = false;
}

void handleMenuKey(sf::Keyboard::Key code, sf::RenderWindow& window)
{
    if (code == sf::Keyboard::Key::Up || code == sf::Keyboard::Key::W)
    {
        menuChoice = (menuChoice + MENU_COUNT - 1) % MENU_COUNT;
        noSaveMessage = false;
    }
    else if (code == sf::Keyboard::Key::Down || code == sf::Keyboard::Key::S)
    {
        menuChoice = (menuChoice + 1) % MENU_COUNT;
        noSaveMessage = false;
    }
    else if (code == sf::Keyboard::Key::Enter)
    {
        switch (menuChoice)
        {
        case MENU_NEW:
            startNewGame();
            break;
        case MENU_CONTINUE:
            if (hasSavedGame())
                continueGame();
            else
                noSaveMessage = true;
            break;
        case MENU_EXIT:
            window.close();
            break;
        }
    }
}

void handlePlayingKey(sf::Keyboard::Key code)
{
    if (code == sf::Keyboard::Key::Up || code == sf::Keyboard::Key::W)
        keyDir = 1;
    else if (code == sf::Keyboard::Key::Down || code == sf::Keyboard::Key::S)
        keyDir = 2;
    else if (code == sf::Keyboard::Key::Left || code == sf::Keyboard::Key::A)
        keyDir = 3;
    else if (code == sf::Keyboard::Key::Right || code == sf::Keyboard::Key::D)
        keyDir = 4;
    else if (code == sf::Keyboard::Key::Escape)
        goToMenu();                // saving comes in the next part
}

// each state handles only its own keys
void handleInput(sf::RenderWindow& window)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            switch (gameState)
            {
            case MENU:
                handleMenuKey(key->code, window);
                break;
            case PLAYING:
                handlePlayingKey(key->code);
                break;
            case GAME_OVER:
                if (key->code == sf::Keyboard::Key::Enter)
                    goToMenu();
                break;
            }
        }
    }
}

void updatePlaying()
{
    if (!mazeLoaded)
        return;

    playerPrevRow = playerRow;
    playerPrevCol = playerCol;
    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghostPrevRow[i] = ghostRow[i];
        ghostPrevCol[i] = ghostCol[i];
    }

    movePlayer();
    eatPellet();

    if (pelletCount == 0)
    {
        endGame(true);
        return;
    }

    moveGhosts();
    checkCollisions();
    if (gameState != PLAYING)
        return;

    if (frightenedTimer > 0)
    {
        frightenedTimer--;
        if (frightenedTimer == 0)
            for (int i = 0; i < NUM_GHOSTS; i++)
                ghostBlue[i] = false;
    }
}

void updateGame()
{
    switch (gameState)
    {
    case MENU:
        break;
    case PLAYING:
        updatePlaying();
        break;
    case GAME_OVER:
        break;
    }
}

void drawMaze(sf::RenderWindow& window)
{
    sf::RectangleShape wallBlock({ TILE, TILE });
    wallBlock.setFillColor(sf::Color(33, 33, 222));

    sf::CircleShape pellet(3.f);
    pellet.setFillColor(sf::Color::White);

    sf::CircleShape powerPellet(7.f);
    powerPellet.setFillColor(sf::Color::White);

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            float x = c * TILE;
            float y = HUD_HEIGHT + r * TILE;   // maze starts below the hud

            if (maze[r][c] == '#')
            {
                wallBlock.setPosition({ x, y });
                window.draw(wallBlock);
            }
            else if (maze[r][c] == '.')
            {
                pellet.setPosition({ x + 12.f, y + 12.f });
                window.draw(pellet);
            }
            else if (maze[r][c] == 'o')
            {
                powerPellet.setPosition({ x + 8.f, y + 8.f });
                window.draw(powerPellet);
            }
        }
    }
}

void drawPlayer(sf::RenderWindow& window)
{
    sf::CircleShape pacman(12.f);
    pacman.setFillColor(sf::Color::Yellow);
    pacman.setPosition({ playerCol * TILE + 3.f, HUD_HEIGHT + playerRow * TILE + 3.f });
    window.draw(pacman);
}

void drawGhosts(sf::RenderWindow& window)
{
    sf::CircleShape ghost(12.f);
    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghost.setFillColor(ghostBlue[i] ? sf::Color(90, 120, 255) : GHOST_COLORS[i]);
        ghost.setPosition({ ghostCol[i] * TILE + 3.f, HUD_HEIGHT + ghostRow[i] * TILE + 3.f });
        window.draw(ghost);
    }
}

// score on the left, lives as yellow circles on the right
void drawHUD(sf::RenderWindow& window)
{
    if (fontLoaded)
    {
        sf::Text scoreText(font, "Score: " + to_string(score), 24);
        scoreText.setPosition({ 15.f, 10.f });
        window.draw(scoreText);
    }

    sf::CircleShape lifeIcon(10.f);
    lifeIcon.setFillColor(sf::Color::Yellow);
    for (int i = 0; i < lives; i++)
    {
        lifeIcon.setPosition({ WIDTH - 35.f - i * 28.f, 15.f });
        window.draw(lifeIcon);
    }
}

void drawPlaying(sf::RenderWindow& window)
{
    if (mazeLoaded)
    {
        drawMaze(window);
        drawPlayer(window);
        drawGhosts(window);
        drawHUD(window);
    }
    else if (fontLoaded)
    {
        sf::Text error(font, "Error: maze.txt not found or invalid", 22);
        error.setFillColor(sf::Color::Red);
        error.setPosition({ 20.f, 300.f });
        window.draw(error);
    }
}

void drawMenu(sf::RenderWindow& window)
{
    if (!fontLoaded)
        return;

    sf::Text title(font, "PAC-MAN", 64);
    title.setFillColor(sf::Color::Yellow);
    sf::FloatRect b = title.getLocalBounds();
    title.setPosition({ (WIDTH - b.size.x) / 2.f, 120.f });
    window.draw(title);

    const string labels[MENU_COUNT] = { "New Game", "Continue", "Exit" };
    for (int i = 0; i < MENU_COUNT; i++)
    {
        bool selected = (i == menuChoice);
        string line = selected ? "> " + labels[i] : labels[i];

        sf::Text item(font, line, 32);
        item.setFillColor(selected ? sf::Color::Yellow : sf::Color::White);
        b = item.getLocalBounds();
        item.setPosition({ (WIDTH - b.size.x) / 2.f, 270.f + i * 60.f });
        window.draw(item);
    }

    if (noSaveMessage)
    {
        sf::Text msg(font, "No saved game", 24);
        msg.setFillColor(sf::Color::Red);
        b = msg.getLocalBounds();
        msg.setPosition({ (WIDTH - b.size.x) / 2.f, 480.f });
        window.draw(msg);
    }

    sf::Text hint(font, "Up/Down to choose, Enter to select", 18);
    hint.setFillColor(sf::Color(150, 150, 150));
    b = hint.getLocalBounds();
    hint.setPosition({ (WIDTH - b.size.x) / 2.f, 600.f });
    window.draw(hint);
}

// message, final score and "Press Enter"
void drawGameOverScreen(sf::RenderWindow& window)
{
    if (!fontLoaded)
        return;

    sf::Text title(font, playerWon ? "YOU WIN" : "GAME OVER", 56);
    title.setFillColor(playerWon ? sf::Color::Green : sf::Color::Red);
    sf::FloatRect b = title.getLocalBounds();
    title.setPosition({ (WIDTH - b.size.x) / 2.f, 220.f });
    window.draw(title);

    sf::Text scoreText(font, "Final Score: " + to_string(score), 30);
    b = scoreText.getLocalBounds();
    scoreText.setPosition({ (WIDTH - b.size.x) / 2.f, 320.f });
    window.draw(scoreText);

    sf::Text hint(font, "Press Enter", 24);
    hint.setFillColor(sf::Color(180, 180, 180));
    b = hint.getLocalBounds();
    hint.setPosition({ (WIDTH - b.size.x) / 2.f, 390.f });
    window.draw(hint);
}

void drawGame(sf::RenderWindow& window)
{
    window.clear();

    switch (gameState)
    {
    case MENU:
        drawMenu(window);
        break;
    case PLAYING:
        drawPlaying(window);
        break;
    case GAME_OVER:
        drawGameOverScreen(window);
        break;
    }

    window.display();
}

int main()
{
    srand(time(0));
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "Pac-Man");
    window.setFramerateLimit(60);

    fontLoaded = font.openFromFile("arial.ttf");
    if (!fontLoaded)
        cout << "Could not load arial.ttf" << endl;

    mazeLoaded = loadMaze("maze.txt");
    if (!mazeLoaded)
        cout << "Could not load maze.txt" << endl;

    resetPositions();

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}