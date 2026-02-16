# NCTV Player (C++ / Qt6 / libVLC)

Native C++17 digital signage player for Raspberry Pi, replacing the legacy Electron/Node.js architecture. Built with Qt6 Quick (QML) for the UI and libVLC for hardware-accelerated 4K HEVC video playback.

## Architecture

| Layer | Technology | Purpose |
|-------|-----------|---------|
| **UI** | Qt Quick / QML | Fullscreen borderless window with 4 transparent zones |
| **Video** | libVLC (C API) | Hardware-accelerated HEVC decode, direct overlay rendering |
| **Logic** | C++17 | Playlist scanning, config management, process control |
| **Transcoding** | HandBrakeCLI | Background H.265 optimization via QProcess |
| **Packaging** | dpkg-deb + Aptly | Debian package distribution to Pi fleet |
| **Watchdog** | systemd | Crash recovery with `Restart=always` |

## Zone Layout (1920×1080)

```
┌──────────────────────────────────────────────────────┐
│                    (21px top bar)                     │
├────────────────────────────┬─────────────────────────┤
│                            │                         │
│      Main Zone             │    Vertical Zone        │
│      0,21 → 1472×828      │    1472,21 → 448×849    │
│                            │                         │
│                            │                         │
├────────────────────────────┴─────────────────────────┤
│              Horizontal Zone                         │
│              0,870 → 1920×189                        │
└──────────────────────────────────────────────────────┘
       Background Zone: 0,0 → 1920×1080 (z:0)
```

## Prerequisites

### Windows (Development)
- **CMake** 3.20+
- **Qt6** (Core, Gui, Quick, QuickControls2, Multimedia, Widgets)
- **VLC SDK** — download from videolan.org, extract into `external/vlc-sdk-windows/`
- **Visual Studio 2022** Build Tools (MSVC)
- **HandBrakeCLI** (optional, for video optimization testing)

### Raspberry Pi (Production)
- **Raspberry Pi OS** (Debian Bullseye or newer)
- `sudo apt install cmake qt6-base-dev qt6-declarative-dev qt6-multimedia-dev libvlc-dev handbrake-cli`

## Building

### Windows
```bat
scripts\build_windows.bat
```

### Raspberry Pi (native build + .deb packaging)
```bash
chmod +x scripts/*.sh
./scripts/build_pi_deb.sh
```

### Install on Pi
```bash
sudo dpkg -i nctv-player_1.0.1_armhf.deb
```

## Configuration

Config file location: `/etc/nctv-player/config.ini` (Pi) or `./config.ini` (dev)

```ini
[General]
kioskMode=true
retryIntervalMs=5000
imageDurationMs=10000
audioEnabled=false

[Paths]
playlistRoot=/var/lib/nctv-player/playlist
logPath=/var/log/nctv-player.log

[Display]
targetWidth=1920
targetHeight=1080

[Optimization]
optimizedSuffix=_optimized
```

## Playlist Directory Structure

```
/var/lib/nctv-player/playlist/
├── playlist-background/    # Full-screen background media
├── playlist-main/          # Primary content zone
├── playlist-horizontal/    # Bottom ticker/banner
└── playlist-vertical/      # Right sidebar content
```

Place `.mp4`, `.mkv`, `.jpg`, `.png`, etc. files in the appropriate zone folder. The player auto-scans on startup and loops continuously.

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Escape` | Quit (or close menu) |
| `F11` | Toggle debug overlay |
| `F12` | Toggle settings menu |

## systemd Service

The player runs as a systemd service with automatic crash recovery:
```bash
sudo systemctl status nctv-player
sudo systemctl restart nctv-player
sudo journalctl -u nctv-player -f
```

## License

Proprietary — NCompass TV
