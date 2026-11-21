#include <SFML/Graphics.hpp>
#include <optional>

const int WIDTH = 570;
const int HEIGHT = 680;

void handleInput(sf::RenderWindow& window)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();
    }
}


void updateGame()
{
}


void drawGame(sf::RenderWindow& window)
{
    window.clear();
    window.display();
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "Pac-Man");
    window.setFramerateLimit(60);   

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}