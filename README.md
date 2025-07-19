# vefxio + eamio subscreen for iidx 12-24




## project layout
```
/
„¥„Ÿ„Ÿ src/
„    „¥„Ÿ„Ÿ api.cpp - Exported API that both eamio.dll and
„    „        vefxio.dll use to talk to backend.dll
„    „¥„Ÿ„Ÿ backend.cpp - Main code for spawning the window
„    „        initializing dx11 context, hooking and handling touch
„    „¤„Ÿ„Ÿ ...
„¥„Ÿ„Ÿ include/
„    „¥„Ÿ„Ÿ globals.hpp - All global variables
„    „¥„Ÿ„Ÿ stype.hpp - UI style vars
„    „¤„Ÿ„Ÿ ...
„¥„Ÿ„Ÿ assets/
„    „¥„Ÿ„Ÿ assets.rc - Windows resource file
„    „¤„Ÿ„Ÿ resource_def.h - Asset -> ID associations
„¤„Ÿ„Ÿ libs/
    „¥„Ÿ„Ÿ imgui/      (https://github.com/ocornut/imgui)
    „¥„Ÿ„Ÿ json.hpp    (https://github.com/nlohmann/json)
    „¤„Ÿ„Ÿ stb_image.h (https://github.com/nothings/stb)
```