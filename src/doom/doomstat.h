//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//


#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_loop.h"

// We need the playr data structure as well.
#include "d_player.h"

// Game mode/mission
#include "d_mode.h"

#include "net_defs.h"

#include "crispy.h"


// ------------------------
// Command line parameters.
//
extern  boolean	nomonsters;	// checkparm of -nomonsters
extern  boolean	respawnparm;	// checkparm of -respawn
extern  boolean	fastparm;	// checkparm of -fast

extern  boolean	devparm;	// DEBUG: launched with -devparm


// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t	gamemode;
extern GameMission_t	gamemission;
extern GameVersion_t    gameversion;
extern GameVariant_t    gamevariant;

// Convenience macro.
// 'gamemission' can be equal to pack_chex or pack_hacx, but these are
// just modified versions of doom and doom2, and should be interpreted
// as the same most of the time.

#define logical_gamemission                             \
    (gamemission == pack_chex ? doom :                  \
     gamemission == pack_hacx ? doom2 : gamemission)

// Set if homebrew PWAD stuff has been added.
extern  boolean	modifiedgame;

// compatibility with old engines (monster behavior, metrics, etc.)
extern int compatibility, default_compatibility;          // killough 1/31/98

extern int demo_version;           // killough 7/19/98: Version of demo

// Only true when playing back an old demo -- used only in "corner cases"
// which break playback but are otherwise unnoticable or are just desirable:

#define demo_compatibility (demo_version < 200) /* killough 11/98: macroized */

#define mbf21 (demo_version == 221)

// killough 7/19/98: whether monsters should fight against each other
extern int monster_infighting, default_monster_infighting;

extern int monkeys, default_monkeys;

extern int demo_insurance, default_demo_insurance;      // killough 4/5/98

// -------------------------------------------
// killough 10/98: compatibility vector

enum {
  comp_telefrag,
  comp_dropoff,
  comp_vile,
  comp_pain,
  comp_skull,
  comp_blazing,
  comp_doorlight,
  comp_model,
  comp_god,
  comp_falloff,
  comp_floors,
  comp_skymap,
  comp_pursuit,
  comp_doorstuck,
  comp_staylift,
  comp_zombie,
  comp_stairs,
  comp_infcheat,
  comp_zerotags,

  // from PrBoom+/Eternity Engine (part of mbf21 spec)
  comp_respawn,
  comp_soul,

  // mbf21
  comp_ledgeblock,
  comp_friendlyspawn,

  MBF21_COMP_TOTAL,

  COMP_TOTAL=32  // Some extra room for additional variables
};

extern int comp[COMP_TOTAL], default_comp[COMP_TOTAL];

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t		startskill;
extern  int             startepisode;
extern	int		startmap;

// Savegame slot to load on startup.  This is the value provided to
// the -loadgame option.  If this has not been provided, this is -1.
extern  int             startloadgame;

extern  boolean		autostart;

// Selected by user.
extern  skill_t         gameskill;
extern  int		gameepisode;
extern  int		gamemap;

// If non-zero, exit the level after this number of minutes
extern  int             timelimit;

// Nightmare mode flag, single player.
extern  boolean         respawnmonsters;

// Netgame? Only true if >1 player.
extern  boolean	netgame;

// 0=Cooperative; 1=Deathmatch; 2=Altdeath
extern int deathmatch;

// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.

// From m_menu.c:
//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
extern int sfxVolume;
extern int musicVolume;

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  boolean statusbaractive;

extern  boolean automapactive;	// In AutoMap mode?
extern  boolean	menuactive;	// Menu overlayed?
extern  boolean	paused;		// Game Pause?
extern  boolean	viewactive;
extern  boolean	nodrawers;

extern  boolean testcontrols;
extern  int     testcontrols_mousespeed;

// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int	viewangleoffset;

// Player taking events, and displaying.
extern  int	consoleplayer;
extern  int	displayplayer;

// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int	totalkills;
extern	int	totalitems;
extern	int	totalsecret;
extern	int	extrakills; // [crispy] count spawned monsters

// Timer, for scores.
extern  int	levelstarttic;	// gametic at level start
extern  int	leveltime;	// tics in game play for par
extern  int	totalleveltimes; // [crispy] CPhipps - total time for all completed levels

// --------------------------------------
// DEMO playback/recording related stuff.

extern  boolean	usergame;
extern  boolean	demoplayback;
extern  boolean	demorecording;

// Round angleturn in ticcmds to the nearest 256.  This is used when
// recording Vanilla demos in netgames.
extern boolean lowres_turn;

// Quit after playing a demo from cmdline.
extern  boolean		singledemo;

extern  gamestate_t     gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

// Bookkeeping on players - state.
extern	player_t	players[MAXPLAYERS];

// Alive? Disconnected?
extern  boolean		playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
#define MAX_DM_STARTS   10
extern  mapthing_t      deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t*	deathmatch_p;

// Player spawn spots.
extern  mapthing_t      playerstarts[MAXPLAYERS];
extern  boolean         playerstartsingame[MAXPLAYERS];
// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t		wminfo;







//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern  char        *savegamedir;

// if true, load all graphics at level load
extern  boolean         precache;


// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t     wipegamestate;

extern  int             mouseSensitivity;
extern  int             mouseSensitivity_x2;
extern  int             mouseSensitivity_y;

extern  int             bodyqueslot;

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.
extern int		skyflatnum;

// Netgame stuff (buffers and pointers, i.e. indices).
extern	int		rndindex;
extern  ticcmd_t       *netcmds;


//-----------------------------------------------------------------------------

extern int allow_pushers;         // MT_PUSH Things    // phares 3/10/98
extern int default_allow_pushers;

extern int variable_friction;  // ice & mud            // phares 3/10/98
extern int default_variable_friction;

extern int monsters_remember;                          // killough 3/1/98
extern int default_monsters_remember;

extern int dogs, default_dogs;     // killough 7/19/98: Marine's best friend :)
extern int dog_jumping, default_dog_jumping;   // killough 10/98

// killough 8/8/98: distance friendly monsters tend to stay from player
extern int distfriend, default_distfriend;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
extern int monster_backing, default_monster_backing;

// killough 9/9/98: whether monsters intelligently avoid hazards
extern int monster_avoid_hazards, default_monster_avoid_hazards;

// killough 10/98: whether monsters are affected by friction
extern int monster_friction, default_monster_friction;

// killough 9/9/98: whether monsters help friends
extern int help_friends, default_help_friends;

extern int flashing_hom; // killough 10/98

extern int doom_weapon_toggles;   // killough 10/98

#endif
