# Video Merger Pro 🎬

An intuitive, high-performance desktop application for Windows built with **C++20** and **Qt 6 (Widgets)** that merges multiple video clips into a single file. 

The application dynamically analyzes incoming media streams: if codecs and formats match, it merges clips instantly via lossless **stream-copying** without re-encoding; if formats or resolutions vary, it standardizes and **transcodes** them into an optimized MP4.

---

## Features

- **Drag-and-Drop Workflow:** Drop individual video files or entire directory trees directly into the queue.
- **Visual Reordering:** Reorder your timeline effortlessly using drag-and-drop within the source list.
- **Embedded Media Preview:** Real-time playback scrubbing powered by Qt Multimedia to inspect clips before merging.
- **Smart Concat Engine:**
  - **Zero Loss / Ultra Fast:** Performs instant stream copy concatenation when video/audio codecs and containers match.
  - **Dynamic Fallback:** Automatically rescales, pads, and transcodes mixed resolutions or incompatible codecs to standardized 1080p MP4.
- **Automated FFmpeg Bootstrapping:** Automatically detects local or system FFmpeg binaries. If missing, offers to safely download and extract an official Windows build to local app data.
- **Safety First:** Prevents accidental overwrites with confirmation dialogs and handles file paths with special characters and spaces safely.
- **Adaptive UI:** Clean modern design styled to render cleanly across both Windows Light and Dark themes.

---

## Supported Formats

Imports all common container formats:
`.mp4`, `.mkv`, `.avi`, `.mov`, `.ts`, `.flv`, `.webm`, `.wmv`, `.m4v`

Outputs standardized, universally compatible `.mp4` video files.

---

## Prerequisites

To build from source on Windows 10/11:

- **Visual Studio 2022** (or newer) with the **Desktop development with C++** workload.
- **Qt 6.5+** (Widgets, Multimedia, MultimediaWidgets, Network modules installed).
- **CMake 3.22+**.
- **Ninja** (included by default with Visual Studio).

---

## Building from Source

1. Clone the repository:
```cmd
   git clone https://github.com/ripsnortntear/video-merger-pro.git
   cd video-merger-pro
```
2. Open the x64 Native Tools Command Prompt for Visual Studio.

3. Configure the project with CMake (pointing to your Qt MSVC directory):
```cmd
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\<YOUR_QT_VERSION>\msvc2022_64"
```
4. Build the executable:
```cmd
  cmake --build build
```
5: Deploy Qt runtime dependencies:
```cmd
cd build
C:\Qt\<YOUR_QT_VERSION>\msvc2022_64\bin\windeployqt.exe --release --multimedia --network VideoMerger.exe
```

6. Run the application:
```cmd
VideoMerger.exe
```

## Creating the Windows Installer

This project includes an Inno Setup script (installer.iss) to produce a single-file .exe installer.

Install Inno Setup 6.

Stage your deployed VideoMerger.exe and Qt dependencies inside C:\VideoMergerApp\.

Compile installer.iss:

```cmd
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer.iss
```
The output setup package will be generated inside C:\Output\.

Technology Stack
Language: C++20
GUI Framework: Qt 6 (Widgets, Multimedia)
Media Engine: FFmpeg & FFprobe
Build System: CMake & Ninja
Packaging: Inno Setup 6

License:
This project is licensed under the GNU General Public License v3.0 (GPLv3). See the LICENSE file for details.
