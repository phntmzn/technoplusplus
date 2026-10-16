# Infinite Techno MIDI Generator

A C++17 procedural music generator that continuously creates randomized techno MIDI patterns and multitrack arrangements. Generate drum patterns, basslines, arpeggios, and chords, then import the MIDI files into your favorite digital audio workstation (DAW).

Built for macOS with a standard C++ compiler and no external MIDI libraries.

## Features

- **Infinite generation** — continuously generate new MIDI files until stopped.
- **Techno drums** — four-on-the-floor kicks, snares, claps, closed hi-hats, and open hi-hats.
- **Basslines** — randomized bass patterns using minor-scale notes.
- **Arpeggios** — randomized melodic sequences with different rhythms and octave variations.
- **Chords** — generated chord patterns for layering and harmonic variation.
- **Multitrack arrangements** — combine drums, basslines, arpeggios, and chords into one MIDI file.
- **Tempo variation** — randomized tempos between 125 and 145 BPM.
- **Variable arrangements** — generate patterns ranging from 8 to 32 bars.
- **Persistent numbering** — preserve the next file number across normal restarts.
- **Organized output** — save each musical component in its own folder.
- **Standard MIDI Files** — generate `.mid` files compatible with popular music production software.
- **Lightweight** — uses the C++ standard library without external MIDI dependencies.

## Requirements

- macOS
- Apple Clang or another compatible C++17 compiler
- Terminal
- A MIDI-compatible DAW or MIDI player to use the generated files musically

The project uses C++17 filesystem support and standard library facilities.

## Project Structure

```text
infinite-techno/
├── CMakeLists.txt
├── README.md
├── src/
│   └── main.cpp
└── output/
    ├── .next_id
    ├── drums/
    ├── basslines/
    ├── arpeggios/
    ├── chords/
    └── full_tracks/
```

The output directories are created automatically when the program runs.

## Installation

### 1. Install the macOS Command Line Tools

If you do not already have a C++ compiler installed, run:

```bash
xcode-select --install
```

Verify the compiler installation:

```bash
clang++ --version
```

### 2. Create the project

Create the project directories:

```bash
mkdir -p infinite-techno/src
cd infinite-techno
```

Save the C++ generator as:

```text
src/main.cpp
```

### 3. Compile

Run the following command from the project root:

```bash
clang++ -std=c++17 -O2 src/main.cpp -o infinite_techno
```

This compiles the generator into an executable named `infinite_techno`.

No external MIDI libraries are required.

## Usage

Start the generator:

```bash
./infinite_techno
```

The program will continuously generate MIDI files and print information about each arrangement.

Example output:

```text
====================================
   INFINITE TECHNO MIDI GENERATOR
====================================
Output: /path/to/infinite-techno/output
Tempo: 125-145 BPM
Press Ctrl+C to stop.

[1] 132 BPM | 16 bars | Root MIDI 40 | techno_1
[2] 140 BPM | 24 bars | Root MIDI 36 | techno_2
[3] 127 BPM | 12 bars | Root MIDI 43 | techno_3
```

The example output is illustrative; actual tempos, lengths, and notes will vary.

To stop generation, press:

```text
Ctrl+C
```

Generated files remain in the output directories.

## Generated MIDI Files

Each generation produces five MIDI files.

| Folder | Filename pattern | Contents |
|---|---|---|
| `output/drums/` | `techno_N_drums.mid` | Techno percussion |
| `output/basslines/` | `techno_N_bass.mid` | Bassline patterns |
| `output/arpeggios/` | `techno_N_arp.mid` | Melodic arpeggios |
| `output/chords/` | `techno_N_chords.mid` | Chord patterns |
| `output/full_tracks/` | `techno_N_full.mid` | Combined multitrack arrangement |

Here, `N` is the generated arrangement number.

For example:

```text
output/
├── drums/
│   └── techno_1_drums.mid
├── basslines/
│   └── techno_1_bass.mid
├── arpeggios/
│   └── techno_1_arp.mid
├── chords/
│   └── techno_1_chords.mid
└── full_tracks/
    └── techno_1_full.mid
```

## Musical Generation

### Tempo

Each arrangement receives a randomized tempo between 125 and 145 BPM.

### Drums

The drum generator uses General MIDI percussion note numbers.

| Instrument | MIDI note |
|---|---:|
| Bass drum / kick | 36 |
| Snare | 38 |
| Hand clap | 39 |
| Closed hi-hat | 42 |
| Open hi-hat | 46 |

Percussion events are placed on MIDI channel 10, represented internally as channel index 9.

The drum generator includes four-on-the-floor kicks, snare hits, hi-hat subdivisions, and randomized percussion variations.

### Basslines

Basslines are generated from a natural minor scale. Rhythmic patterns vary through randomized note selection, rests, note lengths, and occasional octave changes.

### Arpeggios

Arpeggios use minor-scale notes and randomized rhythmic divisions. Melodic variations include ascending sequences, changing note degrees, and occasional octave jumps.

### Chords

Chord patterns use different three-note voicings and change approximately every two bars.

### Full Arrangements

The `full_tracks` directory contains multitrack Standard MIDI Files with separate tracks for:

1. Drums
2. Bassline
3. Arpeggio
4. Chords

The tempo is stored as MIDI tempo metadata.

**Note:** This is procedural MIDI generation, not audio synthesis. The files contain musical events and require virtual instruments, synthesizers, or drum samples to produce sound.

## Using the MIDI Files in a DAW

Import the generated MIDI files into a MIDI-compatible application, such as:

- Ableton Live
- Logic Pro
- FL Studio
- GarageBand
- REAPER

For the full arrangement, open a project in your DAW and import a file from `output/full_tracks/`.

Assign suitable instruments to the tracks:

| Track | Suggested instrument |
|---|---|
| Drums | Drum rack or drum sampler |
| Bassline | Monophonic bass synthesizer |
| Arpeggio | Pluck or sequenced synthesizer |
| Chords | Synth pad or chord synthesizer |

For individual MIDI files, import the corresponding file from its dedicated folder.

Drum note mappings can vary between drum instruments, so you may need to adjust the mapping in your DAW.

## Configuration

The main configuration options are defined near the top of `src/main.cpp`.

```cpp
constexpr int PPQ = 480;

constexpr int MIN_BPM = 125;
constexpr int MAX_BPM = 145;

constexpr int MIN_BARS = 8;
constexpr int MAX_BARS = 32;

constexpr int GENERATION_DELAY_MS = 250;
```

| Setting | Description |
|---|---|
| `PPQ` | MIDI ticks per quarter note |
| `MIN_BPM` | Minimum generated tempo |
| `MAX_BPM` | Maximum generated tempo |
| `MIN_BARS` | Minimum arrangement length |
| `MAX_BARS` | Maximum arrangement length |
| `GENERATION_DELAY_MS` | Approximate delay between generations |

Change these values and recompile to customize the generator.

For example, to generate faster techno:

```cpp
constexpr int MIN_BPM = 135;
constexpr int MAX_BPM = 155;
```

To generate longer arrangements:

```cpp
constexpr int MIN_BARS = 32;
constexpr int MAX_BARS = 64;
```

Longer arrangements produce larger MIDI files and take slightly longer to generate.

## Persistent Numbering

The generator stores its next arrangement number in:

```text
output/.next_id
```

This prevents normal restarts from reusing arrangement numbers.

Keep this file if you want numbering to continue across runs. Deleting it may cause numbering to restart or be recovered from existing full-track filenames.

Avoid running multiple instances simultaneously in the same output directory, because they can conflict over numbering and output files.

## Troubleshooting

### Compiler not found

If Terminal cannot find `clang++`, install the macOS Command Line Tools:

```bash
xcode-select --install
```

### Permission denied

Make sure you are running the executable from a directory where you have write permissions.

If necessary, compile again:

```bash
clang++ -std=c++17 -O2 src/main.cpp -o infinite_techno
```

### MIDI files are silent

MIDI files do not contain audio. Open them in a DAW and assign virtual instruments or drum samples.

### Drum sounds are incorrect

Check that the drum instrument uses General MIDI percussion mappings. Some drum racks use different note assignments.

### Too many files

The generator is designed to run indefinitely. It does not automatically delete older files.

Stop the program with `Ctrl+C` and remove unwanted output files manually.

Monitor available disk space if you leave the generator running for extended periods.

## Limitations

- Generated patterns are randomized and may not always sound musically coherent.
- The generator uses a limited set of scales and chord structures.
- The arrangements do not include automated mixing, audio rendering, or mastering.
- MIDI note velocity is randomized, but MIDI does not specify the resulting instrument sound.
- The program generates MIDI rather than WAV, MP3, or other audio formats.
- Infinite generation requires sufficient disk space for the growing collection of files.

## Future Improvements

Potential extensions include:

- Acid techno basslines and 303-style sequences
- Euclidean and polyrhythmic drum generation
- More scales, keys, and chord progressions
- Evolving 64- or 128-bar arrangements
- Breakdowns, builds, fills, and transitions
- Swing and groove controls
- MIDI humanization and velocity shaping
- Configurable random seeds
- Command-line options for tempo, duration, and output location
- Automatic rendering to WAV using external synthesizers
- More advanced melodic and harmonic constraints

## License

Choose a license before distributing the project publicly. For an open-source project, the MIT License is one option.

## Author

Created with C++17 for procedural techno MIDI generation.
```

### One additional file

Your README includes a project structure with `CMakeLists.txt`. If you want to support CMake builds as well as the direct compiler command, create this file in the project root:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(InfiniteTechno LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(infinite_techno src/main.cpp)

if(APPLE)
    target_compile_options(infinite_techno PRIVATE -O2)
endif()
```

You can then compile using either the direct `clang++` command or CMake.
