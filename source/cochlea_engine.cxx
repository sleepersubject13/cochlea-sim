#include "cochlea_engine.hxx"
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>

CochleaEngine::CochleaEngine(float chamber_width, float chamber_height)
    : m_width{chamber_width}, m_height{chamber_height} {

    // Create 10 Inner Hair Cells spaced evenly across the middle of the cochlea
    constexpr std::size_t kNumCells = 10;
    const float cell_width  = 46.0f;
    const float cell_height = 110.0f;
    const float start_x     = 140.0f;
    const float spacing     = (m_width - 220.0f) / static_cast<float>(kNumCells);
    const float cell_y      = 270.0f;

    for (std::size_t i = 0; i < kNumCells; ++i) {
        Vec2 pos{start_x + static_cast<float>(i) * spacing, cell_y};
        m_hair_cells.push_back(std::make_unique<InnerHairCell>(i, pos, Vec2{cell_width, cell_height}));
    }

    rebuild_snapshot();

    // Launch all 3 background worker threads with automatic std::stop_token injection
    m_diffusion_thread = std::jthread([this](std::stop_token st) { diffusion_worker_loop(st); });
    m_biology_thread   = std::jthread([this](std::stop_token st) { biology_worker_loop(st); });
    m_nerve_thread     = std::jthread([this](std::stop_token st) { nerve_consumer_loop(st); });
}

CochleaEngine::~CochleaEngine() {
    // Explicitly request stop and join threads before member vectors/mutexes are destroyed
    m_diffusion_thread.request_stop();
    m_biology_thread.request_stop();
    m_nerve_thread.request_stop();
}

void CochleaEngine::inject_viral_dose(Vec2 injection_point, int count) {
    // Thread-local RNG so main thread & workers never share random state
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> angle_dist{0.0f, 6.283185f};
    std::uniform_real_distribution<float> speed_dist{35.0f, 130.0f};

    std::lock_guard<std::mutex> lock{m_particles_mutex};
    for (int i = 0; i < count; ++i) {
        float angle = angle_dist(rng);
        float speed = speed_dist(rng);
        // Bias drift slightly rightward from the Round Window Membrane into the cochlea
        Vec2 velocity{std::cos(angle) * speed + 25.0f, std::sin(angle) * speed};
        m_particles.push_back(AdenoAssociatedVirus{injection_point, velocity, true});
    }
}

void CochleaEngine::adjust_sound_amplitude(float delta) {
    float current = m_sound_amplitude.load();
    float updated = std::clamp(current + delta, 0.0f, 1.0f);
    m_sound_amplitude.store(updated);
}

void CochleaEngine::reset_simulation() {
    {
        std::lock_guard<std::mutex> p_lock{m_particles_mutex};
        m_particles.clear();
    }
    {
        std::lock_guard<std::mutex> pulse_lock{m_pulses_mutex};
        m_active_pulses.clear();
    }
    for (auto& cell : m_hair_cells) {
        cell->reset();
    }
    m_total_spikes.store(0);
    m_estimated_hearing_db.store(100.0f);
    rebuild_snapshot();
}

RenderSnapshot CochleaEngine::get_render_snapshot() const {
    // Parallel read lock: 60 FPS UI copies snapshot without blocking physics threads long
    std::shared_lock<std::shared_mutex> lock{m_snapshot_mutex};
    return m_latest_snapshot;
}

// =========================================================
// WORKER 1: BROWNIAN VIRAL DIFFUSION & RECEPTOR BINDING (120 Hz)
// =========================================================
void CochleaEngine::diffusion_worker_loop(std::stop_token stoken) {
    std::mt19937 rng{1337};
    std::uniform_real_distribution<float> brownian_nudge{-18.0f, 18.0f};

    constexpr float dt = 1.0f / 120.0f;
    const float fluid_top    = 110.0f;
    const float fluid_bottom = 385.0f;
    const float fluid_left   = 70.0f;
    const float fluid_right  = m_width - 40.0f;

    while (!stoken.stop_requested()) {
        {
            std::lock_guard<std::mutex> lock{m_particles_mutex};
            for (auto& p : m_particles) {
                if (!p.is_active) continue;

                // Apply random Brownian fluid collisions
                p.velocity.x = std::clamp(p.velocity.x + brownian_nudge(rng), -160.0f, 160.0f);
                p.velocity.y = std::clamp(p.velocity.y + brownian_nudge(rng), -160.0f, 160.0f);

                // Move particle (2D kinematics)
                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;

                // Bounce off cochlear chamber walls
                if (p.position.x < fluid_left)   { p.position.x = fluid_left;   p.velocity.x = -p.velocity.x; }
                if (p.position.x > fluid_right)  { p.position.x = fluid_right;  p.velocity.x = -p.velocity.x; }
                if (p.position.y < fluid_top)    { p.position.y = fluid_top;    p.velocity.y = -p.velocity.y; }
                if (p.position.y > fluid_bottom) { p.position.y = fluid_bottom; p.velocity.y = -p.velocity.y; }

                // Check if viral particle binds to any Inner Hair Cell
                for (auto& cell : m_hair_cells) {
                    if (cell->try_absorb_particle(p.position)) {
                        // Absorbed!
                        p.is_active = false;
                        break;
                    }
                }
            }

            // Clean up absorbed particles cleanly at end of tick
            std::erase_if(m_particles, [](const AdenoAssociatedVirus& p) { return !p.is_active; });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

// =========================================================
// WORKER 2: OTOFERLIN SYNTHESIS, SOUND WAVES & SNAPSHOTS (120 Hz)
// =========================================================
void CochleaEngine::biology_worker_loop(std::stop_token stoken) {
    constexpr float dt = 1.0f / 120.0f;

    while (!stoken.stop_requested()) {
        // 1. Advance incoming sound wave phase
        float amplitude = m_sound_amplitude.load();
        float phase = m_wave_phase.load() + (4.5f * dt);
        if (phase > 6.283185f) phase -= 6.283185f;
        m_wave_phase.store(phase);

        // 2. Update each Inner Hair Cell's otoferlin synthesis & check for synaptic firing
        for (auto& cell : m_hair_cells) {
            if (auto fire_event = cell->update_biology(dt, phase, amplitude)) {
                // Producer pushes onto Chapter 4 condition_variable queue!
                m_synaptic_queue.push(*fire_event);
            }
        }

        // 3. Advance visual electrical pulses traveling down auditory nerve fibers
        {
            std::lock_guard<std::mutex> lock{m_pulses_mutex};
            for (auto& pulse : m_active_pulses) {
                // Travels down nerve fiber in ~0.6 seconds
                pulse.progress += 1.6f * dt;
            }
            std::erase_if(m_active_pulses, [](const NervePulse& np) { return np.progress >= 1.0f; });
        }

        // 4. Package fresh RenderSnapshot for the 60 FPS SFML UI
        rebuild_snapshot();

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

// =========================================================
// WORKER 3: AUDITORY NERVE CONSUMER (Chapter 4 condition_variable_any)
// =========================================================
void CochleaEngine::nerve_consumer_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        // Sleeps at 0% CPU until a hair cell fires OR stoken shuts down the engine!
        std::shared_ptr<SynapticFireEvent> event = m_synaptic_queue.wait_and_pop(stoken);
        if (!event) {
            break; // Shutdown signal received via stop_token
        }

        // Spawn a new electrical action potential traveling down that cell's nerve fiber
        {
            std::lock_guard<std::mutex> lock{m_pulses_mutex};
            m_active_pulses.push_back(NervePulse{event->cell_index, 0.0f});
        }

        m_total_spikes.fetch_add(1);
    }
}

// =========================================================
// DOUBLE-BUFFERED SNAPSHOT BUILDER
// =========================================================
void CochleaEngine::rebuild_snapshot() {
    RenderSnapshot next_snapshot;
    next_snapshot.sound_amplitude = m_sound_amplitude.load();
    next_snapshot.wave_phase      = m_wave_phase.load();
    next_snapshot.total_spikes    = m_total_spikes.load();

    // Copy active particle positions
    {
        std::lock_guard<std::mutex> p_lock{m_particles_mutex};
        next_snapshot.particle_positions.reserve(m_particles.size());
        for (const auto& p : m_particles) {
            if (p.is_active) {
                next_snapshot.particle_positions.push_back(p.position);
            }
        }
    }

    // Copy hair cell visual states and calculate average otoferlin recovery
    float total_otoferlin = 0.0f;
    next_snapshot.cells.reserve(m_hair_cells.size());
    for (const auto& cell : m_hair_cells) {
        HairCellVisual visual = cell->get_visual_state();
        total_otoferlin += visual.otoferlin_level;
        next_snapshot.cells.push_back(visual);
    }

    // Estimate clinical hearing threshold: 100 dB (profound loss) -> 20 dB (normal hearing)
    float avg_otoferlin = m_hair_cells.empty() ? 0.0f : (total_otoferlin / m_hair_cells.size());
    float hearing_db = 100.0f - (avg_otoferlin * 80.0f);
    m_estimated_hearing_db.store(hearing_db);
    next_snapshot.estimated_hearing_db = hearing_db;

    // Copy active nerve pulses
    {
        std::lock_guard<std::mutex> pulse_lock{m_pulses_mutex};
        next_snapshot.active_pulses = m_active_pulses;
    }

    // Swap into double-buffered snapshot under exclusive write lock
    std::unique_lock<std::shared_mutex> snap_lock{m_snapshot_mutex};
    m_latest_snapshot = std::move(next_snapshot);
}
