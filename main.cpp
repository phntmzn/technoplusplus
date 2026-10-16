// main.cpp
// Infinite Techno MIDI Generator
//
// DESCRIPTION:
//   Continuously generates randomized techno MIDI patterns and tracks.
//   Generates drums, basslines, arpeggios, chords, and full arrangements.
//
// COMPILE ON macOS:
//   clang++ -std=c++17 -O2 src/main.cpp -o infinite_techno
//
// RUN:
//   ./infinite_techno
//
// STOP:
//   Press Ctrl+C
//
// OUTPUT:
//   output/drums/
//   output/basslines/
//   output/arpeggios/
//   output/chords/
//   output/full_tracks/
//
// FEATURES:
//   - Randomized techno tempos between 125 and 145 BPM
//   - Four-on-the-floor kick drums
//   - Snare, clap, closed hi-hat, and open hi-hat patterns
//   - Randomized minor-scale basslines
//   - Synth-style arpeggio note sequences
//   - Chord progressions
//   - Multitrack Standard MIDI Files
//   - Persistent file numbering across restarts
//   - No external MIDI libraries required
//
// REQUIREMENTS:
//   C++17 or newer
//   macOS with Apple Clang or another compatible C++ compiler

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <limits>
#include <csignal>

namespace fs = std::filesystem;

// ------------------------------------------------------------
// CONFIGURATION
// ------------------------------------------------------------

constexpr int PPQ = 480;
constexpr int BEATS_PER_BAR = 4;
constexpr int TICKS_PER_BAR = PPQ * BEATS_PER_BAR;

constexpr int MIN_BPM = 125;
constexpr int MAX_BPM = 145;

constexpr int MIN_BARS = 8;
constexpr int MAX_BARS = 32;

constexpr int GENERATION_DELAY_MS = 250;

const fs::path OUTPUT_DIR = "output";
const fs::path STATE_FILE = "output/.next_id";

std::mt19937 rng(std::random_device{}());

volatile std::sig_atomic_t running = 1;

void handleSignal(int) {
    running = 0;
}

// ------------------------------------------------------------
// RANDOM HELPERS
// ------------------------------------------------------------

int randomInt(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng);
}

double randomDouble(double min, double max) {
    return std::uniform_real_distribution<double>(min, max)(rng);
}

bool chance(int percentage) {
    return randomInt(1, 100) <= percentage;
}

// ------------------------------------------------------------
// MIDI EVENT STRUCTURES
// ------------------------------------------------------------

struct Event {
    uint32_t tick;
    std::vector<uint8_t> data;
};

struct Track {
    std::vector<Event> events;
    uint32_t endTick = 0;

    void addEvent(uint32_t tick, std::vector<uint8_t> data) {
        events.push_back({tick, std::move(data)});
        endTick = std::max(endTick, tick);
    }

    void note(
        int channel,
        int pitch,
        int velocity,
        uint32_t start,
        uint32_t length
    ) {
        pitch = std::clamp(pitch, 0, 127);
        velocity = std::clamp(velocity, 1, 127);

        addEvent(start, {
            static_cast<uint8_t>(0x90 | channel),
            static_cast<uint8_t>(pitch),
            static_cast<uint8_t>(velocity)
        });

        addEvent(start + length, {
            static_cast<uint8_t>(0x80 | channel),
            static_cast<uint8_t>(pitch),
            0
        });
    }
};

// ------------------------------------------------------------
// MIDI BINARY HELPERS
// ------------------------------------------------------------

void writeBE16(std::ofstream& file, uint16_t value) {
    file.put(static_cast<char>((value >> 8) & 0xFF));
    file.put(static_cast<char>(value & 0xFF));
}

void writeBE32(std::ofstream& file, uint32_t value) {
    file.put(static_cast<char>((value >> 24) & 0xFF));
    file.put(static_cast<char>((value >> 16) & 0xFF));
    file.put(static_cast<char>((value >> 8) & 0xFF));
    file.put(static_cast<char>(value & 0xFF));
}

void writeVLQ(std::vector<uint8_t>& output, uint32_t value) {
    uint8_t buffer[5];
    int count = 0;

    buffer[count++] = static_cast<uint8_t>(value & 0x7F);

    while ((value >>= 7) != 0) {
        buffer[count++] =
            static_cast<uint8_t>((value & 0x7F) | 0x80);
    }

    while (count > 0) {
        output.push_back(buffer[--count]);
    }
}

// ------------------------------------------------------------
// MIDI TRACK ENCODING
// ------------------------------------------------------------

std::vector<uint8_t> encodeTrack(Track track) {
    std::stable_sort(
        track.events.begin(),
        track.events.end(),
        [](const Event& a, const Event& b) {
            if (a.tick != b.tick) {
                return a.tick < b.tick;
            }

            // Note-off events precede note-on events at the same tick.
            int aType = a.data[0] & 0xF0;
            int bType = b.data[0] & 0xF0;

            bool aOff = aType == 0x80;
            bool bOff = bType == 0x80;

            return aOff && !bOff;
        }
    );

    std::vector<uint8_t> output;
    uint32_t previousTick = 0;

    for (const auto& event : track.events) {
        writeVLQ(output, event.tick - previousTick);

        output.insert(
            output.end(),
            event.data.begin(),
            event.data.end()
        );

        previousTick = event.tick;
    }

    // End-of-track meta event.
    writeVLQ(output, 0);
    output.insert(output.end(), {0xFF, 0x2F, 0x00});

    return output;
}

// ------------------------------------------------------------
// MIDI FILE WRITER
// ------------------------------------------------------------

void saveMIDI(
    const fs::path& path,
    const std::vector<Track>& tracks,
    int bpm
) {
    std::ofstream file(path, std::ios::binary);

    if (!file) {
        std::cerr << "ERROR: Cannot write " << path << '\n';
        return;
    }

    // Tempo track.
    Track tempoTrack;

    uint32_t microsecondsPerQuarter =
        static_cast<uint32_t>(60000000 / bpm);

    tempoTrack.addEvent(0, {
        0xFF, 0x51, 0x03,
        static_cast<uint8_t>((microsecondsPerQuarter >> 16) & 0xFF),
        static_cast<uint8_t>((microsecondsPerQuarter >> 8) & 0xFF),
        static_cast<uint8_t>(microsecondsPerQuarter & 0xFF)
    });

    std::vector<Track> allTracks;
    allTracks.push_back(tempoTrack);

    for (const auto& track : tracks) {
        allTracks.push_back(track);
    }

    // Standard MIDI File header, format 1.
    file.write("MThd", 4);
    writeBE32(file, 6);
    writeBE16(file, 1);

    writeBE16(
        file,
        static_cast<uint16_t>(allTracks.size())
    );

    writeBE16(file, PPQ);

    for (const auto& track : allTracks) {
        auto bytes = encodeTrack(track);

        file.write("MTrk", 4);
        writeBE32(file, static_cast<uint32_t>(bytes.size()));

        file.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
    }

    if (!file) {
        std::cerr << "ERROR: Failed while writing " << path << '\n';
    }
}

// ------------------------------------------------------------
// TRACK NAMING
// ------------------------------------------------------------

Track makeNamedTrack(const std::string& name) {
    Track track;

    std::vector<uint8_t> meta = {
        0xFF, 0x03,
        static_cast<uint8_t>(name.size())
    };

    meta.insert(meta.end(), name.begin(), name.end());

    track.addEvent(0, std::move(meta));

    return track;
}

// ------------------------------------------------------------
// SCALE HELPERS
// ------------------------------------------------------------

// A natural minor scale.
const std::array<int, 7> MINOR_SCALE = {
    0, 2, 3, 5, 7, 8, 10
};

int scaleNote(int root, int degree) {
    int octave = degree / 7;
    int index = degree % 7;

    if (index < 0) {
        index += 7;
        --octave;
    }

    return root + MINOR_SCALE[index] + octave * 12;
}

// ------------------------------------------------------------
// TECHNO DRUM GENERATOR
// ------------------------------------------------------------

Track makeDrums(int bars) {
    Track drums = makeNamedTrack("Techno Drums");

    // General MIDI percussion notes, channel 10 (index 9).
    constexpr int KICK = 36;
    constexpr int SNARE = 38;
    constexpr int CLAP = 39;
    constexpr int CLOSED_HAT = 42;
    constexpr int OPEN_HAT = 46;

    uint32_t totalTicks = bars * TICKS_PER_BAR;
    constexpr uint32_t STEP = PPQ / 4;

    for (uint32_t tick = 0; tick < totalTicks; tick += STEP) {
        uint32_t step = tick / STEP;
        uint32_t beat = tick / PPQ;
        int beatPosition = static_cast<int>(beat % 4);
        int subdivision = static_cast<int>(step % 4);

        // Four-on-the-floor kick.
        if (tick % PPQ == 0) {
            drums.note(
                9, KICK, randomInt(108, 124),
                tick, 60
            );
        }

        // Syncopated kick variations.
        if (tick % PPQ != 0 && chance(10)) {
            drums.note(
                9, KICK, randomInt(75, 105),
                tick, 50
            );
        }

        // Snare on beats 2 and 4.
        if (tick % PPQ == 0 &&
            (beatPosition == 1 || beatPosition == 3)) {

            drums.note(
                9, SNARE, randomInt(88, 112),
                tick, 60
            );

            if (chance(55)) {
                drums.note(
                    9, CLAP, randomInt(72, 100),
                    tick, 60
                );
            }
        }

        // Closed hi-hats on sixteenth notes.
        if (chance(94)) {
            int velocity = subdivision == 0
                ? randomInt(75, 105)
                : randomInt(38, 78);

            drums.note(
                9, CLOSED_HAT, velocity,
                tick, 30
            );
        }

        // Open hi-hat offbeats.
        if (subdivision == 2 && chance(38)) {
            drums.note(
                9, OPEN_HAT, randomInt(65, 95),
                tick, 100
            );
        }

        // Occasional extra percussion.
        if (subdivision == 3 && chance(8)) {
            drums.note(
                9, randomInt(37, 40),
                randomInt(45, 85),
                tick, 35
            );
        }
    }

    return drums;
}

// ------------------------------------------------------------
// TECHNO BASSLINE GENERATOR
// ------------------------------------------------------------

Track makeBassline(int bars, int root) {
    Track bass = makeNamedTrack("Techno Bassline");

    uint32_t totalTicks = bars * TICKS_PER_BAR;
    uint32_t stepLength = PPQ / 4;

    int pattern = randomInt(0, 4);

    for (uint32_t tick = 0; tick < totalTicks; tick += stepLength) {
        int step = static_cast<int>(tick / stepLength);

        // Rest probability creates rhythmic variation.
        if (chance(20)) {
            continue;
        }

        int degree = 0;

        switch (pattern) {
            case 0:
                degree = (step % 4 == 0) ? 0 : randomInt(0, 2);
                break;

            case 1:
                degree = step % 7;
                break;

            case 2:
                degree = (step % 2 == 0) ? 0 : 2;
                break;

            default:
                degree = randomInt(0, 6);
                break;
        }

        int pitch = scaleNote(root, degree) - 12;

        if (chance(12)) {
            pitch -= 12;
        }

        uint32_t length = static_cast<uint32_t>(
            randomInt(90, 210)
        );

        bass.note(
            0,
            pitch,
            randomInt(78, 118),
            tick,
            length
        );
    }

    return bass;
}

// ------------------------------------------------------------
// TECHNO ARPEGGIO GENERATOR
// ------------------------------------------------------------

Track makeArpeggio(int bars, int root) {
    Track arp = makeNamedTrack("Techno Arpeggio");

    uint32_t totalTicks = bars * TICKS_PER_BAR;

    int rhythm = randomInt(0, 2);

    uint32_t stepLength = rhythm == 0
        ? PPQ / 4
        : rhythm == 1
            ? PPQ / 2
            : PPQ / 3;

    int pattern = randomInt(0, 4);

    for (uint32_t tick = 0; tick < totalTicks; tick += stepLength) {
        int step = static_cast<int>(tick / stepLength);
        int degree;

        switch (pattern) {
            case 0:
                degree = step % 4;
                break;

            case 1:
                degree = (step * 2) % 7;
                break;

            case 2:
                degree = step % 7;
                break;

            default:
                degree = randomInt(0, 6);
                break;
        }

        int pitch = scaleNote(root + 12, degree);

        // Occasional octave jumps.
        if (chance(15)) {
            pitch += 12;
        }

        // Occasional fifths for variation.
        if (chance(8)) {
            pitch += 7;
        }

        uint32_t noteLength = std::max<uint32_t>(
            30, stepLength * 3 / 4
        );

        arp.note(
            1,
            pitch,
            randomInt(65, 110),
            tick,
            noteLength
        );
    }

    return arp;
}

// ------------------------------------------------------------
// TECHNO CHORD GENERATOR
// ------------------------------------------------------------

Track makeChords(int bars, int root) {
    Track chords = makeNamedTrack("Techno Chords");

    for (int bar = 0; bar < bars; bar += 2) {
        int chordType = randomInt(0, 3);

        std::array<int, 3> intervals;

        switch (chordType) {
            case 0:
                intervals = {0, 3, 7};
                break;

            case 1:
                intervals = {0, 3, 8};
                break;

            case 2:
                intervals = {0, 5, 10};
                break;

            default:
                intervals = {0, 2, 7};
                break;
        }

        uint32_t start = bar * TICKS_PER_BAR;

        uint32_t length = std::min<uint32_t>(
            2 * TICKS_PER_BAR,
            bars * TICKS_PER_BAR - start
        );

        for (int interval : intervals) {
            chords.note(
                2,
                root + 12 + interval,
                randomInt(55, 88),
                start,
                length
            );
        }
    }

    return chords;
}

// ------------------------------------------------------------
// PERSISTENT FILE NUMBERING
// ------------------------------------------------------------

uint64_t loadNextID() {
    std::ifstream file(STATE_FILE);

    uint64_t id = 1;

    if (file >> id) {
        return std::max<uint64_t>(1, id);
    }

    // Recover the next ID from existing full-track filenames.
    const std::string prefix = "techno_";
    const std::string suffix = "_full.mid";

    std::error_code ec;

    for (const auto& entry :
         fs::directory_iterator(
             OUTPUT_DIR / "full_tracks", ec
         )) {

        if (ec || !entry.is_regular_file()) {
            continue;
        }

        std::string name = entry.path().filename().string();

        if (name.rfind(prefix, 0) != 0 ||
            name.size() <= prefix.size() + suffix.size()) {
            continue;
        }

        if (name.substr(name.size() - suffix.size()) != suffix) {
            continue;
        }

        std::string number = name.substr(
            prefix.size(),
            name.size() - prefix.size() - suffix.size()
        );

        try {
            uint64_t existing = std::stoull(number);

            if (existing < std::numeric_limits<uint64_t>::max()) {
                id = std::max(id, existing + 1);
            }
        } catch (...) {
            // Ignore filenames that don't contain a valid number.
        }
    }

    return id;
}

void saveNextID(uint64_t id) {
    std::ofstream file(STATE_FILE, std::ios::trunc);

    if (file) {
        file << id << '\n';
    }
}

// ------------------------------------------------------------
// OUTPUT DIRECTORIES
// ------------------------------------------------------------

void createDirectories() {
    fs::create_directories(OUTPUT_DIR / "drums");
    fs::create_directories(OUTPUT_DIR / "basslines");
    fs::create_directories(OUTPUT_DIR / "arpeggios");
    fs::create_directories(OUTPUT_DIR / "chords");
    fs::create_directories(OUTPUT_DIR / "full_tracks");
}

// ------------------------------------------------------------
// MAIN GENERATOR
// ------------------------------------------------------------

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    try {
        createDirectories();
    } catch (const fs::filesystem_error& error) {
        std::cerr << "Cannot create output directories: "
                  << error.what() << '\n';
        return 1;
    }

    uint64_t id = loadNextID();

    std::cout << "\n";
    std::cout << "====================================\n";
    std::cout << "   INFINITE TECHNO MIDI GENERATOR\n";
    std::cout << "====================================\n";
    std::cout << "Output: "
              << fs::absolute(OUTPUT_DIR) << '\n';
    std::cout << "Tempo: "
              << MIN_BPM << "-" << MAX_BPM << " BPM\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    while (running) {
        try {
            int bpm = randomInt(MIN_BPM, MAX_BPM);
            int bars = randomInt(MIN_BARS, MAX_BARS);
            int root = randomInt(33, 45);

            std::string prefix =
                "techno_" + std::to_string(id);

            Track drums = makeDrums(bars);
            Track bass = makeBassline(bars, root);
            Track arp = makeArpeggio(bars, root);
            Track chords = makeChords(bars, root);

            // Individual MIDI files.
            saveMIDI(
                OUTPUT_DIR / "drums" /
                    (prefix + "_drums.mid"),
                {drums},
                bpm
            );

            saveMIDI(
                OUTPUT_DIR / "basslines" /
                    (prefix + "_bass.mid"),
                {bass},
                bpm
            );

            saveMIDI(
                OUTPUT_DIR / "arpeggios" /
                    (prefix + "_arp.mid"),
                {arp},
                bpm
            );

            saveMIDI(
                OUTPUT_DIR / "chords" /
                    (prefix + "_chords.mid"),
                {chords},
                bpm
            );

            // Full multitrack MIDI arrangement.
            saveMIDI(
                OUTPUT_DIR / "full_tracks" /
                    (prefix + "_full.mid"),
                {drums, bass, arp, chords},
                bpm
            );

            ++id;
            saveNextID(id);

            std::cout
                << "[" << (id - 1) << "] "
                << bpm << " BPM | "
                << bars << " bars | "
                << "Root MIDI " << root
                << " | " << prefix
                << '\n';

        } catch (const std::exception& error) {
            std::cerr << "Generation error: "
                      << error.what() << '\n';

            std::this_thread::sleep_for(
                std::chrono::seconds(1)
            );

            continue;
        }

        // Brief pause between tracks.
        for (int i = 0;
             i < GENERATION_DELAY_MS / 50 && running;
             ++i) {

            std::this_thread::sleep_for(
                std::chrono::milliseconds(50)
            );
        }
    }

    std::cout << "\nStopping generator...\n";
    std::cout << "Generated files are saved in: "
              << fs::absolute(OUTPUT_DIR) << '\n';

    return 0;
}
