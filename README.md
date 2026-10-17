<div align="center">

<img src="icon.jpg" alt="stonestory_nx" width="160">

# stonestory_nx

**Stone Story RPG on Nintendo Switch**

An unofficial Nintendo Switch port of the Android version of  
**Stone Story RPG — 4.33.0**

**By xFlippy**

[![Switch](https://img.shields.io/badge/Nintendo_Switch-Homebrew-E60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)](#)
[![Ko-fi](https://img.shields.io/badge/Support_on_Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/flippyy)

</div>

---

## About

`stonestory_nx` brings **Stone Story RPG Android 4.33.0 (ARM64, version code 355)** to Nintendo Switch as an experimental homebrew port.

The included compatibility layer runs the Android package under Horizon OS and starts the game directly. 

> Bring your own legally obtained XAPK. This repository and the NRO downloads contain no game package, game libraries or gameplay assets. The homebrew icon comes from the Android game.

The port uses offline startup. Local progress uses the port's own save directory.

---

## Controls

Touchscreen input and a controller-operated cursor are supported.

### During a Run

| Button | Action |
|---|---|
| **MINUS** | Toggle the visible cursor |
| **Left stick** | Move the cursor while cursor mode is enabled |
| **A** | Click at the cursor; hold to drag |
| **Touchscreen** | Use the game's on-screen buttons directly |
| **B** | Android back |
| **PLUS** | Exit to hbmenu |

---

## Build

### Requirements

- [devkitPro](https://devkitpro.org/), devkitA64 and libnx
- Switch portlibs: Mesa/FreeType headers, libdrm_nouveau, libvorbis/libogg, curl, Mbed TLS, zlib and liblzma
- Linux or WSL with GNU Make, Bash, Python 3.11+, curl, tar, patch and Ninja
- Python's `packaging` and `MarkupSafe` modules; bison, flex and m4, or Ubuntu/WSL's `apt-get` to fetch these generators locally
- A host C compiler plus zlib/liblzma development headers for the importer and host checks (`zlib1g-dev liblzma-dev` on Ubuntu)
- Your own **Stone Story RPG Android 4.33.0 ARM64 XAPK** for playing

### Compile

Enter the checkout:

```bash
cd stonestory_nx
```

Set the SDK paths:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PATH
```

Compile the port:

```bash
make -j$(nproc)
```

For a clean rebuild:

```bash
make clean
make -j$(nproc)
```

The result is **`stonestory_nx.nro`**.

---

## Running

Arrange the SD card as follows:

```text
sd:/switch/stonestory_nx/
├── stonestory_nx.nro
└── game_data/
    └── stone_story.xapk
```

Name your original archive **`stone_story.xapk`** and put it inside **`game_data/`**. Leave the base APK and ARM64/resource splits inside the XAPK intact.

You can validate and import the supported archive on your computer:

```bash
python3 tools/import_apk.py /path/to/StoneStory.xapk
```

This creates `game_data/stone_story.xapk`. The importer checks the package and supported archive hash; unknown builds are rejected. Copy the generated `game_data/` folder alongside the NRO on the SD card.

**Launch using application/title takeover.** Hold **R** when starting an installed Switch title to open hbmenu, then select **Stone Story RPG**. Use this mode instead of the Album applet.

The port creates its `saves/`, `cache/`, `config/` and `logs/` directories on demand. Progress, preferences and databases live under `saves/com.MartianRexInc.StoneStory/`. Back up `saves/` before updates; update the NRO while keeping your game archive and saves. Logs are written to `logs/stonestory_nx.log`.

---

## Support

Support the port's development on Ko-fi:

<div align="center">

[![Support me on Ko-fi](https://img.shields.io/badge/Support_me_on_Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/flippyy)

</div>

---

## Disclaimer

This fan-made port is independent of [**Martian Rex, Inc.**](https://stonestoryrpg.com/) and is not an official Stone Story RPG release.

The Stone Story RPG name, artwork, sound and game content remain the property of their owners. **xFlippy** is the port maintainer shown in the homebrew menu. Third-party notices and source distribution status are recorded in [LICENSES.md](LICENSES.md).

---

## Contribution

Issues and pull requests with reproducible fixes are welcome. Reports should identify the game version and launch mode, describe what happened and include the relevant log excerpt. Keep game archives, native game libraries, private saves and credentials out of public reports.
