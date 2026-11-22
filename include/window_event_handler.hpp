#ifndef WINDOW_EVENT_HANDLER_HPP_
#define WINDOW_EVENT_HANDLER_HPP_

#include <SFML/Graphics.hpp>

#include "cochlea_engine.hxx"

class WindowEventHandler {
    public:
        explicit WindowEventHandler(sf::RenderWindow&, CochleaEngine&);
        void operator()(const sf::Event::Closed&);
        void operator()(const sf::Event::KeyPressed&);
        void operator()(const sf::Event::MouseButtonPressed&);

    private:
        sf::RenderWindow& m_window;
        CochleaEngine& m_engine;

};
#endif