# Waddle
![Logo](./assets/logo.png)

![Version](https://img.shields.io/badge/version-1.1.0-808080?style=flat-square)
![License](https://img.shields.io/badge/license-GPL%20v3-808080?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Windows-808080?style=flat-square)
![Language](https://img.shields.io/badge/language-C%2B%2B-808080?style=flat-square)
![Downloads](https://img.shields.io/github/downloads/mxncmrchnd/waddle/total?style=flat-square&color=808080)
![Repo Size](https://img.shields.io/github/repo-size/mxncmrchnd/waddle?style=flat-square&color=808080)

Made by Maxence Marchand, 2026

## What is Waddle ?

Waddle is a small VST3 plugin for sidechaining. Drop it into a mixer track, tweak the parameters to your liking, and enjoy the magic.

## How to unstall it ?

Just download the [ZIP file](https://github.com/mxncmrchnd/waddle/releases/download/v1.1.0/Waddle.vst3.zip) and unzip it to the destination of your choice. Then make your DAW point towards that folder.

## What are the requirements ?
 
- Windows 10 or later
- A VST3-compatible DAW (FL Studio, Ableton, Reaper, etc.)

## How does it work ?

![Screenshot](./screenshot_2.png)

**Wave type** : the type of envelope (exponential, linear, logarithmic or sine)

**Depth** : how much the volume ducks

**Curve** : how fast the volume gets back to its original level

**Rate** : 1/1, 1/2, 1/4 or 1/8, the frequency of the sidechain (default : 1/4)

## How to build it by source ?
 
### What's needed ?
 
- [JUCE 8](https://juce.com)
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools) with Desktop development with C++
- CMake 3.22+
 
### How to build ?
 
```bash
git clone https://github.com/YOUR_USERNAME/waddle.git
cd waddle
cmake -B build -DJUCE_DIR=path/to/JUCE
cmake --build build --config Release
```
 
The built plugin will be at `build/Waddle_artefacts/Release/Waddle.vst3`.

## License

Waddle is licensed under the [GPL v3](LICENSE) license, in compliance with the JUCE open source license.
