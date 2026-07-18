# About Future of This Project

- Currently this project supports only a spesific file type (.mp4) and a spesific resolution (1080p to pixelated 1080p).  
- I will recreate this project with c++. It will support other file types and resolution automatically.  
- Not only video will be pixelated but also I will work on audio(like 8-bit retro sound maybe), currently audio output is literally copy of the original video.  
- Also there will be an UI and ready packages for three main linux distro (debian, fedora, arch) and also win11 (linux packages are priory goal).  
- The first stage of the roadmap includes only video but after than audio and image stage will be succeed.
- This project will never became a video editor projects. I aim mostly creating a converter. Also this project is a lesson for me to learning details about video decoder-encoder structure.

### C version moved another branch. Now, main branch is C++.

# C Video Pixelator (with FFmpeg API)

This project converts high-resolution videos ($1920\times 1080$) into sharp, retro pixel art style ($32 \times 18$) in real-time using C language and the **FFmpeg API**.

Vide synchronization (PTS) and the original audio stream (with zero loss using the **Stream Copy** method) are preserved during the process. The converted video is re-enlarrged to its original resolution, preserving sharp blocks so that it can ve viewed directly on modern players.

## Features
- Non-blurred, sharp retro pixel effect with **Nearest Neighbor (SWS_POINT)** sampling.
- **Dynamic Audio Copying:** The audio stream is transferred directly to the output container without decoding (without re-encoding).
- **Multi-threading Support:** During the encoding phase, `libx264` automatically uses all logical cores of the processor at 100% load to provide high-speed rendering.

---

## Installing Dependencies

If you have a execution file just ffmpeg is enough for all distros but if you don't have an execution file, these dependencies are necessary to build.

### Arch Linux / CachyOS
```bash
sudo pacman -Syu base-devel ffmpeg
```

### Fedora 

To install the build tools, the `pkg-config` tool that manages nested dependencies, and the full library set that will enable the `libx264` encoder by bypassing H.264 license restrictions on Fedora, run the following steps sequentially in the terminal:


#### 1. Activate the free and non-free repositories of RPM Fusion.
```bash
sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
```

#### 2. Install the build tools (gcc, make) and the pkg-config package.
```bash
sudo dnf group install "Development Tools"
sudo dnf install pkgconf-pkg-config
```

#### 3. Replace Fedora's restricted default FFmpeg package with the full version in RPM Fusion.
```bash
sudo dnf swap ffmpeg-free ffmpeg --allowerasing
```

#### 4. Install the developer headers and the libx264 engine.
```bash
sudo dnf install ffmpeg-devel x264 x264-libs libavcodec-freeworld
```

### Debian and Ubuntu (debian based)
```bash
sudo apt update && sudo apt install -y libavformat-dev libavcodec-dev libswscale-dev libavutil-dev pkg-config cmake build-essential
sudo apt update && sudo apt install ffmpeg
```

# If you want to build manually:
## Standard Build
```bash
gcc -o pixelator main.c -lavformat -lavcodec -lswscale -lavutil
```
## With Include Path
```bash
gcc -o pixelator main.c -I /usr/include/ffmpeg -lavformat -lavcodec -lswscale -lavutil
```

# How to use:
```bash
./pixelator <input_video_path> <output_video_path>
```

# Technical Analysis
A 3.5-minute 1080p video (tried: Never Gonna Give You Up) is fully processed in an average of ~60-65 seconds on modern multi-core processors like the Ryzen 7 7735HS, thanks to libx264's multi-threading optimization. The process is entirely CPU-bound and does not encounter disk I/O bottlenecks.

# AI TOOLS WERE USED IN THIS CODE (For "C" version)