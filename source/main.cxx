#include <SFML/Graphics.hpp>

#include "window_event_handler.hpp"

int main() {

    sf::RenderWindow app {sf::VideoMode({1000, 600}), "Cochlea Simulator"};

    app.setFramerateLimit(60);

    WindowEventHandler event_handler {app};

    while (app.isOpen()) {
        app.handleEvents(event_handler);
    }
}