#ifndef COCHLEA_ENGINE_HXX_
#define COCHLEA_ENGINE_HXX_

#include "sim_types.hxx"
#include "inner_hair_cell.hxx"
#include "threadsafe_queue.hxx"

#include <vector>
#include <memory>
#include <thread>
#include <stop_token>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <future>

class CochleaEngine {
public:
    CochleaEngine(float chamber_width, float chamber_height);
    ~CochleaEngine();

    // Non-copyable and non-movable because it owns active worker threads & mutexes
    CochleaEngine(const CochleaEngine&) = delete;
    CochleaEngine& operator=(const CochleaEngine&) = delete;

    // 1. USER INTERACTION COMMANDS (Called from Main SFML Thread)
    void inject_viral_dose(Vec2 injection_point, int count = 250);
    void adjust_sound_amplitude(float delta);
    void reset_simulation();
    

    // 2. DOUBLE-BUFFERED RENDER SNAPSHOT (Parallel Shared Read for 60 FPS SFML)
    RenderSnapshot get_render_snapshot() const;

    // 3. ASYNCHRONOUS DIAGNOSTIC
    std::future<AudiogramReport> run_audiogram_async();

    static inline const unsigned k_window_width {1'100};
    static inline const unsigned k_window_height {680};

private:
    // Background Worker Loops (Run on dedicated std::jthreads)
    void diffusion_worker_loop(std::stop_token stoken);
    void biology_worker_loop(std::stop_token stoken);
    void nerve_consumer_loop(std::stop_token stoken);

    void rebuild_snapshot();

    const float m_width;
    const float m_height;

    // Fixed-address Inner Hair Cells (unique_ptr prevents reallocation while threads run)
    std::vector<std::unique_ptr<InnerHairCell>> m_hair_cells;

    // Active AAV-OTOF viral vectors diffusing in fluid
    mutable std::mutex m_particles_mutex;
    std::vector<AdenoAssociatedVirus> m_particles;

    // Producer-Consumer Queue for Synaptic Vesicle Firing Events
    threadsafe_queue<SynapticFireEvent> m_synaptic_queue;

    // Active electrical pulses traveling down auditory nerve fibers
    mutable std::mutex m_pulses_mutex;
    std::vector<NervePulse> m_active_pulses;

    // Atomic simulation parameters & telemetry
    std::atomic<float> m_sound_amplitude{0.65f};
    std::atomic<float> m_wave_phase{0.0f};
    std::atomic<std::uint64_t> m_total_spikes{0};
    // 100 dB (deaf) -> 20 dB (restored)
    std::atomic<float> m_estimated_hearing_db{100.0f};

    // Double-buffered snapshot protected by std::shared_mutex
    mutable std::shared_mutex m_snapshot_mutex;
    RenderSnapshot m_latest_snapshot;

    // C++20 Worker Threads (Declared LAST so they are destroyed FIRST!)
    std::jthread m_diffusion_thread;
    std::jthread m_biology_thread;
    std::jthread m_nerve_thread;
    std::jthread m_diagnostic_thread;

};

#endif
