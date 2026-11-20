#include "inner_hair_cell.hxx"


#include <mutex>
#include <cmath>
#include <algorithm>

InnerHairCell::InnerHairCell(std::size_t index, Vec2 position, Vec2 size)
    : m_index{index}, m_position{position}, m_size{size} {}

bool InnerHairCell::try_absorb_particle(Vec2 particle_pos) {
    // Fast lock-free bounding-box check first (m_pos and m_size are const!)
    bool inside_x = (particle_pos.x >= m_position.x) && (particle_pos.x <= m_position.x + m_size.x);
    bool inside_y = (particle_pos.y >= m_position.y) && (particle_pos.y <= m_position.y + m_size.y);

    if (!inside_x || !inside_y) {
        return false; // Particle missed this cell; zero lock overhead!
    }

    // Particle hit the cell membrane: acquire exclusive lock to absorb viral vector
    std::unique_lock<std::shared_mutex> lock{m_mutex};
    ++m_viral_copies;
    return true;
}

std::optional<SynapticFireEvent> InnerHairCell::update_biology(float dt, float wave_phase, float sound_amplitude) {
    std::unique_lock<std::shared_mutex> lock{m_mutex};

    // 1. Protein Synthesis: Each absorbed AAV-OTOF copy transcribes otoferlin over time
    if (m_viral_copies > 0 && m_otoferlin_level < 1.0f) {
        // Diminishing returns curve so 1 copy heals slowly, 5+ copies heal rapidly
        float synthesis_rate = 0.08f * std::min(static_cast<float>(m_viral_copies), 6.0f);
        m_otoferlin_level = std::min(1.0f, m_otoferlin_level + synthesis_rate * dt);
    }

    // 2. Mechanical Deflection: Sound wave sweeps across cells with slight phase offset per cell
    float local_phase = wave_phase - static_cast<float>(m_index) * 0.45f;
    m_deflection = std::sin(local_phase) * sound_amplitude;

    // 3. Refractory Timer Cooldown
    if (m_refractory_timer > 0.0f) {
        m_refractory_timer = std::max(0.0f, m_refractory_timer - dt);
    }

    // 4. Synaptic Vesicle Release Check (The Karolinska OTOF Restoration Milestone!)
    // Requires: (a) Otoferlin protein >= 50%, (b) Strong positive wave deflection, (c) Ready to fire
    constexpr float kOtoferlinThreshold = 0.50f;
    constexpr float kDeflectionTrigger  = 0.45f;
    // Max 4 spikes/sec per cell for clean visuals
    constexpr float kRefractoryCooldown = 0.25f;

    if (m_otoferlin_level >= kOtoferlinThreshold &&
        m_deflection >= kDeflectionTrigger &&
        m_refractory_timer <= 0.0f) {
        
        m_refractory_timer = kRefractoryCooldown;
        return SynapticFireEvent{m_index, m_otoferlin_level, m_deflection};
    }

    return {};
}

void InnerHairCell::reset() {
    std::unique_lock<std::shared_mutex> lock{m_mutex};
    m_viral_copies = 0;
    m_otoferlin_level = 0.0f;
    m_deflection = 0.0f;
    m_refractory_timer = 0.0f;
}

HairCellVisual InnerHairCell::get_visual_state() const {
    std::shared_lock<std::shared_mutex> lock{m_mutex};
    return HairCellVisual{
        m_position,
        m_size,
        m_viral_copies,
        m_otoferlin_level,
        m_deflection
    };
}
