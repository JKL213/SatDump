# SatDump Project Knowledge & Guidelines

## Overview
SatDump is a generic satellite data processing software written in C++17, built with CMake, VOLK, FFTW3, OpenMP/OpenCL, and Dear ImGui.

## Architecture & Directory Structure
- **`src-core/` (`satdump_core` shared library)**:
  - `dsp/` & `common/dsp/`: Digital Signal Processing blocks (demodulators, PLLs, filters, resamplers, IQ correction, FFT, SDR sources, taps).
  - `pipeline/`: Pipeline definitions, pipeline runner, and built-in processing modules (`modules/demod/`, `modules/ccsds/`, etc.).
  - `products/`, `image/`, `projection/`: Satellite data product handling (`.cbor`), image processing, and map projections.
  - `explorer/` & `handlers/`: Interactive data viewer/explorer system for visualizing products and telemetry in the GUI.
  - `common/widgets/`: Reusable ImGui widgets (e.g., `fft_plot.cpp`, constellation plots).
  - `libs/`: Vendored third-party libraries (AngelScript, predict, libcorrect, openjp2, supernovas, calceph, etc.).
- **`src-interface/` (`satdump_interface` shared library)**:
  - Shared UI and application state management between desktop/Android builds.
  - `recorder/`: Live SDR processing, waterfall/FFT display, VFO management (`recorder_vfo.cpp`), live pipeline execution (`recorder_proc.cpp`), and pipeline tuning (`pipeline_tuning.h`).
  - `offline.cpp` & `settings.cpp`: Offline pipeline processing UI and global settings.
- **`src-ui/` (`satdump-ui` executable)**:
  - Desktop GUI frontend (GLFW + OpenGL + Dear ImGui + nativefiledialog).
- **`src-cli/` (`satdump` executable)**:
  - Command-line interface for offline pipeline execution, live recording, and SDR probing.
- **`plugins/`**:
  - Dynamically loaded shared libraries (`.so` / `.dll`) registered via `satdump::eventBus`.
  - Includes satellite mission plugins (e.g., `noaa_metop_support`, `meteor_support`, `elektro_arktika_support`, `orbcomm_support`, `transit_support`), `sdr_sources/` (e.g., `rtlsdr_sdr_support`), `audio_sinks/`, and utility apps.
- **`resources/`**:
  - `pipelines/*.json`: Pipeline step definitions, input levels (`baseband`, `soft`, `cadu`, `frames`), and default module parameters.
  - `instrument_cfgs/*.json`: Instrument configurations.
  - LUTs, projections, shaders, and TLE configurations.

## Build & Test Workflows
- **Build Directory**: `build/` (already configured with `compile_commands.json` exported to root).
  - Run `satdump` or `satdump-ui` from `build/` (where `resources` and `satdump_cfg.json` are expected or symlinked).
  - Build specific targets via: `cmake --build build --target <target_name> -j4`
- **Experimental TRANSIT 5E-5 Plugin (`plugins/transit_support/`)**:
  - Pipeline ID: `transit_5e5_experimental` (`resources/pipelines/Transit.json`).
  - Build with tests: `cmake -S . -B build -DPLUGIN_TRANSIT=ON -DTRANSIT_BUILD_TESTS=ON`
  - Test targets: `transit_protocol_test`, `transit_product_test`, `transit_live_test`, `transit_pam_test`.
- **Recorder Frontend Regression Test (`tests/recorder_frontend_test.cpp`)**:
  - Tests native RTL-SDR offset tuning (E4000), marker frequency with positive/negative frequency shifts, and double-precision shared DC correction (`correct_iq.cpp`).
  - See `tests/recorder_frontend_test.md` for standalone compilation and execution command against `build/libsatdump_core.so`.

## Key Technical Conventions
- **Frequency Shift Convention**: In SatDump pipelines/demodulators, `frequency_shift` is additive (recording center frequency minus target satellite frequency).
- **Formatting**: `.clang-format` is present in the root directory (`BasedOnStyle: LLVM`, `IndentWidth: 4`, `ColumnLimit: 150`).
