#include <SFML/Graphics.hpp>

#include "threadsafe_queue.hxx"

// Uncomment the below line to test if the template class
// successfully compiles. We force the compiler to generate
// an actual instace based on the template by doing an explicit
// instantiation
// template class threadsafe_queue<double>;

int main() {

    sf::Window main_window {sf::VideoMode{sf::Vector2u{1'000, 800}}, "Cochlea Sim", sf::Style::Default};

    while (main_window.isOpen()) {

    }
}