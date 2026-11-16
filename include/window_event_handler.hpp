#ifndef WINDOW_EVENT_HANDLER_HPP_
#define WINDOW_EVENT_HANDLER_HPP_

#include <SFML/Graphics.hpp>

class WindowEventHandler {
    public:
        explicit WindowEventHandler(sf::RenderWindow&);
        void operator()(const sf::Event::Closed&);
        void operator()(const sf::Event::KeyPressed&);

    private:
        sf::RenderWindow& m_window;
};
#endif