#include "cochlea_engine.hxx"
#include "window_event_handler.hpp"


#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>

// Helper to smoothly blend two SFML colors based on t in [0.0, 1.0]
sf::Color lerp_color(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return sf::Color(
        static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<std::uint8_t>(a.b + (b.b - a.b) * t)
    );
}

int main() {

    // 1. Create SFML 3.0 RenderWindow
    sf::RenderWindow window(
        sf::VideoMode({CochleaEngine::k_window_width, CochleaEngine::k_window_height}),
        "CochleaSim - AAV-OTOF Inner Ear Gene Therapy Simulator"
    );
    window.setFramerateLimit(60);

    // 2. Load a standard Linux/WSL system font for the telemetry HUD
    sf::Font font;
    bool font_loaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") ||
                       font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");

    if (!font_loaded) {
        std::cerr << "[Warning] System TTF font not found. HUD text will be hidden.\n";
    }

    // 3. Start the Multi-Threaded Cochlea Biology Engine
    CochleaEngine engine(static_cast<float>(CochleaEngine::k_window_width), static_cast<float>(CochleaEngine::k_window_height));

    // Static anatomical shapes
    sf::RectangleShape chamber_box({static_cast<float>(CochleaEngine::k_window_width) - 110.0f, 285.0f});
    chamber_box.setPosition({70.0f, 105.0f});
    chamber_box.setFillColor(sf::Color(18, 28, 44));
    chamber_box.setOutlineThickness(2.0f);
    chamber_box.setOutlineColor(sf::Color(55, 85, 120));

    // Round Window Membrane (Injection Target on Left Wall)
    sf::RectangleShape round_window({14.0f, 150.0f});
    round_window.setPosition({63.0f, 170.0f});
    round_window.setFillColor(sf::Color(0, 220, 235));


    // 4. Main 60 FPS Render & Input Loop
    while (window.isOpen()) {

        // --- A. Register the poll events to be handled.
        // SFML 3.0 says we can pass in a functor to `window.handleEvents`, with overloaded function-call operators,
        // each handling the particular kind of sf::Event we want. As such, we do not have to implicity write code like:
        //
        // if (const auto* key_pressed = event->get_if<sf::Event::KeyPressed>()) {
        //  ...
        // }
        //
        // instead, we can have something like:
        // ...
        // void operator()(const sf::Event::KeyPressed& key_pressed) {
        // if (key_pressed == sf::KeyBoard::Key::Up) {
        // ...
        //  }
        // }
        // ...
        window.handleEvents(WindowEventHandler{window, engine});
       
        // --- B. Grab Double-Buffered Snapshot (Zero Physics Stutter) ---
        RenderSnapshot snap {engine.get_render_snapshot()};

        // --- C. Render Frame ---
        window.clear(sf::Color(11, 15, 24));

        // 1. Draw Scala Tympani Fluid Chamber & Round Window Membrane
        window.draw(chamber_box);
        window.draw(round_window);

        // 2. Draw Incoming Sound Wave across top of fluid chamber
        constexpr std::size_t kWavePoints {180};
        sf::VertexArray sound_wave(sf::PrimitiveType::LineStrip, kWavePoints);
        for (std::size_t i {}; i < kWavePoints; ++i) {
            float t {static_cast<float>(i) / static_cast<float>(kWavePoints - 1)};
            float x {80.0f + t * (CochleaEngine::k_window_width - 130.0f)};
            float y {155.0f + std::sin(snap.wave_phase - t * 12.0f) * (32.0f * snap.sound_amplitude)};
            sound_wave[i].position = {x, y};
            sound_wave[i].color = sf::Color(120, 190, 255, 200);
        }
        window.draw(sound_wave);

        // 3. Draw Auditory Nerve Fibers & Inner Hair Cells
        // Mutated OTOF (No otoferlin)
        const sf::Color damaged_color(175, 75, 65);
        // Restored Otoferlin synthesis
        const sf::Color restored_color(45, 225, 130);

        for (std::size_t i {}; i < snap.cells.size(); ++i) {
            const auto& cell = snap.cells[i];
            float center_x = cell.position.x + cell.size.x * 0.5f;
            float base_y   = cell.position.y + cell.size.y;

            // Vertical Auditory Nerve Fiber line extending to brainstem
            sf::VertexArray nerve_line(sf::PrimitiveType::Lines, 2);
            nerve_line[0].position = {center_x, base_y};
            nerve_line[0].color    = sf::Color(70, 90, 115);
            nerve_line[1].position = {center_x, static_cast<float>(CochleaEngine::k_window_height) - 35.0f};
            nerve_line[1].color    = sf::Color(40, 55, 75);
            window.draw(nerve_line);

            // Inner Hair Cell Body (Color shifts as otoferlin protein builds up)
            sf::RectangleShape cell_body({cell.size.x, cell.size.y});
            cell_body.setPosition({cell.position.x, cell.position.y});
            cell_body.setFillColor(lerp_color(damaged_color, restored_color, cell.otoferlin_level));
            cell_body.setOutlineThickness(2.0f);
            cell_body.setOutlineColor(sf::Color(210, 225, 240, 140));
            window.draw(cell_body);

            // Stereocilia (Sensory hairs on top of cell that bend with the sound wave)
            sf::VertexArray cilia(sf::PrimitiveType::Lines, 6);
            for (int c {-1}; c <= 1; ++c) {
                std::size_t idx = static_cast<std::size_t>((c + 1) * 2);
                float root_x = center_x + static_cast<float>(c) * 10.0f;
                float tip_x  = root_x + cell.deflection * 18.0f;
                cilia[idx].position     = {root_x, cell.position.y};
                cilia[idx].color        = sf::Color(230, 240, 255);
                cilia[idx + 1].position = {tip_x, cell.position.y - 22.0f};
                cilia[idx + 1].color    = sf::Color(230, 240, 255);
            }
            window.draw(cilia);
        }

        // 4. Draw Active Synaptic Action Potentials (Electrical Nerve Pulses)
        for (const auto& pulse : snap.active_pulses) {
            if (pulse.cell_index >= snap.cells.size()) continue;
            const auto& cell = snap.cells[pulse.cell_index];
            float center_x = cell.position.x + cell.size.x * 0.5f;
            float start_y  = cell.position.y + cell.size.y;
            float end_y    = static_cast<float>(CochleaEngine::k_window_height) - 35.0f;
            float pulse_y  = start_y + pulse.progress * (end_y - start_y);

            sf::CircleShape spark(7.0f);
            spark.setOrigin({7.0f, 7.0f});
            spark.setPosition({center_x, pulse_y});
            spark.setFillColor(sf::Color(255, 240, 90));
            window.draw(spark);
        }

        // 5. Batch-Render All Active AAV-OTOF Viral Vectors in ONE GPU Draw Call!
        sf::VertexArray viral_cloud(sf::PrimitiveType::Points, snap.particle_positions.size());
        for (std::size_t i {}; i < snap.particle_positions.size(); ++i) {
            viral_cloud[i].position = {snap.particle_positions[i].x, snap.particle_positions[i].y};
            viral_cloud[i].color    = sf::Color(0, 245, 255);
        }
        window.draw(viral_cloud);

        // 6. Draw Clinical Telemetry HUD
        if (font_loaded) {
            std::ostringstream hud_stream;
            hud_stream << std::fixed << std::setprecision(1)
                       << "CochleaSim | AAV-OTOF Gene Therapy Simulation\n"
                       << "Active AAV Vectors: " << snap.particle_positions.size()
                       << "   |   Synaptic Spikes: " << snap.total_spikes
                       << "   |   Hearing Threshold: " << snap.estimated_hearing_db << " dB "
                       << (snap.estimated_hearing_db <= 45.0f ? "[RESTORED]" : "[IMPAIRED]") << "\n"
                       << "Controls: [Left-Click / Space] Inject AAV-OTOF Dose   "
                       << "[Up/Down] Sound Volume (" << static_cast<int>(snap.sound_amplitude * 100.0f) << "%)   "
                       << "[R] Reset";

            sf::Text hud_text(font, hud_stream.str(), 15);
            hud_text.setPosition({70.0f, 22.0f});
            hud_text.setFillColor(sf::Color(220, 232, 245));
            window.draw(hud_text);
        }

        window.display();
    }

    return EXIT_SUCCESS;
}
