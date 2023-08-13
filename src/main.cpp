#include <iostream>
#include "Pokemon.hpp"
#include <SFML/Graphics.hpp>
#include <curl/curl.h>

// Callback to handle data received from the server
size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

int main()
{
    std::vector<Pokemon> pokemons = Pokemon::getPokemons();

    // Window dimensions
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Square dimensions
    const int SQUARE_SIZE = 100;

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Display Pokemon Images");

    CURL *curl = curl_easy_init();

    std::vector<sf::Texture> textures(pokemons.size());
    std::vector<sf::Sprite> sprites(pokemons.size());

    // Margin between images
    const int MARGIN = 10;
    const int CANVAS_WIDTH = WINDOW_WIDTH;
    const int POKEMON_PER_ROW = CANVAS_WIDTH / (SQUARE_SIZE + MARGIN);
    const int TOTAL_ROWS = (pokemons.size() + POKEMON_PER_ROW - 1) / POKEMON_PER_ROW;
    const int CANVAS_HEIGHT = TOTAL_ROWS * (SQUARE_SIZE + MARGIN);

    int currentX = MARGIN;
    int currentY = MARGIN;
    float scrollY = 0.0f;

    for (size_t i = 0; i < pokemons.size(); ++i)
    {
        std::cout << "pokemon " << i << std::endl;

        const Pokemon &pokemon = pokemons[i];

        std::string response;
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, pokemon.getImageUrl().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                std::cout << "Failed to fetch the image for Pokemon: " << pokemon.getName()
                          << ". Error: " << curl_easy_strerror(res) << std::endl;
                continue;
            }

            sf::Image image;
            if (!image.loadFromMemory(response.data(), response.size()) ||
                !textures[i].loadFromImage(image))
            {
                std::cout << "Failed to load the image for Pokemon: " << pokemon.getName() << std::endl;
                continue;
            }

            sprites[i].setTexture(textures[i]);
            float scaleX = static_cast<float>(SQUARE_SIZE) / textures[i].getSize().x;
            float scaleY = static_cast<float>(SQUARE_SIZE) / textures[i].getSize().y;
            sprites[i].setScale(scaleX, scaleY);
            sprites[i].setPosition(currentX, currentY);

            currentX += SQUARE_SIZE + MARGIN;
            if (currentX + SQUARE_SIZE > CANVAS_WIDTH)
            {
                currentX = MARGIN;
                currentY += SQUARE_SIZE + MARGIN;
            }
        }
    }

    if (curl)
        curl_easy_cleanup(curl);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
                return 0;
            }
            else if (event.type == sf::Event::MouseWheelScrolled)
            {
                scrollY -= event.mouseWheelScroll.delta * SQUARE_SIZE;
                scrollY = std::max(0.0f, std::min(static_cast<float>(CANVAS_HEIGHT - WINDOW_HEIGHT), scrollY));
            }
        }

        window.clear(sf::Color::Black);
        for (auto &sprite : sprites)
        {
            sprite.move(0, -scrollY);
            if (sprite.getPosition().y + SQUARE_SIZE + scrollY >= 0 && sprite.getPosition().y - scrollY < WINDOW_HEIGHT)
            {
                window.draw(sprite);
            }
            sprite.move(0, scrollY);
        }
        window.display();
    }

    return 0;
}