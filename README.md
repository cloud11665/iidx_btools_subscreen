# [WIP] vefxio + eamio subscreen for iidx 12-24
<img width="1920" height="1080" alt="Capture" src="https://github.com/user-attachments/assets/ddc6eea5-0e5a-42a4-bc4e-d4a4b7b305ed" />

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

## Features
- Ticker
- keypads
- effector sliders
- spice2x CardIO reader support
- spcie2x card json support
- concentration mode
## todo
- replay recording and uploading using official XRPC apis
- scene-awareness
- add auto-releases
- improve readme

## Credits
[Radioo/aic_key_eamio](https://github.com/Radioo/aic_key_eamio) - for the initial aic CardIO implementation and general help with netcode and CI

[aixxe/omnifix](https://github.com/aixxe/omnifix) - for the memory / module management code and general help regarding hooking

[dmadison/LED-Segment-ASCII](https://github.com/dmadison/LED-Segment-ASCII) - for base subscreen font (modified by me to match beatmania)

## changelog
- v0.2.0 (27.07.2025)
	- fixed alpha in some textures as they got saved without it
	- rewrote resource+texture management
	- added attributions to their respective authors
	- Added auto-disable of  windows touch accessibility visual feedback
 	- Added concentration mode	 
- v0.1.0 (26.07.2025)
	- initial release
	- keypads, ticker, CardIO driver, effector
