# MacMagic

ARP-based MAC address spoofing tool for macOS. Lives in your menu bar, works without sudo.

## How It Works

MacMagic uses **ARP reply injection** via libpcap to make the local network see a different MAC address for your machine — without actually changing the hardware MAC. This means:

- No `sudo` required (just BPF access via `access_bpf` group)
- Original MAC is never modified on the interface
- Corrective ARP is sent when spoofing stops to restore the gateway's cache
- Optional DHCP client acquires a new IP using the spoofed MAC

## Features

- Per-interface MAC spoofing (Wi-Fi, Ethernet, etc.)
- Random or custom MAC address
- DHCP support — get a new IP with your spoofed MAC
- Wi-Fi details: SSID, band, channel, signal strength, TX rate
- Network info: IP, gateway, current/hardware MAC per interface
- Native macOS menu bar app (no Dock icon)
- Programmatic icons with automatic dark/light mode support
- macOS 13.0+ (Apple Silicon) / macOS 12.0+ (Intel)

## Screenshots

The app runs entirely from the menu bar. Right-click or click the shield icon to access all features.

## Build

Requires Qt 6, CMake 3.21+, and libpcap (ships with macOS SDK).

```bash
# Install Qt via Homebrew
brew install qt

# Build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
make -j$(sysctl -n hw.ncpu)

# Run
open MacMagic.app
```

## BPF Access

ARP injection requires read/write access to `/dev/bpf*` devices. On macOS, add your user to the `access_bpf` group:

```bash
sudo dseditgroup -o edit -a $(whoami) -t user access_bpf
```

If you have Wireshark installed, this group is usually already configured.

## CI/CD

Every push triggers a GitHub Actions build that produces DMGs for both architectures:

- `MacMagic-arm64.dmg` — Apple Silicon (macOS 13.0+)
- `MacMagic-x86_64.dmg` — Intel (macOS 12.0+)

Pushes to `main` automatically create a GitHub Release with commit history as release notes.

## Author

- GitHub: [github.com/uk0](https://github.com/uk0)
- Blog: [firsh.me](https://firsh.me)

## License

MIT
