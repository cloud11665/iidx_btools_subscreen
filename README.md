# [WIP] vefxio + eamio subscreen for iidx 12-24

## Installation
- place `backend.dll`, `vefxio.dll`, `eamio.dll` in the same folder as your `gamestart-XX.bat`
- for styles that ship with `bm2dx.dll` edit `gamestart-XX.bat` as shown below
```
inject iidxhook2.dll backend.dll bm2dx.exe
```
- otherwise change it to
```
inject -K iidxhook5.dll -K backend.dll bm2dx.dll
```

## Credits
[Radioo/aic_key_eamio](https://github.com/Radioo/aic_key_eamio) - for the initial aic CardIO implementation and general help with netcode and CI

[aixxe/omnifix](https://github.com/aixxe/omnifix) - for the memory / module management code and general help regarding hooking

[dmadison/LED-Segment-ASCII](https://github.com/dmadison/LED-Segment-ASCII) - for base subscreen font (modified by me to match beatmania)


## Features
- Ticker
- keypads
- effector sliders
- spice2x CardIO reader support
- spcie2x card json support
## todo
- replay recording and uploading using official XRPC apis
- scene-awareness
