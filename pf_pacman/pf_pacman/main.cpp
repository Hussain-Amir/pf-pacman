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
const int PELLET_POINTS = 10;
const int POWER_POINTS = 50;

char maze[ROWS][COLS];
int pelletCount = 0;
int playerStartRow = 0, playerStartCol = 0;
int ghostStartRow[NUM_GHOSTS], ghostStartCol[NUM_GHOSTS];
bool mazeLoaded = false;
sf::Font font;
bool fontLoaded = false;

int keyDir = 0;          // 0 none, 1 up, 2 down, 3 left, 4 right
int playerRow = 0, playerCol = 0;
int playerDir = 0;
int playerTimer = 0;

int score = 0;

int ghostRow[NUM_GHOSTS], ghostCol[NUM_GHOSTS];
int ghostDir[NUM_GHOSTS];
int ghostTimer[NUM_GHOSTS];

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

// puts player and ghosts on their start cells
void resetPositions()
{
    playerRow = playerStartRow;
    playerCol = playerStartCol;
    playerDir = 0;
    keyDir = 0;
    playerTimer = 0;

    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghostRow[i] = ghostStartRow[i];
        ghostCol[i] = ghostStartCol[i];
        ghostDir[i] = 0;
        ghostTimer[i] = 0;
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
    }
}

// up <-> down, left <-> right
int oppositeDir(int d)
{
    if (d == 0)
        return 0;
    return (d % 2 == 1) ? d + 1 : d - 1;
}

// moves ghost i one cell every GHOST_SPEED frames
void moveGhost(int i)
{
    ghostTimer[i]++;
    if (ghostTimer[i] < GHOST_SPEED)
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

    // draft: every ghost picks a random open direction
    int chosen = options[rand() % count];

    ghostDir[i] = chosen;
    ghostRow[i] += dRow[chosen];
    ghostCol[i] += dCol[chosen];
}

void moveGhosts()
{
    for (int i = 0; i < NUM_GHOSTS; i++)
        moveGhost(i);
}

void handleInput(sf::RenderWindow& window)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
                keyDir = 1;
            else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
                keyDir = 2;
            else if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
                keyDir = 3;
            else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
                keyDir = 4;
        }
    }
}

void updateGame()
{
    movePlayer();
    eatPellet();
    moveGhosts();
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
        ghost.setFillColor(GHOST_COLORS[i]);
        ghost.setPosition({ ghostCol[i] * TILE + 3.f, HUD_HEIGHT + ghostRow[i] * TILE + 3.f });
        window.draw(ghost);
    }
}

void drawHUD(sf::RenderWindow& window)
{
    if (!fontLoaded)
        return;
    sf::Text scoreText(font, "Score: " + to_string(score), 24);
    scoreText.setPosition({ 15.f, 10.f });
    window.draw(scoreText);
}

void drawGame(sf::RenderWindow& window)
{
    window.clear();

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