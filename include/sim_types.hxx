#ifndef SIM_TYPES_HPP_
#define SIM_TYPES_HPP_

#include <vector>
#include <cstddef>
#include <cinttypes>

// NOTE: We will follow the so-called clean architecture in this implementation.

// This is analogous to SFML's Vector2f struct. We want this file to be complete
// independent of SFMLs types, so that we are better able to design and redesign
// it comfortably withiut having to keep up with SFML's changing API.
// In the code consuming this file, for example, in a sf::Vertex, we can speficy
// an sf::Vector2f (for position of a vertex) like so:
//
// --> [Structured bindings from C++17, and Designated initializers from C++20] <--
// const auto [x, y] = Vec2 {.x = 20.0f, .y = 10.0f};
// 
// sf::Vector2f{x, y};
//
struct Vec2 {
    float x {.0f};
    float y {.0f};
};

// In nature, AAVs are naturally harmless and do not cause any diseases in humans.
// They are often used in gene therapy where to deliver healthy, beneficial genes into
// human cells (think of a microscopic syringe). The wonderful scientists at Karolinsk Institutet
// engineered a working version of the OTOF gene (the gene responsible for deafness)
// in humans, and seeing as it is too large, used an Adeno-Associated virus for the delivery
// into the inner hair cells.
struct AdenoAssociatedVirus {
    Vec2 position;
    Vec2 velocity;
    bool is_active {true};
};

struct SynapticFireEvent {
    // This identifies which Inner Hair Cell fired (for example, cell 0 through 9 along the cochlea).
    // The auditory nerve thread uses this index to know which nerve fiber should light u
    std::size_t cell_index {};
    // The lack/abbsence of this protein in the inner hair cells results in the inability
    // of said cells to successfully conduct an electrical signal from the
    // sound waves passing over them. Said signal is never passed onto the auditory
    // nerve, and as such, the brain never gets to interpret the sound waves
    float otoferlin_level {.0f};
    float stimulus_strenght {.0f};
};

struct NervePulse {
    // Tells the SFML renderer which vertical nerve fiber line to draw the glowing
    // pulse on (matching the hair cell that fired)
    std::size_t cell_index {};
    // A normalized percentage (0.0f to 1.0f) tracking how far the pulse has
    // traveled down the nerve fiber:
    //
    // 0.0f: The pulse just sparked at the base of the Inner Hair Cell
    // 0.5f: The pulse is halfway down the auditory nerve fiber
    // 1.0f: The pulse has reached the bottom (the auditory brainstem) and is removed from the active list
    float progress {.0f};
};

struct HairCellVisual {
    Vec2 position;
    Vec2 size;
    // How many AAV-OTOF viral vectors have successfully entered this cell so fa?
    // This will be useful for displaying small visual markers or tooltips showing
    // gene delivery progress :-)
    int viral_copies {};
    // A normalized ratio from 0.0 (mutated/deaf) to 1.0 (healthy protein levels).
    // SFML uses this number to smoothly blend the cell's fill color from dim amber
    // to glowing emerald green
    float otoferlin_level {.0f};
    // Represents the mechanical bending of the cell's tiny sensory hairs which we call stereocilia
    // as a sound wave passes over it (oscillating between -1.0 and +1.0). We will nistruct SFML to use
    // this to visually tilt the sensory hairs left and right
    float deflection {.0f};
};

    // A photograph of the entire simulation at one frozen instant. Every few milliseconds,
    // the simulation threads package the latest positions, cell states, nerve pulses, and HUD
    // numbers into one RenderSnapshot object
struct RenderSnapshot {
    // Only the (x, y) coordinates of currently active viral particles stripped of velocities and flags
    // for SFML to be able to upload them straight into a sf::VertexArray in one fast pass
    std::vector<Vec2> particle_positions;
    // The visual state of all 8–10 Inner Hair Cells in the cochlea
    std::vector<HairCellVisual> cells;
    // Every electrical action potential currently racing down the auditory nerve fibers
    std::vector<NervePulse> active_pulses;
    // Controlled by Up/Down arrow keys (0.0 to 1.0)
    // sf::Keyboard::ScanCode::Up/ sf::Keyboard::ScanCode::Down
    float sound_amplitude{0.6f};
    // This tracks the horizontal shift of the sine wave over time so SFML can animate
    // the wave rippling smoothly across the top of the screen
    float wave_phase{0.0f};
    // Total number of synaptic nerve impulses fired since the simulation
    // started (displayed on the HUD)
    std::uint64_t total_spikes{0};
    // A live clinical hearing threshold metric calculated by the auditory nerve thread,
    // starting at 100 dB (profound hearing loss when otoferlin is 0%) and dropping down
    // toward 20 dB (normal, restored hearing) as hair cells recover
    float estimated_hearing_db{100.0f};
};

// Ab audiogram report. Returned when a userwants to calculate the clinical audiogram
// diagnostic by pressing 'T' on their keyboard (T= Test)
struct AudiogramReport {
    float avg_otoferlin_pct{0.0f};
    float pure_tone_threshold_db{100.0f};
    int restored_cell_count{};
    std::string clinical_status{"Untreated"};
};


#endif