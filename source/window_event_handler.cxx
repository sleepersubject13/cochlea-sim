#include "window_event_handler.hpp"

WindowEventHandler::WindowEventHandler(sf::RenderWindow& window) : m_window{window} {}

void WindowEventHandler::operator()(const sf::Event::Closed&) {
    m_window.close();
}

void WindowEventHandler::operator()(const sf::Event::KeyPressed& key_pressed) {
    switch (key_pressed.scancode) {
        case sf::Keyboard::Scancode::Escape:
            m_window.close();
            break;
        default:
            break;
    }
}