#include "window_event_handler.hpp"

WindowEventHandler::WindowEventHandler(sf::RenderWindow& window, CochleaEngine& engine):
    m_window{window}, m_engine{engine} {}

void WindowEventHandler::operator()(const sf::Event::Closed&) {
    m_window.close();
}

void WindowEventHandler::operator()(const sf::Event::KeyPressed& key_pressed) {
   if (key_pressed.code == sf::Keyboard::Key::Space) {
    // Spacebar injects a standard dose directly through the Round Window Membrane
        m_engine.inject_viral_dose(Vec2{82.0f, 245.0f}, 250);
    } else if (key_pressed.code == sf::Keyboard::Key::Up) {
        m_engine.adjust_sound_amplitude(0.10f);
    } else if (key_pressed.code == sf::Keyboard::Key::Down) {
        m_engine.adjust_sound_amplitude(-0.10f);
    } else if (key_pressed.code == sf::Keyboard::Key::R) {
        m_engine.reset_simulation();
    }
}

void WindowEventHandler::operator()(const sf::Event::MouseButtonPressed& mouse_click) {
     if (mouse_click.button == sf::Mouse::Button::Left) {
            float click_x {static_cast<float>(mouse_click.position.x)};
            float click_y {static_cast<float>(mouse_click.position.y)};

        // Inject AAV-OTOF viral vectors inside the fluid chamber
        if (click_x >= 70.0f && click_x <= CochleaEngine::k_window_width - 45.0f &&
            click_y >= 110.0f && click_y <= 380.0f) {
                    m_engine.inject_viral_dose(Vec2{click_x, click_y}, 250);
                }
        }
}