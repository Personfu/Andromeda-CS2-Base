<div align="center">

```
 █████╗ ███╗   ██╗██████╗ ██████╗  ██████╗ ███╗   ███╗███████╗██████╗  █████╗ 
██╔══██╗████╗  ██║██╔══██╗██╔══██╗██╔═══██╗████╗ ████║██╔════╝██╔══██╗██╔══██╗
███████║██╔██╗ ██║██║  ██║██████╔╝██║   ██║██╔████╔██║█████╗  ██║  ██║███████║
██╔══██║██║╚██╗██║██║  ██║██╔══██╗██║   ██║██║╚██╔╝██║██╔══╝  ██║  ██║██╔══██║
██║  ██║██║ ╚████║██████╔╝██║  ██║╚██████╔╝██║ ╚═╝ ██║███████╗██████╔╝██║  ██║
╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝╚═════╝ ╚═╝  ╚═╝
                       ◈  C O U N T E R - S T R I K E  2  ◈
                          INTERNAL BASE  ·  PERSONFU FORK
```

![Language](https://img.shields.io/badge/Language-C%2B%2B20-00b4d8?style=flat-square&logo=c%2B%2B)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078d7?style=flat-square&logo=windows)
![Game](https://img.shields.io/badge/Game-Counter--Strike%202-f4a261?style=flat-square)
![Compiler](https://img.shields.io/badge/Compiler-MSVC%202022-5c6bc0?style=flat-square&logo=visualstudio)
![Fork](https://img.shields.io/badge/Forked%20from-or75%2FAndromeda-2d6a4f?style=flat-square&logo=github)

</div>

---

## Architecture

```
Andromeda-CS2-Base/
├── AndromedaClient/              ← Core cheat client
│   ├── Features/
│   │   ├── CAimbot/              ← Smooth aim + TriggerBot  [NEW]
│   │   ├── CVisual/              ← ESP boxes, bone overlay, glow
│   │   └── CInventoryChanger/   ← Full inventory skin management
│   ├── GUI/                      ← ImGui in-game menu (Insert to open)
│   ├── Render/                   ← DirectX 11 render stack
│   └── Settings/                 ← RapidJSON config persistence
├── CS2/
│   ├── Hook/                     ← MinHook VMT detours
│   │   ├── Hook_CreateMove       ← Input tick hook (aimbot entry point)
│   │   ├── Hook_Present          ← D3D11 render hook
│   │   ├── Hook_DrawGlow         ← Glow override
│   │   └── Hook_ParseMessage     ← Protobuf parser (sound ESP example)
│   ├── SDK/                      ← CS2 game SDK (types, math, interfaces)
│   └── Protobuf/                 ← move_crc bypass via protobuf patching
├── GameClient/
│   ├── CL_Bypass                 ← SetViewAngles / SetAttack CRC-safe wrappers
│   ├── CL_Bones                  ← CS2 skeleton instance bone resolver
│   ├── CL_VisibleCheck           ← Trace-based line-of-sight check
│   └── CEntityCache              ← Thread-safe entity snapshot cache
└── Andromeda-Injector/           ← Blackbone manual-map injector
```

---

## Features

### Aimbot *(new in this fork)*
| Setting | Default | Description |
|---|---|---|
| Active | `off` | Master toggle |
| Hold Key | `Left Alt` | Aim assist only while held |
| FOV | `10°` | Target cone — enemies outside are ignored |
| Smoothing | `5.0` | 1 = instant snap · 20 = very soft interpolation |
| Aim Bone | `Head` | Head / Neck / Upper Body |
| Only Enemy | `on` | Never aim at teammates |
| Only Visible | `on` | Trace check before locking on |
| TriggerBot | `off` | Auto-fire when crosshair overlaps target |
| Trigger FOV | `2°` | How precisely on-target before firing |

> Angle writes go through **`CL_Bypass::SetViewAngles`** — the same protobuf CRC-spoof path used by the base, so subtick integrity is maintained.

---

### Visuals (ESP)
- **Player bounding boxes** — 4 styles: Box · Outline Box · Coal Box · Outline Coal Box  
- **Bone ESP** — skeleton overlay on all players  
- **Glow** — per-team colour via `Hook_DrawGlow`  
- **Visibility-aware colouring** — colour shifts when enemy is traced  
- Independent **enemy / teammate** toggles  

---

### Anti-Detection & Bypass
- **`move_crc` bypass** — all angle and attack writes are routed through the protobuf-patching path (`CL_Bypass`) so the server-side CRC check passes cleanly  
- **CS2 subtick input spoofing** — `AddSubtickMoveStep` injects attack inputs at the correct `when` value within the subtick window  
- **Blackbone manual-map injection** — bypasses standard DLL load path; injector with database in `Andromeda-Injector/`  

---

### Developer Infrastructure
- **Crash log** — on exception, dumps a crash offset you can resolve in x64dbg: load the DLL, hit `Home`, subtract `0x1000` from the base, add the crash offset  
- **Schema dumper** — set `DUMP_SCHEMA_ALL_OFFSET 1` in `Common/Include/Config.hpp` to dump all CS2 schema offsets  
- **Protobuf message parser** — `Hook_ParseMessage.cpp` shows a live sound-position ESP example  
- **JSON config** — `CSettingsJson` load/save via RapidJSON; config files drop next to the DLL  
- **`DEV_LOG`** — `DEV_LOG("msg\n")` zero-overhead debug logging  
- **FW1 + FreeType font rendering** — all fonts work in-game (see `CAndromedaClient.cpp`, `CVisual.cpp`)  

---

## Build

> **Requirements:** Visual Studio 2022 · Windows SDK · Platform toolset `v143` · x64

```
1. Open   Andromeda-CS2-Base.sln
2. Set    Configuration → Release | Platform → x64
3. Build  Ctrl+Shift+B
4. Inject Andromeda-CS2-Base.dll via the Blackbone MM injector
5. Menu   Insert key to toggle in-game menu
```

---

## Screenshots

<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/106f81d5-e24f-44af-8449-74b1ca1d94ff" />
<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/d72f589d-832c-4af3-9ba2-64ea25c98e5e" />
<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/97d0d9a4-b7b4-4447-81de-5d7ed263907b" />

---

## Links

| Resource | Link |
|---|---|
| UnknownCheats Thread | [Andromeda CS2 Internal Base](https://www.unknowncheats.me/forum/counter-strike-2-a/722929-andromeda-cs2-internal-base.html) |
| Powered by | [Andromeda Hack](https://andromeda.buzz/) |
| Upstream source | [or75/Andromeda-CS2-Base](https://github.com/or75/Andromeda-CS2-Base) |
| This fork | [Personfu/Andromeda-CS2-Base](https://github.com/Personfu/Andromeda-CS2-Base) |

