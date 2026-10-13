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
