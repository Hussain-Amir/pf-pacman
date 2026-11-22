#include <SFML/Graphics.hpp>
#include <optional>
#include <fstream>
#include <iostream>
using namespace std;

const int WIDTH = 570;
const int HEIGHT = 680;
const float TILE = 30.f;
const int ROWS = 21;
const int COLS = 19;
const float HUD_HEIGHT = 50.f;
const int NUM_GHOSTS = 4;
const int PLAYER_SPEED = 8;

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

const int dRow[5] = { 0, -1, 1, 0, 0 };
const int dCol[5] = { 0, 0, 0, -1, 1 };

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

void drawGame(sf::RenderWindow& window)
{
    window.clear();

    if (mazeLoaded)
    {
        drawMaze(window);
        drawPlayer(window);
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
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "Pac-Man");
    window.setFramerateLimit(60);

    fontLoaded = font.openFromFile("arial.ttf");
    if (!fontLoaded)
        cout << "Could not load arial.ttf" << endl;

    mazeLoaded = loadMaze("maze.txt");
    if (!mazeLoaded)
        cout << "Could not load maze.txt" << endl;

    playerRow = playerStartRow;
    playerCol = playerStartCol;

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}