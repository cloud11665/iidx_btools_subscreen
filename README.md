# vefxio + eamio subscreen for iidx 12-24
<img height="360" alt="Capture" src="https://github.com/user-attachments/assets/ddc6eea5-0e5a-42a4-bc4e-d4a4b7b305ed" />

## Installation

- download the `dll-artifacts.zip` from the [latest commit artifact](https://github.com/cloud11665/iidx_btools_subscreen/actions) 
- extract it (`backend.dll`, `vefxio.dll`, `eamio.dll`)
- place `backend.dll`, `vefxio.dll`, `eamio.dll` in the same folder as your `gamestart-XX.bat`
- for styles that ship with `bm2dx.dll` edit `gamestart-XX.bat` as shown below
```
inject iidxhook2.dll backend.dll bm2dx.exe
```
- otherwise change it to
```
inject -K iidxhook5.dll -K backend.dll bm2dx.dll
```

### BIO2 IIDXIO + subscreen

The artifact also includes `iidxio-bio2-vefx.dll`, an optional adapter for
BIO2 setups. It loads the original Bemanitools BIO2 IIDXIO as a child DLL,
then routes the VEFX functions through this project's `vefxio.dll`:

- `iidx_io_set_loggers`, `iidx_io_init`, and `iidx_io_fini` initialize and
  shut down both the BIO2 child and `vefxio.dll`.
- `iidx_io_ep2_recv` lets `vefx_io_recv` update the normal IIDX pad word after
  the BIO2 child polls hardware.
- `iidx_io_ep2_get_slider` reads sliders from `vefxio.dll`.
- `iidx_io_ep3_write_16seg` writes ticker text to `vefxio.dll`.

To use it, place `backend.dll`, `vefxio.dll`, `iidxio-bio2-vefx.dll`, and the
stock Bemanitools BIO2 IIDXIO DLL in the game directory. Rename the stock BIO2
DLL to `iidxio-bio2-child.dll`, then copy or rename `iidxio-bio2-vefx.dll` to
the `iidxio.dll` filename expected by your IIDX hook. If you want a different
child DLL path, set `IIDX_SUBSCREEN_IIDXIO_CHILD` before launch.

## Features
- Ticker
- keypads
- effector sliders
- spice2x CardIO reader support
- spcie2x card json support
- concentration mode
## todo
- ~~replay recording and uploading using official XRPC apis~~
- ~~scene-awareness~~

## Credits
[Radioo/aic_key_eamio](https://github.com/Radioo/aic_key_eamio) - for the initial aic CardIO implementation and general help with netcode and CI

[aixxe/omnifix](https://github.com/aixxe/omnifix) - for the memory / module management code and general help regarding hooking

[dmadison/LED-Segment-ASCII](https://github.com/dmadison/LED-Segment-ASCII) - for base subscreen font (modified by me to match beatmania)

## changelog
- (master)
    - fixed spice2x card paths
- v0.2.0 (27.07.2025)
	- fixed alpha in some textures as they got saved without it
	- rewrote resource+texture management
	- added attributions to their respective authors
	- Added auto-disable of  windows touch accessibility visual feedback
 	- Added concentration mode	 
- v0.1.0 (26.07.2025)
	- initial release
	- keypads, ticker, CardIO driver, effector
