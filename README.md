regionFOUR
=======

regionFOUR is a region free loader for New3DS/New3DSXL/3DS/3DSXL/2DS which currently works on on firmware versions 9.2 and 9.7 this also allows you to bypass mandatory gamecard firmware updates.

it is a successor to regionthree made to rely on an exploit game (currently cubic ninja, zelda OOT support might come) rather than the web browser. as such it only requires an internet connection the first time it is run, and can then be run offline.

### How to use

Please see instructions on how to run regionFOUR on its webpage : http://smealum.net/regionfour/

### FAQ

- Does this work on the latest firmware version ? Yes, 9.7 is supported.
- Does this let me run homebrew and/or roms ? No, it only lets you run legit physical games from other regions.
- Do I need to connect to the internet every time I want to use this ? No, you only need to connect to the internet the first time. You can then install it to your gamecard's savegame.
- Do I need a flashcart/game/hardware for this ? Yes, regionFOUR currently requires that you own a copy of Cubic Ninja from your own region to run.
- Will this work on my New 3DS ? Yes, this works on the New 3DS, the New 3DS XL, as well as the 3DS, the 3DS XL and the 2DS.
- Will this break or brick my 3DS ? No. There's virtually 0 chance of that happening, all this runs is run of the mill usermode code, nothing dangerous. Nothing unusual is written to your NAND, nothing permanent is done. With that in mind, use at your own risk, I won't take responsibility if something weird does happen.
- Do you take donations ? No, I do not.
- How does it work ? See below.

### Technical stuff

Basically I reuse some ninjhax stuff to get code exec under an application (cubic ninja). From there I use the gspwn exploit to takeover home menu by overwriting a target object located on its linear heap with specially crafted data. With a fake vtable and a nice stack pivot I'm able to get ROP under home menu, and from there I ROP my wait into calling NSS:Reboot to bypass the region check.

For more detail on the cubic-ninja part of regionFOUR and the GPU DMA exploit (gspwn), visit http://smealum.net/?p=517

To build the ROP, use Kingcom's armips assembler https://github.com/Kingcom/armips

### Credits

- All original ROP and code on this repo written by smea
- ns:s region free booting trick and home menu stack pivot found by yellows8
- yellows8 and Myria for helping with testing.
- plutoo <3
