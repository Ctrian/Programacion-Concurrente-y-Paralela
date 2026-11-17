#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <complex>
#include <cstring>

#include "fractal-simd.h"
#include "fractal-serial.h"



// Región del plano complejo que se muestrea para construir la imagen.
double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.5;
double y_max = 1.5;

// Compartido con los archivos de cálculo mediante extern.
int max_iteraciones = 10;
extern std::complex <double> c;

#define WIDTH 1280
#define HEIGHT 720

// Buffer lineal de colores: el píxel (columna i, fila j) está en j*WIDTH+i.
uint32_t *buffer = nullptr;

// Tres implementaciones del mismo problema para comparar su rendimiento.
enum class runtime_type{
    SERIAL_I,
    SERIAL_II,
    SIMD
};


int main()
{

    // new[] reserva memoria; esta forma no inicializa los colores.
    buffer = new uint32_t[WIDTH * HEIGHT];
    runtime_type r_type = runtime_type::SERIAL_I;

    // Ventana SFML y recursos para mostrar los píxeles calculados en CPU.
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Dibujo funcion");

    // Texto superior: modo seleccionado, límite de iteraciones y FPS.
    sf::Font font("arial.TTF");
    sf::Text text(font, "Fractal", 24);
    text.setFillColor(sf::Color::Black);
    text.setPosition({10,10});
    text.setStyle(sf::Text::Bold);
    
    // Ayuda de teclado en el borde inferior de la ventana.
    std::string options = "[1] Serial | [2] Serial_2 | [3] SIMD Up/Down: Iterations";
    sf::Text text_options(font, options, 20);
    text_options.setFillColor(sf::Color::Blue);
    text_options.setPosition({10, window.getView().getSize().y - 40});
    text.setStyle(sf::Text::Bold);

    // La textura almacena los píxeles; el sprite permite dibujarla en la ventana.
    sf::Texture texture({WIDTH, HEIGHT});
    sf::Sprite sprite(texture);
    
    // SFML interpreta la memoria como bytes RGBA, sin convertir cada uint32_t.
    // Esta primera copia ocurre antes del cálculo, con el buffer sin inicializar.
    texture.update((const uint8_t *)buffer);

    int frames = 0;
    int fps = 0;
    sf::Clock clock;

    // Cada ciclo procesa eventos, recalcula el fractal y dibuja la imagen.
    while (window.isOpen())
    {
        // El cambio de modo y de límite se aplica al SOLTAR la tecla.
        while (const std::optional event = window.pollEvent())
        {
            // Solicitar el cierre al recibir el evento de la ventana.
            if (event->is<sf::Event::Closed>())
                window.close();
                else if(event -> is <sf::Event::KeyReleased>()){
                    auto evt = event -> getIf<sf::Event::KeyReleased>();

                    // 1/2/3 seleccionan modo; Up/Down suman o restan diez.
                    // Actualmente no se restringe el valor mínimo ni el máximo.
                    switch (evt ->scancode){
                        case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10;
                        break;
                        case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10;
                        break;
                        case sf::Keyboard::Scan::Num1:
                        r_type = runtime_type::SERIAL_I;
                        break;
                        case sf::Keyboard::Scan::Num2:
                        r_type = runtime_type::SERIAL_II;
                        break;
                        case sf::Keyboard::Scan::Num3:
                        r_type = runtime_type::SIMD;
                        break;
                    }

                    // Pone todos los bytes a cero: RGBA 00 00 00 00
                    // (negro transparente). El cálculo siguiente vuelve a llenarlos.
                    std::memset(buffer, 0 , WIDTH*HEIGHT*sizeof(uint32_t));

                }
        }

        std::string mode ="";        
        
        // Las funciones escriben sobre el mismo buffer; no devuelven otra imagen.
        // Serial I usa std::complex<double>, Serial II componentes double
        // explícitas y SIMD ocho float por vector.
        if(r_type==runtime_type::SERIAL_I){
            mode = "Julia SERIAL_1";
            julia_serial(x_min, y_min, x_max,y_max, WIDTH, HEIGHT, buffer);
        
        }else if(r_type == runtime_type::SERIAL_II){
            mode = "Julia SERIAL_2";
            julia_serial_2(x_min, y_min, x_max,y_max, WIDTH, HEIGHT, buffer);

        }else if(r_type == runtime_type::SIMD){
            mode = "Julia SIMD";
            julia_simd(x_min, y_min, x_max,y_max, WIDTH, HEIGHT, buffer);
      
    }
    
        // Subir a la textura los colores recién calculados.
        texture.update((const uint8_t *)buffer);

        // FPS del ciclo completo (cálculo, transferencia, texto y dibujo).
        // Se cuentan frames durante intervalos de al menos un segundo.
        frames++;

        if (clock.getElapsedTime().asSeconds() >= 1.0f)
        {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // La impresión y el flush en cada frame también influyen en los FPS.
        std::cout << "FPS: " << fps << std::endl;

        // Mostrar el último valor medido; al iniciar fps vale cero.
        auto msg = fmt::format("Fractal: {} | Iteraciones: {} | FPS: {}",
        mode,
        max_iteraciones,fps);

text.setString(msg);


        // Limpiar y dibujar primero el fractal; los textos quedan por encima.
        window.clear();

        // Dibujar el sprite que utiliza la textura del fractal.
        window.draw(sprite);

        // Superponer información de rendimiento e iteraciones.
        window.draw(text);

        // Superponer la ayuda de teclado.
        window.draw(text_options);

        // Presentar el frame terminado.
        window.display();
    }

    // Pendiente: liberar el arreglo reservado con new[] mediante delete[] buffer.
    return 0;
}