#include <fmt/core.h>
#include <fmt/format.h>

#include <SFML/Graphics.hpp>

#include "fractal-serial.h"
#include "fractal-simd.h"

#include <algorithm>
#include <limits>
#include <string>

// Ejes en coordenadas x,y
double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.5;
double y_max = 1.5;

int max_iteraciones = 10;

#define WIDTH 1600
#define HEIGHT 900

// imagen, textura
uint32_t *buffer = nullptr;

enum class runtime_type
{
    SERIAL_I,
    SIMD
};

int main()
{

    buffer = new uint32_t[WIDTH * HEIGHT]{};

    runtime_type r_type = runtime_type::SERIAL_I;

    // Create the main window
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Dibujo funcion");

    // configurar textos
    // configuracion texto Fps e iteraciones
    sf::Font font("arial.TTF");
    sf::Text text(font, "Fractal", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    // configuracion texto inferior
    std::string options = "[1] Serial |[3] SIMD | Up/Down: Iterations";
    sf::Text text_options(font, options, 20);
    text_options.setFillColor(sf::Color::White);
    text_options.setPosition({10, window.getView().getSize().y - 40});
    text_options.setStyle(sf::Text::Bold);

    // Load a sprite to display
    sf::Texture texture({WIDTH, HEIGHT});
    sf::Sprite sprite(texture);

    texture.update((const uint8_t *)buffer);

    int frames = 0;
    int fps = 0;
    sf::Clock clock;

    // Start the game loop
    while (window.isOpen())
    {
        // Process events
        while (const std::optional event = window.pollEvent())
        {
            // sf::Event::KeyPressed
            // Close window: exit
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (event->is<sf::Event::KeyReleased>())
            {
                auto evt = event->getIf<sf::Event::KeyReleased>();

                switch (evt->scancode)
                {
                case sf::Keyboard::Scan::Up:
                    if (max_iteraciones <= std::numeric_limits<int>::max() - 10)
                        max_iteraciones += 10;
                    break;
                case sf::Keyboard::Scan::Down:
                    // Mantener un presupuesto positivo al pulsar Down.
                    max_iteraciones = std::max(10, max_iteraciones - 10);
                    break;
                case sf::Keyboard::Scan::Num1:
                    r_type = runtime_type::SERIAL_I;
                    break;
                case sf::Keyboard::Scan::Num3:
                    r_type = runtime_type::SIMD;
                    break;
                }
            }
        }
        if (!window.isOpen())
            break;

        // Cada modo escribe todos los píxeles antes de actualizar la textura.

        if (r_type == runtime_type::SERIAL_I)
        {
            julia_serial(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, buffer);
        }
        else if (r_type == runtime_type::SIMD)
        {
            julia_simd(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, buffer);
        }
        texture.update((const uint8_t *)buffer);

        // FPS del ciclo completo: cálculo, textura, texto y dibujo.
        frames++;

        if (clock.getElapsedTime().asSeconds() >= 1.0f)
        {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // actualizar los textos
        auto msg = fmt::format("Fractal: {} | Iteraciones: {} | FPS: {}",
            r_type == runtime_type::SERIAL_I ? "SERIAL_I" : "SIMD",
            max_iteraciones, fps);

        text.setString(msg);

        // Clear screen
        window.clear();

        // Draw the sprite
        window.draw(sprite);

        // Texto de Fps e iteracciones
        window.draw(text);

        // Dibujar texto de opciones
        window.draw(text_options);

        // Update the window
        window.display();
    }

    delete[] buffer;
    return 0;
}
