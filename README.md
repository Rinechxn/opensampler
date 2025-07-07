# 🎧 OpenSampler

> Modular Sampler Engine for Desktop, Embedded, and Plugin Platforms  
> Designed for performance, flexibility, and full offline + embedded use.

---

## 📦 Overview

**OpenSampler** is a multi-platform open-source sampler engine that supports:

- 🖥️ **Desktop CLI & Standalone (Qt6 QML)**
- 🎛️ **Plugin (JUCE-based VST3 / CLAP)**
- 📱 **Embedded (Zephyr RTOS + LVGL UI)**
- 🧠 **Custom Player Engine (C/C++/Rust)**
- 📂 **Custom .osmp Format** with:
  - Embedded samples
  - YAML mapping & metadata
  - Zlib compression
  - XOR encryption

---

## 🗂️ Repository Structure

```bash
.
├── software/              # Applications (CLI, Plugin, Standalone)
│   ├── cli/               # Headless CLI player
│   ├── plugins/           # JUCE-based VST3 / CLAP plugins
│   ├── standalone/        # Qt6 QML GUI
│   └── embeded/           # Zephyr RTOS + LVGL firmware
├── osmpsdk/               # Tooling SDK (.osmp builder, converter, etc.)
│   ├── buildtools/        # C-based builder (ELF or Windows)
│   ├── converter/         # SFZ2YAML and related converters
│   ├── initproject/       # Project scaffolding tool
│   ├── player/            # Embedded runtime
│   └── MakeFile
├── libosmp/               # Core playback engine (C/C++/Rust bindings)
│   ├── core/
│   ├── engine/
│   └── osmp.h
├── external/              # Third-party libraries (e.g. JUCE, miniaudio, etc.)
└── pythonscript/          # Python toolchain (YAML builders, converters)
````

---

## ⚙️ Build Tools

| Tool           | Description                             |
| -------------- | --------------------------------------- |
| `osmpbuild`    | Build `.osmp` files from YAML+Samples   |
| `sfz2yaml.py`  | Convert SFZ → YAML format               |
| `buildfile.py` | Python builder (compressed + encrypted) |

### 🔐 `.osmp` Format

* Header: `OSMP` magic + version
* YAML Metadata (UTF-8)
* Global Settings: `volume`, `amp_veltrack`
* Group + Region data (key range, velocity, etc.)
* Embedded Sample (Zlib + XOR encryption)

---

## 🎛️ Plugins & GUI

| Component     | Framework    | Status         |
| ------------- | ------------ | -------------- |
| CLI Player    | C++17 / ALSA | ✅ Ready        |
| Qt Standalone | Qt6 + QML    | 🔧 In Progress |
| JUCE Plugin   | JUCE 7.x     | 🧪 Prototyping |
| Zephyr UI     | LVGL + RTOS  | 🧪 Testing     |

---

## 🧪 Sample Project

```bash
# Build CLI
cd software/cli
make

# Run Player
./opensampler_cli path/to/project.osmp

# Convert SFZ to YAML
python3 pythonscript/sfz2yaml.py input.sfz > mapping.yaml
```

---

## 💻 Dependencies

* `zlib` (compression)
* `yaml-cpp` (optional YAML C++ binding)
* `JUCE` (plugin & GUI)
* `Qt6` (standalone GUI)
* `LVGL` + `Zephyr SDK` (embedded)

---

## 🧠 Philosophy

OpenSampler is built to be:

* **Modular** – Each layer can be used independently (lib, CLI, plugin, embedded)
* **Minimal** – Custom format, small footprint, no dependency hell
* **Cross-platform** – Win / Linux / macOS / Embedded RTOS
* **Producer-friendly** – Simple mappings, hardware sampler-style control

---

## 🛠️ Todo

* [x] .osmp builder (Python + C)
* [x] Zlib + XOR sample embedding
* [ ] Qt GUI Standalone
* [ ] VST3 / CLAP JUCE Plugin
* [ ] Multi-region velocity switching
* [ ] MIDI Live Input Support

---

## 📃 License

MIT License
(C) 2024–2025 Ariz Kamizuki

---

## 🧙‍♂️ Author

**Ariz Kamizuki**
Music Producer, Developer of VeloOS + SphereOS
Project by [Rinechxn](https://github.com/Rinechxn)

---

## 💬 Contact / Discussion

* GitHub Issues
* Discord (soon)
* Forum: [opensampler.io/forum](#)

---

🎵 *Built for producers, hackers, and hardware dreamers.*
