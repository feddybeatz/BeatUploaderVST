# BeatUploaderVST

A custom VST3 audio plugin built with C++ and the JUCE framework designed to streamline a beatmaker's export-to-upload pipeline directly inside FL Studio (or any VST3-compatible DAW).

Instead of manually exporting, navigating folders, and writing upload descriptions separately, **BeatUploaderVST** captures master bus audio in real time, extracts project metadata like BPM, and prepares the file for automated platform distribution.

---

## 🚀 Features

### Current
* **Real-Time DAW BPM Extraction:** Reads live transport tempo from the host DAW using `juce::AudioPlayHead` and updates the UI seamlessly across threads.
* **Direct Master Bus Audio Capture:** Records master output audio streams into a 24-bit stereo `.wav` file on disk via `juce::AudioFormatWriter`.
* **Asynchronous Save Dialog:** Uses Windows native "Save As..." file chooser (`juce::FileChooser`) running asynchronously so neither the DAW GUI nor the audio rendering thread ever freezes.
* **Thread-Safe Architecture:** Strict separation between the high-priority real-time audio thread (`PluginProcessor`) and the visual user interface thread (`PluginEditor`) using lock-free `std::atomic` variables.

### Roadmap
* [ ] **Real-Time Waveform Visualizer:** Live audio buffer visualization during capture using `juce::AudioVisualiserComponent`.
* [ ] **Musical Key Selector:** Integrated dropdown control to tag the beat's scale and key.
* [ ] **Automated YouTube Uploader:** Rendered audio-to-video upload engine targeting the YouTube API, complete with customizable title, tag, and description templates auto-populated with BPM and Key metadata.

---

## 🛠️ Tech Stack & Tools

* **Language:** C++17 / C++20
* **Framework:** [JUCE](https://juce.com/) (AudioPlugin Template)
* **IDE / Compiler:** Visual Studio Code & Microsoft Visual Studio 2026 (MSVC Compiler `cl.exe`)
* **Build System:** Projucer / Visual Studio Solution
* **Target Format:** VST3 (64-bit Windows)

---

