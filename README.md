# vefxio + eamio subscreen for iidx 12-24


## Features
- Ticker
- keypads
- effector sliders
- spice2x CardIO reader support
## todo
- spcie2x card json support
- replay recording and uploading using official XRPC apis
- scene-awareness


## project layout
```
src/
├── api.cpp - Exported API that both eamio.dll and
│       vefxio.dll use to talk to backend.dll
└── backend.cpp - Main code for spawning the window,
│       initializing dx11 context, hooking and handling touch/
└── ...
include/
├── globals.hpp - All global variables
├── stype.hpp - UI style vars
└── ...
assets/
├── assets.rc - Windows resource file
└── resource_def.h - Asset -> ID associations
libs/
├── imgui/      (https://github.com/ocornut/imgui)
├── json.hpp    (https://github.com/nlohmann/json)
└── stb_image.h (https://github.com/nothings/stb)
```
