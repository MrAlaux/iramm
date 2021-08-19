# Nugget Doom

Nugget Doom (formerly known as IRamm) is a fork of [Crispy Doom](https://www.chocolate-doom.org/wiki/index.php/Crispy_Doom), essentially intended as a personal playground to mess around with Doom's source code.

### Features

 * Crouching/ducking (default key: <kbd>C</kbd>)
 * A setting to adjust the player's POV closer to Doomguy's helmet
 * A setting to disable the crisp background in the Crispness menu
 * Crispy's 'Misc. Sound Fixes' setting now makes A_CPosAttack use the Pistol sound effect
 * A setting to fix miscellaneous bugs, namely the following:
   * / Archvile fire spawning at wrong location
   * (0,0) respawning bug
   * A_BruisAttack not calling A_FaceTarget
   * Chaingun plays sound twice with a single bullet
   * IDCHOPPERS invulnerability
   * Lost Soul colliding with items
   * Lost Soul forgets its target
   * No projectile knockback when Chainsaw is equipped
   * Pain state with 0 damage attacks
   * Lopsided Icon of Sin death explosions /
 * A setting to force Berserk Fist/Chainsaw/SSG gibbing
 * A setting to allow to switch between the Chainsaw and the non-Berserk Fist
 * Support for a DSCHGUN lump: if provided, it will be used for A_FireCGun, and for A_CPosAttack only if the above mentioned setting is enabled

### Building

As a Crispy Doom fork, itself a Chocolate Doom fork, their build instructions should also apply here: [Crispy Doom](https://github.com/fabiangreffrath/crispy-doom/wiki/Building-on-Windows); [Chocolate Doom](https://www.chocolate-doom.org/wiki/index.php/Building_Chocolate_Doom_on_Windows).

### Contact

The homepage for Nugget Doom is https://github.com/MrAlaux/Nugget-Doom

Please report any bugs, glitches or crashes that you encounter to the GitHub [Issue Tracker](https://github.com/MrAlaux/Nugget-Doom/issues).

### Acknowledgement

Some former code is based on A_BodyParts from [Chocolate Strife](https://github.com/chocolate-doom/chocolate-doom/blob/master/src/strife).

Help was provided by kraflab (responsible for [dsda-doom](https://github.com/kraflab/dsda-doom)) and Fabian Greffrath himself.

### Legalese (from Crispy's README.md)

Doom is © 1993-1996 Id Software, Inc.; 
Boom 2.02 is © 1999 id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman;
PrBoom+ is © 1999 id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman,
© 1999-2000 Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze,
© 2005-2006 Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko;
Chocolate Doom is © 1993-1996 Id Software, Inc., © 2005 Simon Howard; 
Chocolate Hexen is © 1993-1996 Id Software, Inc., © 1993-2008 Raven Software, © 2008 Simon Howard;
Strawberry Doom is © 1993-1996 Id Software, Inc., © 2005 Simon Howard, © 2008-2010 GhostlyDeath; 
Crispy Doom is additionally © 2014-2019 Fabian Greffrath;
all of the above are released under the [GPL-2+](https://www.gnu.org/licenses/gpl-2.0.html).

SDL 2.0, SDL_mixer 2.0 and SDL_net 2.0 are © 1997-2016 Sam Lantinga and are released under the [zlib license](http://www.gzip.org/zlib/zlib_license.html).

Secret Rabbit Code (libsamplerate) is © 2002-2011 Erik de Castro Lopo and is released under the [GPL-2+](http://www.gnu.org/licenses/gpl-2.0.html).
Libpng is © 1998-2014 Glenn Randers-Pehrson, © 1996-1997 Andreas Dilger, © 1995-1996 Guy Eric Schalnat, Group 42, Inc. and is released under the [libpng license](http://www.libpng.org/pub/png/src/libpng-LICENSE.txt).
Zlib is © 1995-2013 Jean-loup Gailly and Mark Adler and is released under the [zlib license](http://www.zlib.net/zlib_license.html).
