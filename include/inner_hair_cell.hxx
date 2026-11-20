#ifndef INNER_HAIR_CELL_HXX_
#define INNER_HAIR_CELL_HXX_

#include "sim_types.hxx"

#include <optional>
#include <shared_mutex>

class InnerHairCell {
    public:
        InnerHairCell(std::size_t index, Vec2 position, Vec2 size);

        // We need to make this class non-copyable as well as non-movable as
        // it owns a std::shared_mutex
        InnerHairCell(const InnerHairCell&) = delete;
        InnerHairCell& operator=(const InnerHairCell&) = delete;
        
        // 1. VIRAL DIFFUSION WORKER INTERFACE (Exclusive Write)
        // Checks collision with particle_pos; if inside cell, absorbs vector and returns true.
        bool try_absorb_particle(Vec2 particle_pos);

        // 2. CELLULAR BIOLOGY WORKER INTERFACE (Exclusive Write)
        // Synthesizes otoferlin protein from absorbed viral copies and updates sound wave deflection.
        // Returns a SynapticFireEvent if otoferlin >= threshold and a sound crest triggers firing
        std::optional<SynapticFireEvent> update_biology(float dt, float wave_phase, float sound_amplitude);

        // 3. RESET INTERFACE (Exclusive Write)
        // Resets cell back to damaged/deaf state (0 viral copies, 0.0 otoferlin).
        void reset();

        // 4. RENDERER SNAPSHOT INTERFACE (Parallel Shared Read)
        HairCellVisual get_visual_state() const;
    private:
        const std::size_t m_index {};
        const Vec2 m_position;
        const Vec2 m_size;

        std::int_fast64_t m_viral_copies {};
        // 0.0 (damaged/deaf) -> 1.0 (fully restored)
        float m_otoferlin_level {.0f};
        // -1.0 to +1.0 mechanical bend from sound wave
        float m_deflection {.0f};
        // Cooldown between synaptic nerve spikes
        float m_refractory_timer {.0f};

        mutable std::shared_mutex m_mutex;
};

#endif