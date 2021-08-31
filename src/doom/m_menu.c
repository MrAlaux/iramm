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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//


#include <stdlib.h>
#include <ctype.h>


#include "doomdef.h"
#include "doomkeys.h"
#include "dstrings.h"

#include "d_main.h"
#include "deh_main.h"

#include "i_input.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_controls.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_extsaveg.h" // [crispy] savewadfilename

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"
#include "m_crispy.h" // [crispy] Crispness menu

#include "v_trans.h" // [crispy] colored "invert mouse" message

extern patch_t*		hu_font[HU_FONTSIZE];
extern boolean		message_dontfuckwithme;

extern boolean		chat_on;		// in heads-up code

//
// defaulted values
//
int			mouseSensitivity = 5;
int			mouseSensitivity_x2 = 5; // [crispy] mouse sensitivity menu
int			mouseSensitivity_y = 5; // [crispy] mouse sensitivity menu

// Show messages has default, 0 = off, 1 = on
int			showMessages = 1;


// Blocky mode, has default, 0 = high, 1 = normal
int			detailLevel = 0;
int			screenblocks = 10; // [crispy] increased

// temp for screenblocks (0-9)
int			screenSize;

// -1 = no quicksave slot picked!
int			quickSaveSlot;

 // 1 = message to be printed
int			messageToPrint;
// ...and here is the message string!
const char		*messageString;

// message x & y
int			messx;
int			messy;
int			messageLastMenuActive;

// timed message = no input from user
boolean			messageNeedsInput;

void    (*messageRoutine)(int response);

// [crispy] intermediate gamma levels
char gammamsg[5+4][26+2] =
{
    GAMMALVL0,
    GAMMALVL05,
    GAMMALVL1,
    GAMMALVL15,
    GAMMALVL2,
    GAMMALVL25,
    GAMMALVL3,
    GAMMALVL35,
    GAMMALVL4
};

// we are going to be entering a savegame string
int			saveStringEnter;
int             	saveSlot;	// which slot to save in
int			saveCharIndex;	// which char we're editing
static boolean          joypadSave = false; // was the save action initiated by joypad?
// old save description before edit
char			saveOldString[SAVESTRINGSIZE];

boolean			inhelpscreens;
boolean			menuactive;

#define SKULLXOFF		-32
#define LINEHEIGHT		16
#define CRISPY_LINEHEIGHT	10 // [crispy] Crispness menu

extern boolean		sendpause;
char			savegamestrings[10][SAVESTRINGSIZE];

char	endstring[160];

static boolean opldev;

extern boolean speedkeydown (void);

//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short	status;

    char	name[10];

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void	(*routine)(int choice);

    // hotkey in menu
    char	alphaKey;
    const char	*alttext; // [crispy] alternative text for menu items
} menuitem_t;



typedef struct menu_s
{
    short		numitems;	// # of menu items
    struct menu_s*	prevMenu;	// previous menu
    menuitem_t*		menuitems;	// menu items
    void		(*routine)();	// draw routine
    short		x;
    short		y;		// x,y of menu
    short		lastOn;		// last item user was on in menu
    short		lumps_missing;	// [crispy] indicate missing menu graphics lumps
} menu_t;

short		itemOn;			// menu item skull is on
short		skullAnimCounter;	// skull animation counter
short		whichSkull;		// which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
const char *skullName[2] = {"M_SKULL1","M_SKULL2"};

// current menudef
menu_t*	currentMenu;

// phares 3/30/98
// externs added for setup menus

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;
// [FG] mouse buttons for backward motion and turning right/left
extern int mousebbackward;
extern int mousebturnright;
extern int mousebturnleft;
// [FG] mouse button for "use"
extern int mousebuse;
// [FG] prev/next weapon keys and buttons
extern int mousebprevweapon;
extern int mousebnextweapon;
// [FG] double click acts as "use"
extern int dclick_use;
extern int joybfire;
extern int joybstrafe;
// [FG] strafe left/right joystick buttons
extern int joybstrafeleft;
extern int joybstraferight;
extern int joybuse;
extern int joybspeed;
// [FG] prev/next weapon joystick buttons
extern int joybprevweapon;
extern int joybnextweapon;
// [FG] automap joystick button
extern int joybautomap;
// [FG] main menu joystick button
extern int joybmainmenu;
extern int health_red;    // health amount less than which status is red
extern int health_yellow; // health amount less than which status is yellow
extern int health_green;  // health amount above is blue, below is green
extern int armor_red;     // armor amount less than which status is red
extern int armor_yellow;  // armor amount less than which status is yellow
extern int armor_green;   // armor amount above is blue, below is green
extern int ammo_red;      // ammo percent less than which status is red
extern int ammo_yellow;   // ammo percent less is yellow more green
extern int sts_always_red;// status numbers do not change colors
extern int sts_pct_always_gray;// status percents do not change colors
extern int sts_traditional_keys;  // display keys the traditional way
extern int mapcolor_back; // map background
extern int mapcolor_grid; // grid lines color
extern int mapcolor_wall; // normal 1s wall color
extern int mapcolor_fchg; // line at floor height change color
extern int mapcolor_cchg; // line at ceiling height change color
extern int mapcolor_clsd; // line at sector with floor=ceiling color
extern int mapcolor_rkey; // red key color
extern int mapcolor_bkey; // blue key color
extern int mapcolor_ykey; // yellow key color
extern int mapcolor_rdor; // red door color  (diff from keys to allow option)
extern int mapcolor_bdor; // blue door color (of enabling one but not other )
extern int mapcolor_ydor; // yellow door color
extern int mapcolor_tele; // teleporter line color
extern int mapcolor_secr; // secret sector boundary color
extern int mapcolor_exit; // jff 4/23/98 exit line
extern int mapcolor_unsn; // computer map unseen line color
extern int mapcolor_flat; // line with no floor/ceiling changes
extern int mapcolor_sprt; // general sprite color
extern int mapcolor_hair; // crosshair color
extern int mapcolor_sngl; // single player arrow color
extern int mapcolor_plyr[4];// colors for player arrows in multiplayer

extern int mapcolor_frnd;  // friends colors  // killough 8/8/98

extern int map_point_coordinates; // killough 10/98

extern char* chat_macros[];  // chat macros
extern char *wad_files[], *deh_files[]; // killough 10/98
extern const char* shiftxform;
extern int map_secret_after; //secrets do not appear til after bagged
extern default_t defaults[];

// end of externs added for setup menus

//
// PROTOTYPES
//
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);
static void M_Options(int choice);
static void M_EndGame(int choice);
static void M_ReadThis(int choice);
static void M_ReadThis2(int choice);
static void M_QuitDOOM(int choice);

static void M_ChangeMessages(int choice);
static void M_ChangeSensitivity(int choice);
static void M_ChangeSensitivity_x2(int choice); // [crispy] mouse sensitivity menu
static void M_ChangeSensitivity_y(int choice); // [crispy] mouse sensitivity menu
static void M_MouseInvert(int choice); // [crispy] mouse sensitivity menu
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
static void M_ChangeDetail(int choice);
static void M_SizeDisplay(int choice);
static void M_Mouse(int choice); // [crispy] mouse sensitivity menu
static void M_Sound(int choice);

static void M_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_QuickSave(void);
static void M_QuickLoad(void);

static void M_DrawMainMenu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawOptions(void);
static void M_DrawMouse(void); // [crispy] mouse sensitivity menu
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);
static void M_DrawSetup(void);                                     // phares 3/21/98
static void M_DrawHelp (void);                                     // phares 5/04/98

static void M_DrawSaveLoadBorder(int x,int y);
static void M_SetupNextMenu(menu_t *menudef);
static void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
static void M_WriteText(int x, int y, const char *string);
int  M_StringWidth(const char *string); // [crispy] un-static
static int  M_StringHeight(const char *string);
static void M_StartMessage(const char *string, void *routine, boolean input);
static void M_ClearMenus (void);

// phares 3/30/98
// prototypes added to support Setup Menus and Extended HELP screens

int  M_GetKeyString(int,int);
void M_Setup(int choice);
void M_KeyBindings(int choice);
void M_Weapons(int);
void M_StatusBar(int);
void M_Automap(int);
void M_Enemy(int);
void M_Messages(int);
void M_ChatStrings(int);
void M_InitExtendedHelp(void);
void M_ExtHelpNextScreen(int);
void M_ExtHelp(int);
int  M_GetPixelWidth(char*);
void M_DrawKeybnd(void);
void M_DrawWeapons(void);
void M_DrawMenuString(int,int,int);
void M_DrawStatusHUD(void);
void M_DrawExtHelp(void);
void M_DrawAutoMap(void);
void M_DrawEnemy(void);
void M_DrawMessages(void);
void M_DrawChatStrings(void);
void M_Compat(int);       // killough 10/98
void M_General(int);      // killough 10/98
void M_DrawCompat(void);  // killough 10/98
void M_DrawGeneral(void); // killough 10/98
// cph 2006/08/06 - M_DrawString() is the old M_DrawMenuString, except that it is not tied to menu_buffer
void M_DrawString(int,int,int,const char*);

// [FG] alternative text for missing menu graphics lumps
void M_DrawTitle(int x, int y, const char *patch, char *alttext);

menu_t NewDef;                                              // phares 5/04/98

// end of prototypes added to support Setup Menus and Extended HELP screens

// [crispy] Crispness menu
static void M_CrispnessCur(int choice);
static void M_CrispnessNext(int choice);
static void M_CrispnessPrev(int choice);
static void M_DrawCrispness1(void);
static void M_DrawCrispness2(void);
static void M_DrawCrispness3(void);
static void M_DrawCrispness4(void);
static void M_DrawCrispness5(void); // [Nugget]



//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainMenu[]=
{
    {1,"M_NGAME",M_NewGame,'n'},
    {1,"M_OPTION",M_Options,'o'},
    {1,"M_LOADG",M_LoadGame,'l'},
    {1,"M_SAVEG",M_SaveGame,'s'},
    // Another hickup with Special edition.
    {1,"M_RDTHIS",M_ReadThis,'r'},
    {1,"M_QUITG",M_QuitDOOM,'q'}
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainMenu,
    M_DrawMainMenu,
    97,64,
    0
};


//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep5, // [crispy] Sigil
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
{
    {1,"M_EPI1", M_Episode,'k'},
    {1,"M_EPI2", M_Episode,'t'},
    {1,"M_EPI3", M_Episode,'i'},
    {1,"M_EPI4", M_Episode,'t'}
   ,{1,"M_EPI5", M_Episode,'s'} // [crispy] Sigil
};

menu_t  EpiDef =
{
    ep_end,		// # of menu items
    &MainDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    48,63,              // x,y
    ep1			// lastOn
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
{
    {1,"M_JKILL",	M_ChooseSkill, 'i', "I'm too young to die."},
    {1,"M_ROUGH",	M_ChooseSkill, 'h', "Hey, not too rough!."},
    {1,"M_HURT",	M_ChooseSkill, 'h', "Hurt me plenty."},
    {1,"M_ULTRA",	M_ChooseSkill, 'u', "Ultra-Violence."},
    {1,"M_NMARE",	M_ChooseSkill, 'n', "Nightmare!"}
};

menu_t  NewDef =
{
    newg_end,		// # of menu items
    &EpiDef,		// previous menu
    NewGameMenu,	// menuitem_t ->
    M_DrawNewGame,	// drawing routine ->
    48,63,              // x,y
    hurtme		// lastOn
};



//
// OPTIONS MENU
//
enum
{
    general,
    setup,
    endgame,
    messages,
    //detail,
    scrnsize,
    option_empty1,
    mousesens,
    soundvol,
    //crispness, // [crispy] Crispness menu
    opt_end
} options_e;

menuitem_t OptionsMenu[]=
{
    {1,"M_GENERL", M_General, 'g', "GENERAL"},      // killough 10/98
    {1,"M_SETUP",  M_Setup,   's', "SETUP"},                          // phares 3/21/98
    {1,"M_ENDGAM",	M_EndGame,'e', "End Game"},
    {1,"M_MESSG",	M_ChangeMessages,'m', "Messages: "},
    //{1,"M_DETAIL",	M_ChangeDetail,'g', "Graphic Detail: "},
    {2,"M_SCRNSZ",	M_SizeDisplay,'s', "Screen Size"},
    {-1,"",0,'\0'},
    {1,"M_MSENS",	M_Mouse,'m', "Mouse Sensitivity"}, // [crispy] mouse sensitivity menu
    {1,"M_SVOL",	M_Sound,'s', "Sound Volume"},
    //{1,"M_CRISPY",	M_CrispnessCur,'c', "Crispness"} // [crispy] Crispness menu
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

// [crispy] mouse sensitivity menu
enum
{
    mouse_horiz,
    mouse_empty1,
    mouse_horiz2,
    mouse_empty2,
    mouse_vert,
    mouse_empty3,
    mouse_invert,
    mouse_end
} mouse_e;

static menuitem_t MouseMenu[]=
{
    {2,"",	M_ChangeSensitivity,'h'},
    {-1,"",0,'\0'},
    {2,"",	M_ChangeSensitivity_x2,'s'},
    {-1,"",0,'\0'},
    {2,"",	M_ChangeSensitivity_y,'v'},
    {-1,"",0,'\0'},
    {1,"",	M_MouseInvert,'i'},
};

static menu_t  MouseDef =
{
    mouse_end,
    &OptionsDef,
    MouseMenu,
    M_DrawMouse,
    80,64,
    0
};

// [crispy] Crispness menu
enum
{
    crispness_sep_rendering,
    crispness_hires,
    crispness_widescreen,
    crispness_uncapped,
    crispness_vsync,
    crispness_smoothscaling,
    crispness_sep_rendering_,

    crispness_sep_visual,
    crispness_coloredhud,
    crispness_translucency,
    crispness_smoothlight,
    crispness_brightmaps,
    crispness_coloredblood,
    crispness_flipcorpses,
    crispness_sep_visual_,

    crispness1_next,
    crispness1_prev,
    crispness1_end
} crispness1_e;

static menuitem_t Crispness1Menu[]=
{
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleHires,'h'},
    {1,"",	M_CrispyToggleWidescreen,'w'},
    {1,"",	M_CrispyToggleUncapped,'u'},
    {1,"",	M_CrispyToggleVsync,'v'},
    {1,"",	M_CrispyToggleSmoothScaling,'s'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleColoredhud,'c'},
    {1,"",	M_CrispyToggleTranslucency,'e'},
    {1,"",	M_CrispyToggleSmoothLighting,'s'},
    {1,"",	M_CrispyToggleBrightmaps,'b'},
    {1,"",	M_CrispyToggleColoredblood,'c'},
    {1,"",	M_CrispyToggleFlipcorpses,'r'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispnessNext,'n'},
    {1,"",	M_CrispnessPrev,'p'},
};

static menu_t  Crispness1Def =
{
    crispness1_end,
    &OptionsDef,
    Crispness1Menu,
    M_DrawCrispness1,
    48,28,
    1
};

enum
{
    crispness_sep_audible,
    crispness_soundfull,
    crispness_soundfix,
    crispness_sndchannels,
    crispness_soundmono,
    crispness_sep_audible_,

    crispness_sep_navigational,
    crispness_extautomap,
    crispness_smoothmap,
    crispness_automapstats,
    crispness_leveltime,
    crispness_playercoords,
    crispness_secretmessage,
    crispness_sep_navigational_,

    crispness2_next,
    crispness2_prev,
    crispness2_end
} crispness2_e;

static menuitem_t Crispness2Menu[]=
{
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleFullsounds,'p'},
    {1,"",	M_CrispyToggleSoundfixes,'m'},
    {1,"",	M_CrispyToggleSndChannels,'s'},
    {1,"",	M_CrispyToggleSoundMono,'m'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleExtAutomap,'e'},
    {1,"",	M_CrispyToggleSmoothMap,'m'},
    {1,"",	M_CrispyToggleAutomapstats,'s'},
    {1,"",	M_CrispyToggleLeveltime,'l'},
    {1,"",	M_CrispyTogglePlayerCoords,'p'},
    {1,"",	M_CrispyToggleSecretmessage,'s'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispnessNext,'n'},
    {1,"",	M_CrispnessPrev,'p'},
};

static menu_t  Crispness2Def =
{
    crispness2_end,
    &OptionsDef,
    Crispness2Menu,
    M_DrawCrispness2,
    48,28,
    1
};

enum
{
    crispness_sep_tactical,
    crispness_freelook,
    crispness_mouselook,
    crispness_bobfactor,
    crispness_centerweapon,
    crispness_weaponsquat,
    crispness_pitch,
    crispness_neghealth,
    crispness_sep_tactical_,

    crispness_sep_crosshair,
    crispness_crosshair,
    crispness_crosshairtype,
    crispness_crosshairhealth,
    crispness_crosshairtarget,
    crispness_sep_crosshair_,

    crispness3_next,
    crispness3_prev,
    crispness3_end
} crispness3_e;

static menuitem_t Crispness3Menu[]=
{
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleFreelook,'a'},
    {1,"",	M_CrispyToggleMouseLook,'p'},
    {1,"",	M_CrispyToggleBobfactor,'p'},
    {1,"",	M_CrispyToggleCenterweapon,'c'},
    {1,"",	M_CrispyToggleWeaponSquat,'w'},
    {1,"",	M_CrispyTogglePitch,'w'},
    {1,"",	M_CrispyToggleNeghealth,'n'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleCrosshair,'d'},
    {1,"",	M_CrispyToggleCrosshairtype,'c'},
    {1,"",	M_CrispyToggleCrosshairHealth,'c'},
    {1,"",	M_CrispyToggleCrosshairTarget,'h'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispnessNext,'n'},
    {1,"",	M_CrispnessPrev,'p'},
};

static menu_t  Crispness3Def =
{
    crispness3_end,
    &OptionsDef,
    Crispness3Menu,
    M_DrawCrispness3,
    48,28,
    1
};

enum
{
    crispness_sep_physical,
    crispness_freeaim,
    crispness_jumping,
    crispness_overunder,
    crispness_recoil,
    crispness_sep_physical_,

    crispness_sep_demos,
    crispness_demotimer,
    crispness_demotimerdir,
    crispness_demobar,
    crispness_demousetimer,
    crispness_sep_demos_,

    crispness4_next,
    crispness4_prev,
    crispness4_end
} crispness4_e;


static menuitem_t Crispness4Menu[]=
{
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleFreeaim,'v'},
    {1,"",	M_CrispyToggleJumping,'a'},
    {1,"",	M_CrispyToggleOverunder,'w'},
    {1,"",	M_CrispyToggleRecoil,'w'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispyToggleDemoTimer,'v'},
    {1,"",	M_CrispyToggleDemoTimerDir,'a'},
    {1,"",	M_CrispyToggleDemoBar,'w'},
    {1,"",	M_CrispyToggleDemoUseTimer,'u'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispnessNext,'n'},
    {1,"",	M_CrispnessPrev,'p'},
};

static menu_t  Crispness4Def =
{
    crispness4_end,
    &OptionsDef,
    Crispness4Menu,
    M_DrawCrispness4,
    48,28,
    1
};

enum
{
    crispness_sep_nugget,
    crispness_nocrispnessbg,
    crispness_extragibbing,
    crispness_fistswitch,
    crispness_viewheight,
    crispness_bugfixes,
    crispness_sep_nugget_,

    crispness5_next,
    crispness5_prev,
    crispness5_end
} crispness5_e; // [Nugget]


static menuitem_t Crispness5Menu[]= // [Nugget]
{
    {-1,"",0,'\0'},
    {1,"",	M_NuggetToggleCrispBackground,'c'},
    {1,"",	M_NuggetToggleExtraGibbing,'g'},
    {1,"",	M_NuggetToggleFistSwitch,'f'},
    {1,"",	M_NuggetToggleViewheight,'v'},
    {1,"",	M_NuggetToggleBugFixes,'b'},
    {-1,"",0,'\0'},
    {1,"",	M_CrispnessNext,'n'},
    {1,"",	M_CrispnessPrev,'p'},
};

static menu_t  Crispness5Def =
{
    crispness5_end,
    &OptionsDef,
    Crispness5Menu,
    M_DrawCrispness5,
    48,28,
    1
};

static menu_t *CrispnessMenus[] =
{
	&Crispness1Def,
	&Crispness2Def,
	&Crispness3Def,
	&Crispness4Def,
	&Crispness5Def, // [Nugget]
};

static int crispness_cur;

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[]=
{
    {1,"",M_FinishReadThis,0}
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330,175,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    sound_end
} sound_e;

menuitem_t SoundMenu[]=
{
    {2,"M_SFXVOL",M_SfxVol,'s'},
    {-1,"",0,'\0'},
    {2,"M_MUSVOL",M_MusicVol,'m'},
    {-1,"",0,'\0'}
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    80,64,
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load7, // [crispy] up to 8 savegames
    load8, // [crispy] up to 8 savegames
    load_end
} load_e;

menuitem_t LoadMenu[]=
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'},
    {1,"", M_LoadSelect,'7'}, // [crispy] up to 8 savegames
    {1,"", M_LoadSelect,'8'}  // [crispy] up to 8 savegames
};

menu_t  LoadDef =
{
    load_end,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'},
    {1,"", M_SaveSelect,'7'}, // [crispy] up to 8 savegames
    {1,"", M_SaveSelect,'8'}  // [crispy] up to 8 savegames
};

menu_t  SaveDef =
{
    load_end,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
    FILE   *handle;
    int     i;
    char    name[256];

    for (i = 0;i < load_end;i++)
    {
        int retval;
        M_StringCopy(name, P_SaveGameFile(i), sizeof(name));

	handle = fopen(name, "rb");
        if (handle == NULL)
        {
            M_StringCopy(savegamestrings[i], EMPTYSTRING, SAVESTRINGSIZE);
            LoadMenu[i].status = 0;
            continue;
        }
        retval = fread(&savegamestrings[i], 1, SAVESTRINGSIZE, handle);
	fclose(handle);
        LoadMenu[i].status = retval == SAVESTRINGSIZE;
    }
}


//
// M_LoadGame & Cie.
//
static int LoadDef_x = 72, LoadDef_y = 28;
void M_DrawLoad(void)
{
    int             i;

    V_DrawPatchDirect(LoadDef_x, LoadDef_y,
                      W_CacheLumpName(DEH_String("M_LOADG"), PU_CACHE));

    for (i = 0;i < load_end; i++)
    {
	M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
	M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
    int             i;

    V_DrawPatchDirect(x - 8, y + 7,
                      W_CacheLumpName(DEH_String("M_LSLEFT"), PU_CACHE));

    for (i = 0;i < 24;i++)
    {
	V_DrawPatchDirect(x, y + 7,
                          W_CacheLumpName(DEH_String("M_LSCNTR"), PU_CACHE));
	x += 8;
    }

    V_DrawPatchDirect(x, y + 7,
                      W_CacheLumpName(DEH_String("M_LSRGHT"), PU_CACHE));
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    char    name[256];

    M_StringCopy(name, P_SaveGameFile(choice), sizeof(name));

    // [crispy] save the last game you loaded
    SaveDef.lastOn = choice;
    G_LoadGame (name);
    M_ClearMenus ();

    // [crispy] allow quickload before quicksave
    if (quickSaveSlot == -2)
	quickSaveSlot = choice;
}

//
// Selected from DOOM menu
//
void M_LoadGame (int choice)
{
    // [crispy] allow loading game while multiplayer demo playback
    if (netgame && !demoplayback)
    {
	M_StartMessage(DEH_String(LOADNET),NULL,false);
	return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}


//
//  M_SaveGame & Cie.
//
static int SaveDef_x = 72, SaveDef_y = 28;
void M_DrawSave(void)
{
    int             i;

    V_DrawPatchDirect(SaveDef_x, SaveDef_y, W_CacheLumpName(DEH_String("M_SAVEG"), PU_CACHE));
    for (i = 0;i < load_end; i++)
    {
	M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
	M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }

    if (saveStringEnter)
    {
	i = M_StringWidth(savegamestrings[saveSlot]);
	M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*saveSlot,"_");
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame (slot,savegamestrings[slot]);
    M_ClearMenus ();

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
	quickSaveSlot = slot;
}

//
// Generate a default save slot name when the user saves to
// an empty slot via the joypad.
//
static void SetDefaultSaveName(int slot)
{
    // map from IWAD or PWAD?
    if (W_IsIWADLump(maplumpinfo) && strcmp(savegamedir, ""))
    {
        M_snprintf(savegamestrings[itemOn], SAVESTRINGSIZE,
                   "%s", maplumpinfo->name);
    }
    else
    {
        char *wadname = M_StringDuplicate(W_WadNameForLump(maplumpinfo));
        char *ext = strrchr(wadname, '.');

        if (ext != NULL)
        {
            *ext = '\0';
        }

        M_snprintf(savegamestrings[itemOn], SAVESTRINGSIZE,
                   "%s (%s)", maplumpinfo->name,
                   wadname);
        free(wadname);
    }
    M_ForceUppercase(savegamestrings[itemOn]);
    joypadSave = false;
}

// [crispy] override savegame name if it already starts with a map identifier
static boolean StartsWithMapIdentifier (char *str)
{
    M_ForceUppercase(str);

    if (strlen(str) >= 4 &&
        str[0] == 'E' && isdigit(str[1]) &&
        str[2] == 'M' && isdigit(str[3]))
    {
        return true;
    }

    if (strlen(str) >= 5 &&
        str[0] == 'M' && str[1] == 'A' && str[2] == 'P' &&
        isdigit(str[3]) && isdigit(str[4]))
    {
        return true;
    }

    return false;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    int x, y;

    // we are going to be intercepting all chars
    saveStringEnter = 1;

    // [crispy] load the last game you saved
    LoadDef.lastOn = choice;

    // We need to turn on text input:
    x = LoadDef.x - 11;
    y = LoadDef.y + choice * LINEHEIGHT - 4;
    I_StartTextInput(x, y, x + 8 + 24 * 8 + 8, y + LINEHEIGHT - 2);

    saveSlot = choice;
    M_StringCopy(saveOldString,savegamestrings[choice], SAVESTRINGSIZE);
    if (!strcmp(savegamestrings[choice], EMPTYSTRING) ||
        // [crispy] override savegame name if it already starts with a map identifier
        StartsWithMapIdentifier(savegamestrings[choice]))
    {
        savegamestrings[choice][0] = 0;

        if (joypadSave || true) // [crispy] always prefill empty savegame slot names
        {
            SetDefaultSaveName(choice);
        }
    }
    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
    if (!usergame)
    {
	M_StartMessage(DEH_String(SAVEDEAD),NULL,false);
	return;
    }

    if (gamestate != GS_LEVEL)
	return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}



//
//      M_QuickSave
//
static char tempstring[90];

void M_QuickSaveResponse(int key)
{
    if (key == key_menu_confirm)
    {
	M_DoSave(quickSaveSlot);
	S_StartSound(NULL,sfx_swtchx);
    }
}

void M_QuickSave(void)
{
    if (!usergame)
    {
	S_StartSound(NULL,sfx_oof);
	return;
    }

    if (gamestate != GS_LEVEL)
	return;

    if (quickSaveSlot < 0)
    {
	M_StartControlPanel();
	M_ReadSaveStrings();
	M_SetupNextMenu(&SaveDef);
	quickSaveSlot = -2;	// means to pick a slot now
	return;
    }
    DEH_snprintf(tempstring, sizeof(tempstring),
                 QSPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickSaveResponse, true);
}



//
// M_QuickLoad
//
void M_QuickLoadResponse(int key)
{
    if (key == key_menu_confirm)
    {
	M_LoadSelect(quickSaveSlot);
	S_StartSound(NULL,sfx_swtchx);
    }
}


void M_QuickLoad(void)
{
    // [crispy] allow quickloading game while multiplayer demo playback
    if (netgame && !demoplayback)
    {
	M_StartMessage(DEH_String(QLOADNET),NULL,false);
	return;
    }

    if (quickSaveSlot < 0)
    {
	// [crispy] allow quickload before quicksave
	M_StartControlPanel();
	M_ReadSaveStrings();
	M_SetupNextMenu(&LoadDef);
	quickSaveSlot = -2;
	return;
    }
    DEH_snprintf(tempstring, sizeof(tempstring),
                 QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, true);
}




//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
    inhelpscreens = true;

    V_DrawPatchFullScreen(W_CacheLumpName(DEH_String("HELP2"), PU_CACHE), false);
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
    inhelpscreens = true;

    // We only ever draw the second page if this is
    // gameversion == exe_doom_1_9 and gamemode == registered

    V_DrawPatchFullScreen(W_CacheLumpName(DEH_String("HELP1"), PU_CACHE), false);
}

void M_DrawReadThisCommercial(void)
{
    inhelpscreens = true;

    V_DrawPatchFullScreen(W_CacheLumpName(DEH_String("HELP"), PU_CACHE), false);
}


//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
    V_DrawPatchDirect (60, 38, W_CacheLumpName(DEH_String("M_SVOL"), PU_CACHE));

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),
		 16,sfxVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),
		 16,musicVolume);
}

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
    switch(choice)
    {
      case 0:
	if (sfxVolume)
	    sfxVolume--;
	break;
      case 1:
	if (sfxVolume < 15)
	    sfxVolume++;
	break;
    }

    S_SetSfxVolume(sfxVolume * 8);
}

void M_MusicVol(int choice)
{
    switch(choice)
    {
      case 0:
	if (musicVolume)
	    musicVolume--;
	break;
      case 1:
	if (musicVolume < 15)
	    musicVolume++;
	break;
    }

    S_SetMusicVolume(musicVolume * 8);
}




//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
    // [crispy] force status bar refresh
    inhelpscreens = true;

    V_DrawPatchDirect(94, 2,
                      W_CacheLumpName(DEH_String("M_DOOM"), PU_CACHE));
}




//
// M_NewGame
//
void M_DrawNewGame(void)
{
    // [crispy] force status bar refresh
    inhelpscreens = true;

    V_DrawPatchDirect(96, 14, W_CacheLumpName(DEH_String("M_NEWG"), PU_CACHE));
    V_DrawPatchDirect(54, 38, W_CacheLumpName(DEH_String("M_SKILL"), PU_CACHE));
}

void M_NewGame(int choice)
{
    // [crispy] forbid New Game while recording a demo
    if (demorecording)
    {
	return;
    }

    if (netgame && !demoplayback)
    {
	M_StartMessage(DEH_String(NEWGAME),NULL,false);
	return;
    }

    // Chex Quest disabled the episode select screen, as did Doom II.

    if ((gamemode == commercial && !crispy->havenerve && !crispy->havemaster) || gameversion == exe_chex) // [crispy] NRFTL / The Master Levels
	M_SetupNextMenu(&NewDef);
    else
	M_SetupNextMenu(&EpiDef);
}


//
//      M_Episode
//
int     epi;

void M_DrawEpisode(void)
{
    // [crispy] force status bar refresh
    inhelpscreens = true;

    V_DrawPatchDirect(54, 38, W_CacheLumpName(DEH_String("M_EPISOD"), PU_CACHE));
}

void M_VerifyNightmare(int key)
{
    if (key != key_menu_confirm)
	return;

    G_DeferedInitNew(nightmare,epi+1,1);
    M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
	M_StartMessage(DEH_String(NIGHTMARE),M_VerifyNightmare,true);
	return;
    }

    G_DeferedInitNew(choice,epi+1,1);
    M_ClearMenus ();
}

void M_Episode(int choice)
{
    if ( (gamemode == shareware)
	 && choice)
    {
	M_StartMessage(DEH_String(SWSTRING),NULL,false);
	M_SetupNextMenu(&ReadDef1);
	return;
    }

    epi = choice;
    M_SetupNextMenu(&NewDef);
}



//
// M_Options
//
static const char *detailNames[2] = {"M_GDHIGH","M_GDLOW"};
static const char *msgNames[2] = {"M_MSGOFF","M_MSGON"};

void M_DrawOptions(void)
{
    V_DrawPatchDirect(108, 15, W_CacheLumpName(DEH_String("M_OPTTTL"),
                                               PU_CACHE));

//    if (OptionsDef.lumps_missing == -1)
//    {
//    V_DrawPatchDirect(OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT * detail,
//		      W_CacheLumpName(DEH_String(detailNames[detailLevel]),
//			              PU_CACHE));
//    }
//    else
//    if (OptionsDef.lumps_missing > 0)
//    {
//    M_WriteText(OptionsDef.x + M_StringWidth("Graphic Detail: "),
//                OptionsDef.y + LINEHEIGHT * detail + 8 - (M_StringHeight("HighLow")/2),
//                detailLevel ? "Low" : "High");
//    }

    if (OptionsDef.lumps_missing == -1)
    {
    V_DrawPatchDirect(OptionsDef.x + 120, OptionsDef.y + LINEHEIGHT * messages,
                      W_CacheLumpName(DEH_String(msgNames[showMessages]),
                                      PU_CACHE));
    }
    else
    if (OptionsDef.lumps_missing > 0)
    {
    M_WriteText(OptionsDef.x + M_StringWidth("Messages: "),
                OptionsDef.y + LINEHEIGHT * messages + 8 - (M_StringHeight("OnOff")/2),
                showMessages ? "On" : "Off");
    }

// [crispy] mouse sensitivity menu
/*
    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1),
		 10, mouseSensitivity);
*/

    M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),
		 9 + (crispy->widescreen ? 6 : 3),screenSize); // [crispy] Crispy HUD
}

// [crispy] mouse sensitivity menu
static void M_DrawMouse(void)
{
    char mouse_menu_text[48];

    V_DrawPatchDirect (60, LoadDef_y, W_CacheLumpName(DEH_String("M_MSENS"), PU_CACHE));

    M_WriteText(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_horiz + 6,
                "HORIZONTAL: TURN");

    M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_empty1,
		 21, mouseSensitivity);

    M_WriteText(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_horiz2 + 6,
                "HORIZONTAL: STRAFE");

    M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_empty2,
		 21, mouseSensitivity_x2);

    M_WriteText(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_vert + 6,
                "VERTICAL");

    M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_empty3,
		 21, mouseSensitivity_y);

    M_snprintf(mouse_menu_text, sizeof(mouse_menu_text),
               "%sInvert Vertical Axis: %s%s", crstr[CR_NONE],
               mouse_y_invert ? crstr[CR_GREEN] : crstr[CR_DARK],
               mouse_y_invert ? "On" : "Off");
    M_WriteText(MouseDef.x, MouseDef.y + LINEHEIGHT * mouse_invert + 6,
                mouse_menu_text);

    dp_translation = NULL;
}

// [crispy] Crispness menu
#include "m_background.h"
static void M_DrawCrispnessBackground(void)
{
	const byte *const src = crispness_background;
	pixel_t *dest;
	int x, y;

	if (crispy->nocrispnessbg)
        {return;}

	dest = I_VideoBuffer;

	for (y = 0; y < SCREENHEIGHT; y++)
	{
		for (x = 0; x < SCREENWIDTH; x++)
		{
#ifndef CRISPY_TRUECOLOR
			*dest++ = src[(y & 63) * 64 + (x & 63)];
#else
			*dest++ = colormaps[src[(y & 63) * 64 + (x & 63)]];
#endif
		}
	}

	inhelpscreens = true;
}

static char crispy_menu_text[48];

static void M_DrawCrispnessHeader(const char *item)
{
    M_snprintf(crispy_menu_text, sizeof(crispy_menu_text),
               "%s%s", crstr[CR_GOLD], item);
    M_WriteText(ORIGWIDTH/2 - M_StringWidth(item) / 2, 12, crispy_menu_text);
}

static void M_DrawCrispnessSeparator(int y, const char *item)
{
    M_snprintf(crispy_menu_text, sizeof(crispy_menu_text),
               "%s%s", crstr[CR_GOLD], item);
    M_WriteText(currentMenu->x - 8, currentMenu->y + CRISPY_LINEHEIGHT * y, crispy_menu_text);
}

static void M_DrawCrispnessItem(int y, const char *item, int feat, boolean cond)
{
    M_snprintf(crispy_menu_text, sizeof(crispy_menu_text),
               "%s%s: %s%s", cond ? crstr[CR_NONE] : crstr[CR_DARK], item,
               cond ? (feat ? crstr[CR_GREEN] : crstr[CR_DARK]) : crstr[CR_DARK],
               cond && feat ? "On" : "Off");
    M_WriteText(currentMenu->x, currentMenu->y + CRISPY_LINEHEIGHT * y, crispy_menu_text);
}

static void M_DrawCrispnessMultiItem(int y, const char *item, multiitem_t *multiitem, int feat, boolean cond)
{
    M_snprintf(crispy_menu_text, sizeof(crispy_menu_text),
               "%s%s: %s%s", cond ? crstr[CR_NONE] : crstr[CR_DARK], item,
               cond ? (feat ? crstr[CR_GREEN] : crstr[CR_DARK]) : crstr[CR_DARK],
               cond && feat ? multiitem[feat].name : multiitem[0].name);
    M_WriteText(currentMenu->x, currentMenu->y + CRISPY_LINEHEIGHT * y, crispy_menu_text);
}

static void M_DrawCrispnessGoto(int y, const char *item)
{
    M_snprintf(crispy_menu_text, sizeof(crispy_menu_text),
               "%s%s", crstr[CR_GOLD], item);
    M_WriteText(currentMenu->x, currentMenu->y + CRISPY_LINEHEIGHT * y, crispy_menu_text);
}

static void M_DrawCrispness1(void)
{
    M_DrawCrispnessBackground();

    M_DrawCrispnessHeader("Crispness 1/5");

    M_DrawCrispnessSeparator(crispness_sep_rendering, "Rendering");
    M_DrawCrispnessItem(crispness_hires, "High Resolution Rendering", crispy->hires, true);
    M_DrawCrispnessMultiItem(crispness_widescreen, "Widescreen Aspect Ratio", multiitem_widescreen, crispy->widescreen, aspect_ratio_correct);
    M_DrawCrispnessItem(crispness_uncapped, "Uncapped Framerate", crispy->uncapped, true);
    M_DrawCrispnessItem(crispness_vsync, "Enable VSync", crispy->vsync, !force_software_renderer);
    M_DrawCrispnessItem(crispness_smoothscaling, "Smooth Pixel Scaling", crispy->smoothscaling, true);

    M_DrawCrispnessSeparator(crispness_sep_visual, "Visual");
    M_DrawCrispnessMultiItem(crispness_coloredhud, "Colorize HUD Elements", multiitem_coloredhud, crispy->coloredhud, true);
    M_DrawCrispnessMultiItem(crispness_translucency, "Enable Translucency", multiitem_translucency, crispy->translucency, true);
    M_DrawCrispnessItem(crispness_smoothlight, "Smooth Diminishing Lighting", crispy->smoothlight, true);
    M_DrawCrispnessMultiItem(crispness_brightmaps, "Apply Brightmaps to", multiitem_brightmaps, crispy->brightmaps, true);
    M_DrawCrispnessItem(crispness_coloredblood, "Colored Blood and Corpses", crispy->coloredblood, gameversion != exe_chex);
    M_DrawCrispnessItem(crispness_flipcorpses, "Randomly Mirrored Corpses", crispy->flipcorpses, gameversion != exe_chex);

    M_DrawCrispnessGoto(crispness1_next, "Next Page >");
    M_DrawCrispnessGoto(crispness1_prev, "< Last Page");

    dp_translation = NULL;
}

static void M_DrawCrispness2(void)
{
    M_DrawCrispnessBackground();

    M_DrawCrispnessHeader("Crispness 2/5");

    M_DrawCrispnessSeparator(crispness_sep_audible, "Audible");
    M_DrawCrispnessItem(crispness_soundfull, "Play sounds in full length", crispy->soundfull, true);
    M_DrawCrispnessItem(crispness_soundfix, "Misc. Sound Fixes", crispy->soundfix, true);
    M_DrawCrispnessMultiItem(crispness_sndchannels, "Sound Channels", multiitem_sndchannels, snd_channels >> 4, snd_sfxdevice != SNDDEVICE_PCSPEAKER);
    M_DrawCrispnessItem(crispness_soundmono, "Mono SFX", crispy->soundmono, true);

    M_DrawCrispnessSeparator(crispness_sep_navigational, "Navigational");
    M_DrawCrispnessItem(crispness_extautomap, "Extended Automap colors", crispy->extautomap, true);
    M_DrawCrispnessItem(crispness_smoothmap, "Smooth automap lines", crispy->smoothmap, true);
    M_DrawCrispnessMultiItem(crispness_automapstats, "Show Level Stats", multiitem_widgets, crispy->automapstats, true);
    M_DrawCrispnessMultiItem(crispness_leveltime, "Show Level Time", multiitem_widgets, crispy->leveltime, true);
    M_DrawCrispnessMultiItem(crispness_playercoords, "Show Player Coords", multiitem_widgets, crispy->playercoords, true);
    M_DrawCrispnessMultiItem(crispness_secretmessage, "Report Revealed Secrets", multiitem_secretmessage, crispy->secretmessage, true);

    M_DrawCrispnessGoto(crispness2_next, "Next Page >");
    M_DrawCrispnessGoto(crispness2_prev, "< Prev Page");

    dp_translation = NULL;
}

static void M_DrawCrispness3(void)
{
    M_DrawCrispnessBackground();

    M_DrawCrispnessHeader("Crispness 3/5");

    M_DrawCrispnessSeparator(crispness_sep_tactical, "Tactical");

    M_DrawCrispnessMultiItem(crispness_freelook, "Allow Free Look", multiitem_freelook, crispy->freelook, true);
    M_DrawCrispnessItem(crispness_mouselook, "Permanent Mouse Look", crispy->mouselook, true);
    M_DrawCrispnessMultiItem(crispness_bobfactor, "Player View/Weapon Bobbing", multiitem_bobfactor, crispy->bobfactor, true);
    M_DrawCrispnessMultiItem(crispness_centerweapon, "Weapon Attack Alignment", multiitem_centerweapon, crispy->centerweapon, crispy->bobfactor != BOBFACTOR_OFF);
    M_DrawCrispnessItem(crispness_weaponsquat, "Squat weapon down on impact", crispy->weaponsquat, true);
    M_DrawCrispnessItem(crispness_pitch, "Weapon Recoil Pitch", crispy->pitch, true);
    M_DrawCrispnessItem(crispness_neghealth, "Negative Player Health", crispy->neghealth, true);

    M_DrawCrispnessSeparator(crispness_sep_crosshair, "Crosshair");

    M_DrawCrispnessMultiItem(crispness_crosshair, "Draw Crosshair", multiitem_crosshair, crispy->crosshair, true);
    M_DrawCrispnessMultiItem(crispness_crosshairtype, "Crosshair Shape", multiitem_crosshairtype, crispy->crosshairtype + 1, crispy->crosshair);
    M_DrawCrispnessItem(crispness_crosshairhealth, "Color indicates Health", crispy->crosshairhealth, crispy->crosshair);
    M_DrawCrispnessItem(crispness_crosshairtarget, "Highlight on target", crispy->crosshairtarget, crispy->crosshair);

    M_DrawCrispnessGoto(crispness3_next, "Next Page >");
    M_DrawCrispnessGoto(crispness3_prev, "< Prev Page");

    dp_translation = NULL;
}

static void M_DrawCrispness4(void)
{
    M_DrawCrispnessBackground();

    M_DrawCrispnessHeader("Crispness 4/5");

    M_DrawCrispnessSeparator(crispness_sep_physical, "Physical");

    M_DrawCrispnessMultiItem(crispness_freeaim, "Vertical Aiming", multiitem_freeaim, crispy->freeaim, crispy->singleplayer);
    M_DrawCrispnessMultiItem(crispness_jumping, "Allow Jumping/Crouching", multiitem_jump, crispy->jump, crispy->singleplayer);
    M_DrawCrispnessItem(crispness_overunder, "Walk over/under Monsters", crispy->overunder, crispy->singleplayer);
    M_DrawCrispnessItem(crispness_recoil, "Weapon Recoil Thrust", crispy->recoil, crispy->singleplayer);

    M_DrawCrispnessSeparator(crispness_sep_demos, "Demos");

    M_DrawCrispnessMultiItem(crispness_demotimer, "Show Demo Timer", multiitem_demotimer, crispy->demotimer, true);
    M_DrawCrispnessMultiItem(crispness_demotimerdir, "Playback Timer Direction", multiitem_demotimerdir, crispy->demotimerdir + 1, crispy->demotimer & DEMOTIMER_PLAYBACK);
    M_DrawCrispnessItem(crispness_demobar, "Show Demo Progress Bar", crispy->demobar, true);
    M_DrawCrispnessItem(crispness_demousetimer, "\"Use\" Button Timer", crispy->btusetimer, true);

    M_DrawCrispnessGoto(crispness4_next, "Next Page >");
    M_DrawCrispnessGoto(crispness4_prev, "< Prev Page");

    dp_translation = NULL;
}

static void M_DrawCrispness5(void)
{
    M_DrawCrispnessBackground();

    M_DrawCrispnessHeader("Crispness 5/5");

    M_DrawCrispnessSeparator(crispness_sep_nugget, "Nugget"); // [Nugget]
    M_DrawCrispnessItem(crispness_nocrispnessbg, "No Crisp Background", crispy->nocrispnessbg, true);
    M_DrawCrispnessItem(crispness_extragibbing, "Extra Gibbing", crispy->extragibbing, crispy->singleplayer);
    M_DrawCrispnessItem(crispness_fistswitch, "Unpowered Fist/CSaw switch", crispy->fistswitch, crispy->singleplayer);
    M_DrawCrispnessItem(crispness_viewheight, "Adjust view height", crispy->viewheight, crispy->singleplayer);
    M_DrawCrispnessItem(crispness_bugfixes, "Misc. Bug Fixes", crispy->bugfixes, crispy->singleplayer);

    M_DrawCrispnessGoto(crispness5_next, "First Page >");
    M_DrawCrispnessGoto(crispness5_prev, "< Prev Page");

    dp_translation = NULL;
}

void M_Options(int choice)
{
    M_SetupNextMenu(&OptionsDef);
}

// [crispy] correctly handle inverted y-axis
static void M_Mouse(int choice)
{
    if (mouseSensitivity_y < 0)
    {
        mouseSensitivity_y = -mouseSensitivity_y;
        mouse_y_invert = 1;
    }

    if (mouse_acceleration_y < 0)
    {
        mouse_acceleration_y = -mouse_acceleration_y;
        mouse_y_invert = 1;
    }

    M_SetupNextMenu(&MouseDef);
}

static void M_CrispnessCur(int choice)
{
    M_SetupNextMenu(CrispnessMenus[crispness_cur]);
}

static void M_CrispnessNext(int choice)
{
    if (++crispness_cur > arrlen(CrispnessMenus) - 1)
    {
	crispness_cur = 0;
    }

    M_CrispnessCur(0);
}

static void M_CrispnessPrev(int choice)
{
    if (--crispness_cur < 0)
    {
	crispness_cur = arrlen(CrispnessMenus) - 1;
    }

    M_CrispnessCur(0);
}


//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
	players[consoleplayer].message = DEH_String(MSGOFF);
    else
	players[consoleplayer].message = DEH_String(MSGON);

    message_dontfuckwithme = true;
}


//
// M_EndGame
//
void M_EndGameResponse(int key)
{
    if (key != key_menu_confirm)
	return;

    // [crispy] killough 5/26/98: make endgame quit if recording or playing back demo
    if (demorecording || singledemo)
	G_CheckDemoStatus();

    // [crispy] clear quicksave slot
    quickSaveSlot = -1;
    currentMenu->lastOn = itemOn;
    M_ClearMenus ();
    D_StartTitle ();
}

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
    {
	S_StartSound(NULL,sfx_oof);
	return;
    }

    if (netgame)
    {
	M_StartMessage(DEH_String(NETEND),NULL,false);
	return;
    }

    M_StartMessage(DEH_String(ENDGAME),M_EndGameResponse,true);
}




//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}




//
// M_QuitDOOM
//
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};



void M_QuitResponse(int key)
{
    extern int show_endoom;

    if (key != key_menu_confirm)
	return;
    // [crispy] play quit sound only if the ENDOOM screen is also shown
    if (!netgame && show_endoom)
    {
	if (gamemode == commercial)
	    S_StartSound(NULL,quitsounds2[(gametic>>2)&7]);
	else
	    S_StartSound(NULL,quitsounds[(gametic>>2)&7]);
	I_WaitVBL(105);
    }
    I_Quit ();
}


static const char *M_SelectEndMessage(void)
{
    const char **endmsg;

    if (logical_gamemission == doom)
    {
        // Doom 1

        endmsg = doom1_endmsg;
    }
    else
    {
        // Doom 2

        endmsg = doom2_endmsg;
    }

    return endmsg[gametic % NUM_QUITMESSAGES];
}


void M_QuitDOOM(int choice)
{
    // [crispy] fast exit if "run" key is held down
    if (speedkeydown())
	I_Quit();

    DEH_snprintf(endstring, sizeof(endstring), "%s\n\n" DOSY,
                 DEH_String(M_SelectEndMessage()));

    M_StartMessage(endstring,M_QuitResponse,true);
}




void M_ChangeSensitivity(int choice)
{
    switch(choice)
    {
      case 0:
	if (mouseSensitivity)
	    mouseSensitivity--;
	break;
      case 1:
	if (mouseSensitivity < 255) // [crispy] extended range
	    mouseSensitivity++;
	break;
    }
}

void M_ChangeSensitivity_x2(int choice)
{
    switch(choice)
    {
      case 0:
	if (mouseSensitivity_x2)
	    mouseSensitivity_x2--;
	break;
      case 1:
	if (mouseSensitivity_x2 < 255) // [crispy] extended range
	    mouseSensitivity_x2++;
	break;
    }
}

static void M_ChangeSensitivity_y(int choice)
{
    switch(choice)
    {
      case 0:
	if (mouseSensitivity_y)
	    mouseSensitivity_y--;
	break;
      case 1:
	if (mouseSensitivity_y < 255) // [crispy] extended range
	    mouseSensitivity_y++;
	break;
    }
}

static void M_MouseInvert(int choice)
{
    choice = 0;
    mouse_y_invert = !mouse_y_invert;
}


void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    R_SetViewSize (screenblocks, detailLevel);

    if (!detailLevel)
	players[consoleplayer].message = DEH_String(DETAILHI);
    else
	players[consoleplayer].message = DEH_String(DETAILLO);
}




void M_SizeDisplay(int choice)
{
    switch(choice)
    {
      case 0:
	if (screenSize > 0)
	{
	    screenblocks--;
	    screenSize--;
	}
	break;
      case 1:
	if (screenSize < 8 + (crispy->widescreen ? 6 : 3)) // [crispy] Crispy HUD
	{
	    screenblocks++;
	    screenSize++;
	}
	// [crispy] reset to fullscreen HUD
	else
	{
	    screenblocks = 11;
	    screenSize = 8;
	}
	break;
    }


    R_SetViewSize (screenblocks, detailLevel);
}

/////////////////////////////////////////////////////////////////////////////
//
// SETUP MENU (phares)
//
// We've added a set of Setup Screens from which you can configure a number
// of variables w/o having to restart the game. There are 7 screens:
//
//    Key Bindings
//    Weapons
//    Status Bar / HUD
//    Automap
//    Enemies
//    Messages
//    Chat Strings
//
// killough 10/98: added Compatibility and General menus
//

/////////////////////////////
//
// booleans for setup screens
// these tell you what state the setup screens are in, and whether any of
// the overlay screens (automap colors, reset button message) should be
// displayed

boolean setup_active      = false; // in one of the setup screens
boolean set_keybnd_active = false; // in key binding setup screens
boolean set_weapon_active = false; // in weapons setup screen
boolean set_status_active = false; // in status bar/hud setup screen
boolean set_auto_active   = false; // in automap setup screen
boolean set_enemy_active  = false; // in enemies setup screen
boolean set_mess_active   = false; // in messages setup screen
boolean set_chat_active   = false; // in chat string setup screen
boolean setup_select      = false; // changing an item
boolean setup_gather      = false; // gathering keys for value
boolean colorbox_active   = false; // color palette being shown
boolean default_verify    = false; // verify reset defaults decision
boolean set_general_active = false;
boolean set_compat_active = false;

/////////////////////////////
//
// set_menu_itemon is an index that starts at zero, and tells you which
// item on the current screen the cursor is sitting on.
//
// current_setup_menu is a pointer to the current setup menu table.

static int set_menu_itemon; // which setup item is selected?   // phares 3/98
setup_menu_t* current_setup_menu; // points to current setup menu table

/////////////////////////////
//
// The menu_buffer is used to construct strings for display on the screen.

static char menu_buffer[66];

/////////////////////////////
//
// The setup_e enum is used to provide a unique number for each group of Setup
// Screens.

enum
{
  set_compat,
  set_key_bindings,
  set_weapons,
  set_statbar,
  set_automap,
  set_enemy,
  set_messages,
  set_chatstrings,
  set_setup_end
} setup_e;

int setup_screen; // the current setup screen. takes values from setup_e

/////////////////////////////
//
// SetupMenu is the definition of what the main Setup Screen should look
// like. Each entry shows that the cursor can land on each item (1), the
// built-in graphic lump (i.e. "M_KEYBND") that should be displayed,
// the program which takes over when an item is selected, and the hotkey
// associated with the item.

menuitem_t SetupMenu[]=
{
  // [FG] alternative text for missing menu graphics lumps
  {1,"M_COMPAT",M_Compat,     'p', "DOOM COMPATIBILITY"},
  {1,"M_KEYBND",M_KeyBindings,'k', "KEY BINDINGS"},
  {1,"M_WEAP"  ,M_Weapons,    'w', "WEAPONS"},
  {1,"M_STAT"  ,M_StatusBar,  's', "STATUS BAR / HUD"},
  {1,"M_AUTO"  ,M_Automap,    'a', "AUTOMAP"},
  {1,"M_ENEM"  ,M_Enemy,      'e', "ENEMIES"},
  {1,"M_MESS"  ,M_Messages,   'm', "MESSAGES"},
  {1,"M_CHAT"  ,M_ChatStrings,'c', "CHAT STRINGS"},
};

/////////////////////////////
//
// M_DoNothing does just that: nothing. Just a placeholder.

void M_DoNothing(int choice)
{
}

/////////////////////////////
//
// Items needed to satisfy the 'Big Font' menu structures:
//
// the generic_setup_e enum mimics the 'Big Font' menu structures, but
// means nothing to the Setup Menus.

enum
{
  generic_setupempty1,
  generic_setup_end
} generic_setup_e;

// Generic_Setup is a do-nothing definition that the mainstream Menu code
// can understand, while the Setup Menu code is working. Another placeholder.

menuitem_t Generic_Setup[] =
{
  {1,"",M_DoNothing,0}
};

/////////////////////////////
//
// SetupDef is the menu definition that the mainstream Menu code understands.
// This is used by M_Setup (below) to define what is drawn and what is done
// with the main Setup screen.

menu_t  SetupDef =
{
  set_setup_end, // number of Setup Menu items (Key Bindings, etc.)
  &OptionsDef,   // menu to return to when BACKSPACE is hit on this menu
  SetupMenu,     // definition of items to show on the Setup Screen
  M_DrawSetup,   // program that draws the Setup Screen
  59,37,         // x,y position of the skull (modified when the skull is
                 // drawn). The skull is parked on the upper-left corner
                 // of the Setup screens, since it isn't needed as a cursor
  0              // last item the user was on for this menu
};

/////////////////////////////
//
// Here are the definitions of the individual Setup Menu screens. They
// follow the format of the 'Big Font' menu structures. See the comments
// for SetupDef (above) to help understand what each of these says.

menu_t KeybndDef =
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawKeybnd,
  34,5,      // skull drawn here
  0
};

menu_t WeaponDef =
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawWeapons,
  34,5,      // skull drawn here
  0
};

menu_t StatusHUDDef =
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawStatusHUD,
  34,5,      // skull drawn here
  0
};

menu_t AutoMapDef =
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawAutoMap,
  34,5,      // skull drawn here
  0
};

menu_t EnemyDef =                                           // phares 4/08/98
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawEnemy,
  34,5,      // skull drawn here
  0
};

menu_t MessageDef =                                         // phares 4/08/98
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawMessages,
  34,5,      // skull drawn here
  0
};

menu_t ChatStrDef =                                         // phares 4/10/98
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawChatStrings,
  34,5,      // skull drawn here
  0
};

menu_t GeneralDef =                                           // killough 10/98
{
  generic_setup_end,
  &OptionsDef,
  Generic_Setup,
  M_DrawGeneral,
  34,5,      // skull drawn here
  0
};

menu_t CompatDef =                                           // killough 10/98
{
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawCompat,
  34,5,      // skull drawn here
  0
};

/////////////////////////////
//
// M_DrawBackground tiles a 64x64 patch over the entire screen, providing the
// background for the Help and Setup screens.
//
// killough 11/98: rewritten to support hires

void M_DrawBackground(char* patchname, byte *back_dest)
{
  int x,y;
  byte *back_src, *src;

  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

  src = back_src =
    W_CacheLumpNum(firstflat+R_FlatNumForName(patchname),PU_CACHE);

  if (hires)       // killough 11/98: hires support
#if 0              // this tiles it in hires
    for (y = 0 ; y < SCREENHEIGHT*2 ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH*2/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
#else              // while this pixel-doubles it
/*
      for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src,
	     back_dest += SCREENWIDTH*2)
	for (x = 0 ; x < SCREENWIDTH/64 ; x++)
	  {
	    int i = 63;
	    do
	      back_dest[i*2] = back_dest[i*2+SCREENWIDTH*2] =
		back_dest[i*2+1] = back_dest[i*2+SCREENWIDTH*2+1] = src[i];
	    while (--i>=0);
	    back_dest += 128;
	  }
*/
    for (int y = 0; y < SCREENHEIGHT<<1; y++)
      for (int x = 0; x < SCREENWIDTH<<1; x += 2)
      {
          const byte dot = src[(((y>>1)&63)<<6) + ((x>>1)&63)];

          *back_dest++ = dot;
          *back_dest++ = dot;
      }
#endif
  else
/*
    for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
*/
    for (y = 0; y < SCREENHEIGHT; y++)
      for (x = 0; x < SCREENWIDTH; x++)
      {
        *back_dest++ = src[((y&63)<<6) + (x&63)];
      }
}

/////////////////////////////
//
// Draws the Title for the main Setup screen

void M_DrawSetup(void)
{
  M_DrawTitle(124,15,"M_SETUP","SETUP");
}

/////////////////////////////
//
// Uses the SetupDef structure to draw the menu items for the main
// Setup screen

void M_Setup(int choice)
{
  M_SetupNextMenu(&SetupDef);
}

/////////////////////////////
//
// Data that's used by the Setup screen code
//
// Establish the message colors to be used

#define CR_TITLE  CR_GOLD
#define CR_SET    CR_GREEN
#define CR_ITEM   CR_RED
#define CR_HILITE CR_ORANGE
#define CR_SELECT CR_GRAY

// Data used by the Automap color selection code

#define CHIP_SIZE 7 // size of color block for colored items

#define COLORPALXORIG ((320 - 16*(CHIP_SIZE+1))/2)
#define COLORPALYORIG ((200 - 16*(CHIP_SIZE+1))/2)

#define PAL_BLACK   0
#define PAL_WHITE   4

static byte colorblock[(CHIP_SIZE+4)*(CHIP_SIZE+4)];

// Data used by the Chat String editing code

#define CHAT_STRING_BFR_SIZE 128

// chat strings must fit in this screen space
// killough 10/98: reduced, for more general uses
#define MAXCHATWIDTH         272

int   chat_index;
char* chat_string_buffer; // points to new chat strings while editing

/////////////////////////////
//
// phares 4/17/98:
// Added 'Reset to Defaults' Button to Setup Menu screens
// This is a small button that sits in the upper-right-hand corner of
// the first screen for each group. It blinks when selected, thus the
// two patches, which it toggles back and forth.

char ResetButtonName[2][8] = {"M_BUTT1","M_BUTT2"};

/////////////////////////////
//
// phares 4/18/98:
// Consolidate Item drawing code
//
// M_DrawItem draws the description of the provided item (the left-hand
// part). A different color is used for the text depending on whether the
// item is selected or not, or whether it's about to change.

void M_DrawStringDisable(int cx, int cy, const char* ch);

void M_DrawItem(setup_menu_t* s)
{
  int x = s->m_x;
  int y = s->m_y;
  int flags = s->m_flags;
  if (flags & S_RESET)

    // This item is the reset button
    // Draw the 'off' version if this isn't the current menu item
    // Draw the blinking version in tune with the blinking skull otherwise

    V_DrawPatchDirect(x,y,0,W_CacheLumpName(ResetButtonName
					    [flags & (S_HILITE|S_SELECT) ?
					    whichSkull : 0], PU_CACHE));
  else  // Draw the item string
    {
      char *p, *t;
      int w = 0;
      int color =
	flags & S_SELECT ? CR_SELECT :
	flags & S_HILITE ? CR_HILITE :
	flags & (S_TITLE|S_NEXT|S_PREV) ? CR_TITLE : CR_ITEM; // killough 10/98

      // killough 10/98:
      // Enhance to support multiline text separated by newlines.
      // This supports multiline items on horizontally-crowded menus.

      for (p = t = strdup(s->m_text); (p = strtok(p,"\n")); y += 8, p = NULL)
	{      // killough 10/98: support left-justification:
	  strcpy(menu_buffer,p);
	  if (!(flags & S_LEFTJUST))
	    w = M_GetPixelWidth(menu_buffer) + 4;
	  if (flags & S_DISABLE)
	    M_DrawStringDisable(x - w, y, menu_buffer);
	  else
	  M_DrawMenuString(x - w, y ,color);
	  // [FG] print a blinking "arrow" next to the currently highlighted menu item
	  if (s == current_setup_menu + set_menu_itemon && whichSkull)
	  {
	    if (flags & S_DISABLE)
	      M_DrawStringDisable(x - w - 8, y, ">");
	    else
	    M_DrawString(x - w - 8, y, color, ">");
	  }
	}
      free(t);
    }
}

// If a number item is being changed, allow up to N keystrokes to 'gather'
// the value. Gather_count tells you how many you have so far. The legality
// of what is gathered is determined by the low/high settings for the item.

#define MAXGATHER 5
int  gather_count;
char gather_buffer[MAXGATHER+1];  // killough 10/98: make input character-based

/////////////////////////////
//
// phares 4/18/98:
// Consolidate Item Setting drawing code
//
// M_DrawSetting draws the setting of the provided item (the right-hand
// part. It determines the text color based on whether the item is
// selected or being changed. Then, depending on the type of item, it
// displays the appropriate setting value: yes/no, a key binding, a number,
// a paint chip, etc.

void M_DrawSetting(setup_menu_t* s)
{
  int x = s->m_x, y = s->m_y, flags = s->m_flags, color;

  // Determine color of the text. This may or may not be used
  // later, depending on whether the item is a text string or not.

  color = flags & S_SELECT ? CR_SELECT : flags & S_HILITE ? CR_HILITE : CR_SET;

  // Is the item a YES/NO item?

  if (flags & S_YESNO)
    {
      strcpy(menu_buffer,s->var.def->location->i ? "YES" : "NO");
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
        strcat(menu_buffer, " <");
      if (flags & S_DISABLE)
        M_DrawStringDisable(x,y,menu_buffer);
      else
      M_DrawMenuString(x,y,color);
      return;
    }

  // Is the item a simple number?

  if (flags & S_NUM)
    {
      // killough 10/98: We must draw differently for items being gathered.
      if (flags & (S_HILITE|S_SELECT) && setup_gather)
	{
	  gather_buffer[gather_count] = 0;
	  strcpy(menu_buffer, gather_buffer);
	}
      else
	sprintf(menu_buffer,"%d",s->var.def->location->i);
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
        strcat(menu_buffer, " <");
      if (flags & S_DISABLE)
        M_DrawStringDisable(x,y,menu_buffer);
      else
      M_DrawMenuString(x,y,color);
      return;
    }

  // Is the item a key binding?

  if (flags & S_KEY) // Key Binding
    {
      int *key = s->var.m_key;

      // Draw the key bound to the action

      if (key)
	{
	  M_GetKeyString(*key,0); // string to display
	  if (key == &key_use && dclick_use && mousebuse == -1)
	    {
	      // For the 'use' key, you have to build the string

	      if (s->m_mouse)
		sprintf(menu_buffer+strlen(menu_buffer), "/DBL-CLK MB%d",
			mousebforward>-1?mousebforward+1:mousebstrafe+1);
	      if (s->m_joy)
		sprintf(menu_buffer+strlen(menu_buffer), "/JSB%d",
			*s->m_joy+1);
	    }
	  else
	    if (key == &key_up   || key == &key_speed ||
		key == &key_fire || key == &key_strafe ||
		// [FG] support more joystick and mouse buttons
		s->m_mouse || s->m_joy)
	      {
		if (s->m_mouse)
		  sprintf(menu_buffer+strlen(menu_buffer), "/MB%d",
			  *s->m_mouse+1);
		if (s->m_joy)
		  sprintf(menu_buffer+strlen(menu_buffer), "/JSB%d",
			  *s->m_joy+1);
	      }
	  // [FG] print a blinking "arrow" next to the currently highlighted menu item
	  if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
	    strcat(menu_buffer, " <");
	  M_DrawMenuString(x,y,color);
	}
      return;
    }

  // Is the item a weapon number?
  // OR, Is the item a colored text string from the Automap?
  //
  // killough 10/98: removed special code, since the rest of the engine
  // already takes care of it, and this code prevented the user from setting
  // their overall weapons preferences while playing Doom 1.
  //
  // killough 11/98: consolidated weapons code with color range code

  if (flags & (S_WEAP|S_CRITEM)) // weapon number or color range
    {
      sprintf(menu_buffer,"%d", s->var.def->location->i);
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
      {
        if (flags & S_DISABLE)
          M_DrawStringDisable(x + 8, y, " <");
        else
        M_DrawString(x + 8, y, color, " <");
      }
      if (flags & S_DISABLE)
        M_DrawStringDisable(x, y, menu_buffer);
      else
      M_DrawMenuString(x,y, flags & S_CRITEM ? s->var.def->location->i : color);
      return;
    }

  // Is the item a paint chip?

  if (flags & S_COLOR) // Automap paint chip
    {
      int i, ch;
      byte *ptr = colorblock;

      // draw the border of the paint chip

      for (i = 0 ; i < (CHIP_SIZE+2)*(CHIP_SIZE+2) ; i++)
	*ptr++ = PAL_BLACK;
      V_DrawBlock(x+WIDESCREENDELTA,y-1,0,CHIP_SIZE+2,CHIP_SIZE+2,colorblock);

      // draw the paint chip

      ch = s->var.def->location->i;
      if (!ch) // don't show this item in automap mode
	V_DrawPatchDirect (x+1,y,0,W_CacheLumpName("M_PALNO",PU_CACHE));
      else
	{
	  ptr = colorblock;
	  for (i = 0 ; i < CHIP_SIZE*CHIP_SIZE ; i++)
	    *ptr++ = ch;
	  V_DrawBlock(x+1+WIDESCREENDELTA,y,0,CHIP_SIZE,CHIP_SIZE,colorblock);
	}
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
        M_DrawString(x + 8, y, color, " <");
      return;
    }

  // Is the item a chat string?
  // killough 10/98: or a filename?

  if (flags & S_STRING)
    {
      char *text = s->var.def->location->s;

      // Are we editing this string? If so, display a cursor under
      // the correct character.

      if (setup_select && (s->m_flags & (S_HILITE|S_SELECT)))
	{
	  int cursor_start, char_width, i;
	  char c[2];

	  // If the string is too wide for the screen, trim it back,
	  // one char at a time until it fits. This should only occur
	  // while you're editing the string.

	  while (M_GetPixelWidth(text) >= MAXCHATWIDTH)
	    {
	      int len = strlen(text);
	      text[--len] = 0;
	      if (chat_index > len)
		chat_index--;
	    }

	  // Find the distance from the beginning of the string to
	  // where the cursor should be drawn, plus the width of
	  // the char the cursor is under..

	  *c = text[chat_index]; // hold temporarily
	  c[1] = 0;
	  char_width = M_GetPixelWidth(c);
	  if (char_width == 1)
	    char_width = 7; // default for end of line
	  text[chat_index] = 0; // NULL to get cursor position
	  cursor_start = M_GetPixelWidth(text);
	  text[chat_index] = *c; // replace stored char

	  // Now draw the cursor

	  for (i = 0 ; i < char_width ; i++)
	    colorblock[i] = PAL_BLACK;
	  V_DrawBlock(x+cursor_start-1+WIDESCREENDELTA,y+7,0,char_width,1,colorblock);
	}

      // Draw the setting for the item

      strcpy(menu_buffer,text);
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
        strcat(menu_buffer, " <");
      M_DrawMenuString(x,y,color);
      return;
    }

  // [FG] selection of choices

  if (flags & S_CHOICE)
    {
      int i = s->var.def->location->i;
      if (i >= 0 && s->selectstrings[i])
        strcpy(menu_buffer, s->selectstrings[i]);
      // [FG] print a blinking "arrow" next to the currently highlighted menu item
      if (s == current_setup_menu + set_menu_itemon && whichSkull && !setup_select)
        strcat(menu_buffer, " <");
      M_DrawMenuString(x,y,color);
      return;
    }
}

/////////////////////////////
//
// M_DrawScreenItems takes the data for each menu item and gives it to
// the drawing routines above.

void M_DrawScreenItems(setup_menu_t* src)
{
  if (print_warning_about_changes > 0)   // killough 8/15/98: print warning
  {
    if (warning_about_changes & S_BADVAL)
      {
	strcpy(menu_buffer, "Value out of Range");
	M_DrawMenuString(100,176,CR_RED);
      }
    else
      if (warning_about_changes & S_PRGWARN)
	{
	  strcpy(menu_buffer,
		 "Warning: Program must be restarted to see changes");
	  M_DrawMenuString(3, 176, CR_RED);
	}
      else
	if (warning_about_changes & S_BADVID)
	  {
	    strcpy(menu_buffer, "Video mode not supported");
	    M_DrawMenuString(80,176,CR_RED);
	  }
	else
	  {
	    strcpy(menu_buffer,
		   "Warning: Changes are pending until next game");
	    M_DrawMenuString(18,184,CR_RED);
	  }
  }

  while (!(src->m_flags & S_END))
    {
      // See if we're to draw the item description (left-hand part)

      if (src->m_flags & S_SHOWDESC)
	M_DrawItem(src);

      // See if we're to draw the setting (right-hand part)

      if (src->m_flags & S_SHOWSET)
	M_DrawSetting(src);
      src++;
    }
}

/////////////////////////////
//
// Data used to draw the "are you sure?" dialogue box when resetting
// to defaults.

#define VERIFYBOXXORG (66+WIDESCREENDELTA)
#define VERIFYBOXYORG 88
#define PAL_GRAY1  91
#define PAL_GRAY2  98
#define PAL_GRAY3 105

// And the routine to draw it.

static void M_DrawVerifyBox()
{
  byte block[188];
  int i;

  for (i = 0 ; i < 181 ; i++)
    block[i] = PAL_BLACK;
  for (i = 0 ; i < 17 ; i++)
    V_DrawBlock(VERIFYBOXXORG+3,VERIFYBOXYORG+3+i,0,181,1,block);
  for (i = 0 ; i < 187 ; i++)
    block[i] = PAL_GRAY1;
  V_DrawBlock(VERIFYBOXXORG,VERIFYBOXYORG,0,187,1,block);
  V_DrawBlock(VERIFYBOXXORG,VERIFYBOXYORG,0,1,23,block);
  V_DrawBlock(VERIFYBOXXORG+184,VERIFYBOXYORG+2,0,1,19,block);
  V_DrawBlock(VERIFYBOXXORG+2,VERIFYBOXYORG+20,0,183,1,block);
  for (i = 0 ; i < 185 ; i++)
    block[i] = PAL_GRAY2;
  V_DrawBlock(VERIFYBOXXORG+1,VERIFYBOXYORG+1,0,185,1,block);
  V_DrawBlock(VERIFYBOXXORG+185,VERIFYBOXYORG+1,0,1,21,block);
  V_DrawBlock(VERIFYBOXXORG+1,VERIFYBOXYORG+21,0,185,1,block);
  V_DrawBlock(VERIFYBOXXORG+1,VERIFYBOXYORG+1,0,1,21,block);
  for (i = 0 ; i < 187 ; i++)
    block[i] = PAL_GRAY3;
  V_DrawBlock(VERIFYBOXXORG+2,VERIFYBOXYORG+2,0,183,1,block);
  V_DrawBlock(VERIFYBOXXORG+2,VERIFYBOXYORG+2,0,1,19,block);
  V_DrawBlock(VERIFYBOXXORG+186,VERIFYBOXYORG,0,1,23,block);
  V_DrawBlock(VERIFYBOXXORG,VERIFYBOXYORG+22,0,187,1,block);
}

void M_DrawDefVerify()
{
  M_DrawVerifyBox();

  // The blinking messages is keyed off of the blinking of the
  // cursor skull.

  if (whichSkull) // blink the text
    {
      strcpy(menu_buffer,"Reset to defaults? (Y or N)");
      M_DrawMenuString(VERIFYBOXXORG+8-WIDESCREENDELTA,VERIFYBOXYORG+8,CR_RED);
    }
}

// [FG] delete a savegame

static void M_DrawDelVerify(void)
{
  M_DrawVerifyBox();

  if (whichSkull) {
    strcpy(menu_buffer,"Delete savegame? (Y or N)");
    M_DrawMenuString(VERIFYBOXXORG+8-WIDESCREENDELTA,VERIFYBOXYORG+8,CR_RED);
  }
}

/////////////////////////////
//
// phares 4/18/98:
// M_DrawInstructions writes the instruction text just below the screen title
//
// killough 8/15/98: rewritten

void M_DrawInstructions()
{
  default_t *def = current_setup_menu[set_menu_itemon].var.def;
  int flags = current_setup_menu[set_menu_itemon].m_flags;

  if (flags & S_DISABLE)
    return;

  // killough 8/15/98: warn when values are different
  if (flags & (S_NUM|S_YESNO) && def->current && def->current->i!=def->location->i)
    {
      int allow = allow_changes() ? 8 : 0;
      if (!(setup_gather | print_warning_about_changes | demoplayback))
	{
	  strcpy(menu_buffer,
		 "Warning: Current actual setting differs from the");
	  M_DrawMenuString(4, 184-allow, CR_RED);
	  strcpy(menu_buffer,
		 "default setting, which is the one being edited here.");
	  M_DrawMenuString(4, 192-allow, CR_RED);
	  if (allow)
	    {
	      strcpy(menu_buffer,
		     "However, changes made here will take effect now.");
	      M_DrawMenuString(4, 192, CR_RED);
	    }
	}
      if (allow && setup_select)            // killough 8/15/98: Set new value
	if (!(flags & (S_LEVWARN | S_PRGWARN)))
	  def->current->i = def->location->i;
    }

  // There are different instruction messages depending on whether you
  // are changing an item or just sitting on it.

  { // killough 11/98: reformatted
    const char *s = "";
    int color = CR_HILITE,x = setup_select ? color = CR_SELECT, flags & S_KEY ?
      current_setup_menu[set_menu_itemon].m_mouse ||
      current_setup_menu[set_menu_itemon].m_joy ?
      (s = "Press key or button for this action", 49)                        :
      (s = "Press key for this action", 84)                                  :
      flags & S_YESNO  ? (s = "Press ENTER key to toggle", 78)               :
      // [FG] selection of choices
      flags & S_CHOICE ? (s = "Press left or right to choose", 70)           :
      flags & S_WEAP   ? (s = "Enter weapon number", 97)                     :
      flags & S_NUM    ? (s = "Enter value. Press ENTER when finished.", 37) :
      flags & S_COLOR  ? (s = "Select color and press enter", 70)            :
      flags & S_CRITEM ? (s = "Enter value", 125)                            :
      flags & S_CHAT   ? (s = "Type/edit chat string and Press ENTER", 43)   :
      flags & S_FILE   ? (s = "Type/edit filename and Press ENTER", 52)      :
      flags & S_RESET  ? 43 : 0  /* when you're changing something */        :
      flags & S_RESET  ? (s = "Press ENTER key to reset to defaults", 43)    :
      // [FG] clear key bindings with the DEL key
      flags & S_KEY    ? (s = "Press Enter to Change, Del to Clear", 43)     :
      (s = "Press Enter to Change", 91);
    strcpy(menu_buffer, s);
    M_DrawMenuString(x,20,color);
  }
}




//
//      Menu Functions
//
void
M_DrawThermo
( int	x,
  int	y,
  int	thermWidth,
  int	thermDot )
{
    int		xx;
    int		i;
    char	num[4];

    if (!thermDot)
    {
        dp_translation = cr[CR_DARK];
    }

    xx = x;
    V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERML"), PU_CACHE));
    xx += 8;
    for (i=0;i<thermWidth;i++)
    {
	V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERMM"), PU_CACHE));
	xx += 8;
    }
    V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERMR"), PU_CACHE));

    M_snprintf(num, 4, "%3d", thermDot);
    M_WriteText(xx + 8, y + 3, num);

    // [crispy] do not crash anymore if value exceeds thermometer range
    if (thermDot >= thermWidth)
    {
        thermDot = thermWidth - 1;
        dp_translation = cr[CR_DARK];
    }

    V_DrawPatchDirect((x + 8) + thermDot * 8, y,
		      W_CacheLumpName(DEH_String("M_THERMO"), PU_CACHE));

    dp_translation = NULL;
}


void
M_StartMessage
( const char	*string,
  void*		routine,
  boolean	input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
    // [crispy] entering menus while recording demos pauses the game
    if (demorecording && !paused)
    {
        sendpause = true;
    }
    return;
}


//
// Find string width from hu_font chars
//
int M_StringWidth(const char *string)
{
    size_t             i;
    int             w = 0;
    int             c;

    for (i = 0;i < strlen(string);i++)
    {
	// [crispy] correctly center colorized strings
	if (string[i] == cr_esc)
	{
	    if (string[i+1] >= '0' && string[i+1] <= '0' + CRMAX - 1)
	    {
		i++;
		continue;
	    }
	}

	c = toupper(string[i]) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE)
	    w += 4;
	else
	    w += SHORT (hu_font[c]->width);
    }

    return w;
}



//
//      Find string height from hu_font chars
//
int M_StringHeight(const char* string)
{
    size_t             i;
    int             h;
    int             height = SHORT(hu_font[0]->height);

    h = height;
    for (i = 0;i < strlen(string);i++)
	if (string[i] == '\n')
	    h += height;

    return h;
}


//
//      Write a string using the hu_font
//
void
M_WriteText
( int		x,
  int		y,
  const char *string)
{
    int		w;
    const char *ch;
    int		c;
    int		cx;
    int		cy;


    ch = string;
    cx = x;
    cy = y;

    while(1)
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = x;
	    cy += 12;
	    continue;
	}
	// [crispy] support multi-colored text
	if (c == cr_esc)
	{
	    if (*ch >= '0' && *ch <= '0' + CRMAX - 1)
	    {
		c = *ch++;
		dp_translation = cr[(int) (c - '0')];
		continue;
	    }
	}

	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c>= HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	if (cx+w > ORIGWIDTH)
	    break;
	V_DrawPatchDirect(cx, cy, hu_font[c]);
	cx+=w;
    }
}

// These keys evaluate to a "null" key in Vanilla Doom that allows weird
// jumping in the menus. Preserve this behavior for accuracy.

static boolean IsNullKey(int key)
{
    return key == KEY_PAUSE || key == KEY_CAPSLOCK
        || key == KEY_SCRLCK || key == KEY_NUMLOCK;
}

// [crispy] reload current level / go to next level
// adapted from prboom-plus/src/e6y.c:369-449
static int G_ReloadLevel(void)
{
  int result = false;

  if (gamestate == GS_LEVEL)
  {
    // [crispy] restart demos from the map they were started
    if (demorecording)
    {
      gamemap = startmap;
    }
    G_DeferedInitNew(gameskill, gameepisode, gamemap);
    result = true;
  }

  return result;
}

static int G_GotoNextLevel(void)
{
  byte doom_next[5][9] = {
    {12, 13, 19, 15, 16, 17, 18, 21, 14},
    {22, 23, 24, 25, 29, 27, 28, 31, 26},
    {32, 33, 34, 35, 36, 39, 38, 41, 37},
    {42, 49, 44, 45, 46, 47, 48, 51, 43},
    {52, 53, 54, 55, 56, 59, 58, 11, 57},
  };
  byte doom2_next[33] = {
     2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
    12, 13, 14, 15, 31, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 1,
    32, 16, 3
  };
  byte nerve_next[9] = {
    2, 3, 4, 9, 6, 7, 8, 1, 5
  };

  int changed = false;

    if (gamemode == commercial)
    {
      if (crispy->havemap33)
        doom2_next[1] = 33;

      if (W_CheckNumForName("map31") < 0)
        doom2_next[14] = 16;

      if (gamemission == pack_hacx)
      {
        doom2_next[30] = 16;
        doom2_next[20] = 1;
      }

      if (gamemission == pack_master)
      {
        doom2_next[1] = 3;
        doom2_next[14] = 16;
        doom2_next[20] = 1;
      }
    }
    else
    {
      if (gamemode == shareware)
        doom_next[0][7] = 11;

      if (gamemode == registered)
        doom_next[2][7] = 11;

      if (!crispy->haved1e5)
        doom_next[3][7] = 11;

      if (gameversion == exe_chex)
      {
        doom_next[0][2] = 14;
        doom_next[0][4] = 11;
      }
    }

  if (gamestate == GS_LEVEL)
  {
    int epsd, map;

    if (gamemode == commercial)
    {
      epsd = gameepisode;
      if (gamemission == pack_nerve)
        map = nerve_next[gamemap-1];
      else
        map = doom2_next[gamemap-1];
    }
    else
    {
      epsd = doom_next[gameepisode-1][gamemap-1] / 10;
      map = doom_next[gameepisode-1][gamemap-1] % 10;
    }

    // [crispy] special-casing for E1M10 "Sewers" support
    if (crispy->havee1m10 && gameepisode == 1)
    {
	if (gamemap == 1)
	{
	    map = 10;
	}
	else
	if (gamemap == 10)
	{
	    epsd = 1;
	    map = 2;
	}
    }

    G_DeferedInitNew(gameskill, epsd, map);
    changed = true;
  }

  return changed;
}

/////////////////////////////
//
// The Key Binding Screen tables.

#define KB_X  160
#define KB_PREV  57
#define KB_NEXT 310
#define KB_Y   31

// phares 4/16/98:
// X,Y position of reset button. This is the same for every screen, and is
// only defined once here.

#define X_BUTTON 301
#define Y_BUTTON   3

// Definitions of the (in this case) five key binding screens.

setup_menu_t keys_settings1[];
setup_menu_t keys_settings2[];
setup_menu_t keys_settings3[];
setup_menu_t keys_settings4[];
setup_menu_t keys_settings5[];

// The table which gets you from one screen table to the next.

setup_menu_t* keys_settings[] =
{
  keys_settings1,
  keys_settings2,
  keys_settings3,
  keys_settings4,
  keys_settings5,
  NULL
};

int mult_screens_index; // the index of the current screen in a set

// Here's an example from this first screen, with explanations.
//
//  {
//  "STRAFE",      // The description of the item ('strafe' key)
//  S_KEY,         // This is a key binding item
//  m_scrn,        // It belongs to the m_scrn group. Its key cannot be
//                 // bound to two items in this group.
//  KB_X,          // The X offset of the start of the right-hand side
//  KB_Y+ 8*8,     // The Y offset of the start of the right-hand side.
//                 // Always given in multiples off a baseline.
//  &key_strafe,   // The variable that holds the key value bound to this
//                    OR a string that holds the config variable name.
//                    OR a pointer to another setup_menu
//  &mousebstrafe, // The variable that holds the mouse button bound to
                   // this. If zero, no mouse button can be bound here.
//  &joybstrafe,   // The variable that holds the joystick button bound to
                   // this. If zero, no mouse button can be bound here.
//  }

// The first Key Binding screen table.
// Note that the Y values are ascending. If you need to add something to
// this table, (well, this one's not a good example, because it's full)
// you need to make sure the Y values still make sense so everything gets
// displayed.
//
// Note also that the first screen of each set has a line for the reset
// button. If there is more than one screen in a set, the others don't get
// the reset button.
//
// Note also that this screen has a "NEXT ->" line. This acts like an
// item, in that 'activating' it moves you along to the next screen. If
// there's a "<- PREV" item on a screen, it behaves similarly, moving you
// to the previous screen. If you leave these off, you can't move from
// screen to screen.

setup_menu_t keys_settings1[] =  // Key Binding screen strings
{
  {"MOVEMENT"    ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y},
  {"FORWARD"     ,S_KEY       ,m_scrn,KB_X,KB_Y+1*8,{&key_up},&mousebforward},
  {"BACKWARD"    ,S_KEY       ,m_scrn,KB_X,KB_Y+2*8,{&key_down},&mousebbackward},
  {"TURN LEFT"   ,S_KEY       ,m_scrn,KB_X,KB_Y+3*8,{&key_left},&mousebturnleft},
  {"TURN RIGHT"  ,S_KEY       ,m_scrn,KB_X,KB_Y+4*8,{&key_right},&mousebturnright},
  {"RUN"         ,S_KEY       ,m_scrn,KB_X,KB_Y+5*8,{&key_speed},0,&joybspeed},
  {"STRAFE LEFT" ,S_KEY       ,m_scrn,KB_X,KB_Y+6*8,{&key_strafeleft},0,&joybstrafeleft},
  {"STRAFE RIGHT",S_KEY       ,m_scrn,KB_X,KB_Y+7*8,{&key_straferight},0,&joybstraferight},
  {"STRAFE"      ,S_KEY       ,m_scrn,KB_X,KB_Y+8*8,{&key_strafe},&mousebstrafe,&joybstrafe},
  {"AUTORUN"     ,S_KEY       ,m_scrn,KB_X,KB_Y+9*8,{&key_autorun}},
  {"180 TURN"    ,S_KEY       ,m_scrn,KB_X,KB_Y+10*8,{&key_reverse}},
  {"USE"         ,S_KEY       ,m_scrn,KB_X,KB_Y+11*8,{&key_use},&mousebuse,&joybuse},

  {"MENUS"       ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y+12*8},
  {"NEXT ITEM"   ,S_KEY       ,m_menu,KB_X,KB_Y+13*8,{&key_menu_down}},
  {"PREV ITEM"   ,S_KEY       ,m_menu,KB_X,KB_Y+14*8,{&key_menu_up}},
  {"LEFT"        ,S_KEY       ,m_menu,KB_X,KB_Y+15*8,{&key_menu_left}},
  {"RIGHT"       ,S_KEY       ,m_menu,KB_X,KB_Y+16*8,{&key_menu_right}},
  {"BACKSPACE"   ,S_KEY       ,m_menu,KB_X,KB_Y+17*8,{&key_menu_backspace}},
  {"SELECT ITEM" ,S_KEY       ,m_menu,KB_X,KB_Y+18*8,{&key_menu_enter}},
  {"EXIT"        ,S_KEY       ,m_menu,KB_X,KB_Y+19*8,{&key_menu_escape},0,&joybmainmenu},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  {"NEXT ->",S_SKIP|S_NEXT,m_null,KB_NEXT,KB_Y+20*8, {keys_settings2}},

  // Final entry
  {0,S_SKIP|S_END,m_null}

};

setup_menu_t keys_settings2[] =  // Key Binding screen strings
{
  {"SCREEN"      ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y},

  // phares 4/13/98:
  // key_help and key_escape can no longer be rebound. This keeps the
  // player from getting themselves in a bind where they can't remember how
  // to get to the menus, and can't remember how to get to the help screen
  // to give them a clue as to how to get to the menus. :)

  // Also, the keys assigned to these functions cannot be bound to other
  // functions. Introduce an S_KEEP flag to show that you cannot swap this
  // key with other keys in the same 'group'. (m_scrn, etc.)

  {"HELP"        ,S_SKIP|S_KEEP ,m_scrn,0   ,0    ,{&key_help}},
  {"MENU"        ,S_SKIP|S_KEEP ,m_scrn,0   ,0    ,{&key_escape},0,&joybmainmenu},
  // killough 10/98: hotkey for entering setup menu:
  {"SETUP"       ,S_KEY       ,m_scrn,KB_X,KB_Y+ 1*8,{&key_setup}},
  {"PAUSE"       ,S_KEY       ,m_scrn,KB_X,KB_Y+ 2*8,{&key_pause}},
  {"AUTOMAP"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 3*8,{&key_map},0,&joybautomap},
  {"VOLUME"      ,S_KEY       ,m_scrn,KB_X,KB_Y+ 4*8,{&key_soundvolume}},
  {"HUD"         ,S_KEY       ,m_scrn,KB_X,KB_Y+ 5*8,{&key_hud}},
  {"MESSAGES"    ,S_KEY       ,m_scrn,KB_X,KB_Y+ 6*8,{&key_messages}},
  {"GAMMA FIX"   ,S_KEY       ,m_scrn,KB_X,KB_Y+ 7*8,{&key_gamma}},
  {"SPY"         ,S_KEY       ,m_scrn,KB_X,KB_Y+ 8*8,{&key_spy}},
  {"LARGER VIEW" ,S_KEY       ,m_scrn,KB_X,KB_Y+ 9*8,{&key_zoomin}},
  {"SMALLER VIEW",S_KEY       ,m_scrn,KB_X,KB_Y+10*8,{&key_zoomout}},
  {"SCREENSHOT"  ,S_KEY       ,m_scrn,KB_X,KB_Y+11*8,{&key_screenshot}},
  {"GAME"        ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y+12*8},
  {"SAVE"        ,S_KEY       ,m_scrn,KB_X,KB_Y+13*8,{&key_savegame}},
  {"LOAD"        ,S_KEY       ,m_scrn,KB_X,KB_Y+14*8,{&key_loadgame}},
  {"QUICKSAVE"   ,S_KEY       ,m_scrn,KB_X,KB_Y+15*8,{&key_quicksave}},
  {"QUICKLOAD"   ,S_KEY       ,m_scrn,KB_X,KB_Y+16*8,{&key_quickload}},
  {"END GAME"    ,S_KEY       ,m_scrn,KB_X,KB_Y+17*8,{&key_endgame}},
  {"QUIT"        ,S_KEY       ,m_scrn,KB_X,KB_Y+18*8,{&key_quit}},
  {"<- PREV", S_SKIP|S_PREV,m_null,KB_PREV,KB_Y+20*8, {keys_settings1}},
  {"NEXT ->", S_SKIP|S_NEXT,m_null,KB_NEXT,KB_Y+20*8, {keys_settings3}},

  // Final entry

  {0,S_SKIP|S_END,m_null}
};

setup_menu_t keys_settings3[] =  // Key Binding screen strings
{
  {"WEAPONS" ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y},
  {"FIST"    ,S_KEY       ,m_scrn,KB_X,KB_Y+ 1*8,{&key_weapon1}},
  {"PISTOL"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 2*8,{&key_weapon2}},
  {"SHOTGUN" ,S_KEY       ,m_scrn,KB_X,KB_Y+ 3*8,{&key_weapon3}},
  {"CHAINGUN",S_KEY       ,m_scrn,KB_X,KB_Y+ 4*8,{&key_weapon4}},
  {"ROCKET"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 5*8,{&key_weapon5}},
  {"PLASMA"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 6*8,{&key_weapon6}},
  {"BFG",     S_KEY       ,m_scrn,KB_X,KB_Y+ 7*8,{&key_weapon7}},
  {"CHAINSAW",S_KEY       ,m_scrn,KB_X,KB_Y+ 8*8,{&key_weapon8}},
  {"SSG"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 9*8,{&key_weapon9}},
  {"BEST"    ,S_KEY       ,m_scrn,KB_X,KB_Y+10*8,{&key_weapontoggle}},
  {"FIRE"    ,S_KEY       ,m_scrn,KB_X,KB_Y+11*8,{&key_fire},&mousebfire,&joybfire},
  // [FG] prev/next weapon keys and buttons
  {"PREV"    ,S_KEY       ,m_scrn,KB_X,KB_Y+12*8,{&key_prevweapon},&mousebprevweapon,&joybprevweapon},
  {"NEXT"    ,S_KEY       ,m_scrn,KB_X,KB_Y+13*8,{&key_nextweapon},&mousebnextweapon,&joybnextweapon},
  // [FG] reload current level / go to next level
  {"LEVELS"      ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y+14*8},
  {"RELOAD LEVEL",S_KEY   ,m_scrn,KB_X,KB_Y+15*8,{&key_menu_reloadlevel}},
  {"NEXT LEVEL"  ,S_KEY   ,m_scrn,KB_X,KB_Y+16*8,{&key_menu_nextlevel}},

  {"<- PREV",S_SKIP|S_PREV,m_null,KB_PREV,KB_Y+20*8, {keys_settings2}},
  {"NEXT ->",S_SKIP|S_NEXT,m_null,KB_NEXT,KB_Y+20*8, {keys_settings4}},

  // Final entry

  {0,S_SKIP|S_END,m_null}

};

setup_menu_t keys_settings4[] =  // Key Binding screen strings
{
  {"AUTOMAP"    ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y},
  {"FOLLOW"     ,S_KEY       ,m_map ,KB_X,KB_Y+ 1*8,{&key_map_follow}},
  {"ZOOM IN"    ,S_KEY       ,m_map ,KB_X,KB_Y+ 2*8,{&key_map_zoomin}},
  {"ZOOM OUT"   ,S_KEY       ,m_map ,KB_X,KB_Y+ 3*8,{&key_map_zoomout}},
  {"SHIFT UP"   ,S_KEY       ,m_map ,KB_X,KB_Y+ 4*8,{&key_map_up}},
  {"SHIFT DOWN" ,S_KEY       ,m_map ,KB_X,KB_Y+ 5*8,{&key_map_down}},
  {"SHIFT LEFT" ,S_KEY       ,m_map ,KB_X,KB_Y+ 6*8,{&key_map_left}},
  {"SHIFT RIGHT",S_KEY       ,m_map ,KB_X,KB_Y+ 7*8,{&key_map_right}},
  {"MARK PLACE" ,S_KEY       ,m_map ,KB_X,KB_Y+ 8*8,{&key_map_mark}},
  {"CLEAR MARKS",S_KEY       ,m_map ,KB_X,KB_Y+ 9*8,{&key_map_clear}},
  {"FULL/ZOOM"  ,S_KEY       ,m_map ,KB_X,KB_Y+10*8,{&key_map_gobig}},
  {"GRID"       ,S_KEY       ,m_map ,KB_X,KB_Y+11*8,{&key_map_grid}},
  {"OVERLAY"    ,S_KEY       ,m_map ,KB_X,KB_Y+12*8,{&key_map_overlay}},
  {"ROTATE"     ,S_KEY       ,m_map ,KB_X,KB_Y+13*8,{&key_map_rotate}},

  {"<- PREV" ,S_SKIP|S_PREV,m_null,KB_PREV,KB_Y+20*8, {keys_settings3}},
  {"NEXT ->",S_SKIP|S_NEXT,m_null,KB_NEXT,KB_Y+20*8, {keys_settings5}},

  // Final entry

  {0,S_SKIP|S_END,m_null}

};

setup_menu_t keys_settings5[] =
{
  {"CHATTING"   ,S_SKIP|S_TITLE,m_null,KB_X,KB_Y},
  {"BEGIN CHAT" ,S_KEY       ,m_scrn,KB_X,KB_Y+1*8,{&key_chat}},
  {"PLAYER 1"   ,S_KEY       ,m_scrn,KB_X,KB_Y+2*8,{&destination_keys[0]}},
  {"PLAYER 2"   ,S_KEY       ,m_scrn,KB_X,KB_Y+3*8,{&destination_keys[1]}},
  {"PLAYER 3"   ,S_KEY       ,m_scrn,KB_X,KB_Y+4*8,{&destination_keys[2]}},
  {"PLAYER 4"   ,S_KEY       ,m_scrn,KB_X,KB_Y+5*8,{&destination_keys[3]}},
  {"BACKSPACE"  ,S_KEY       ,m_scrn,KB_X,KB_Y+6*8,{&key_backspace}},
  {"ENTER"      ,S_KEY       ,m_scrn,KB_X,KB_Y+7*8,{&key_enter}},

  {"<- PREV" ,S_SKIP|S_PREV,m_null,KB_PREV,KB_Y+20*8, {keys_settings4}},

  // Final entry

  {0,S_SKIP|S_END,m_null}

};

// Setting up for the Key Binding screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_KeyBindings(int choice)
{
  M_SetupNextMenu(&KeybndDef);

  setup_active = true;
  setup_screen = ss_keys;
  set_keybnd_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = keys_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// The drawing part of the Key Bindings Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawKeybnd(void)

{
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  // Set up the Key Binding screen

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(84,2,"M_KEYBND","KEY BINDINGS");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}

/////////////////////////////
//
// The Weapon Screen tables.

#define WP_X 203
#define WP_Y  33

// There's only one weapon settings screen (for now). But since we're
// trying to fit a common description for screens, it gets a setup_menu_t,
// which only has one screen definition in it.
//
// Note that this screen has no PREV or NEXT items, since there are no
// neighboring screens.

enum {           // killough 10/98: enum for y-offset info
  weap_recoil,
  weap_bobbing,
  weap_bfg,
  weap_center, // [FG] centered weapon sprite
  weap_stub1,
  weap_pref1,
  weap_pref2,
  weap_pref3,
  weap_pref4,
  weap_pref5,
  weap_pref6,
  weap_pref7,
  weap_pref8,
  weap_pref9,
  weap_stub2,
  weap_toggle,
  weap_toggle2,
};

setup_menu_t weap_settings1[];

setup_menu_t* weap_settings[] =
{
  weap_settings1,
  NULL
};

// [FG] centered or bobbing weapon sprite
static const char *weapon_attack_alignment_strings[] = {
  "OFF", "CENTERED", "BOBBING", NULL
};

setup_menu_t weap_settings1[] =  // Weapons Settings screen
{
  {"ENABLE RECOIL", S_YESNO,m_null,WP_X, WP_Y+ weap_recoil*8, {"weapon_recoil"}},
  {"ENABLE BOBBING",S_YESNO,m_null,WP_X, WP_Y+weap_bobbing*8, {"player_bobbing"}},

  {"CLASSIC BFG"      ,S_YESNO,m_null,WP_X,  // killough 8/8/98
   WP_Y+ weap_bfg*8, {"classic_bfg"}},

  // [FG] centered or bobbing weapon sprite
  {"Weapon Attack Alignment",S_CHOICE,m_null,WP_X, WP_Y+weap_center*8, {"center_weapon"}, 0, 0, NULL, weapon_attack_alignment_strings},

  {"1ST CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref1*8, {"weapon_choice_1"}},
  {"2nd CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref2*8, {"weapon_choice_2"}},
  {"3rd CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref3*8, {"weapon_choice_3"}},
  {"4th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref4*8, {"weapon_choice_4"}},
  {"5th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref5*8, {"weapon_choice_5"}},
  {"6th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref6*8, {"weapon_choice_6"}},
  {"7th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref7*8, {"weapon_choice_7"}},
  {"8th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref8*8, {"weapon_choice_8"}},
  {"9th CHOICE WEAPON",S_WEAP,m_null,WP_X,WP_Y+weap_pref9*8, {"weapon_choice_9"}},

  {"Enable Fist/Chainsaw\n& SG/SSG toggle", S_YESNO, m_null, WP_X,
   WP_Y+ weap_toggle*8, {"doom_weapon_toggles"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  // Final entry
  {0,S_SKIP|S_END,m_null}

};

// Setting up for the Weapons screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Weapons(int choice)
{
  M_SetupNextMenu(&WeaponDef);

  setup_active = true;
  setup_screen = ss_weap;
  set_weapon_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = weap_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}


// The drawing part of the Weapons Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawWeapons(void)
{
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(109,2,"M_WEAP","WEAPONS");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}

/////////////////////////////
//
// The Status Bar / HUD tables.

#define ST_X 203
#define ST_Y  31

// Screen table definitions

setup_menu_t stat_settings1[];

setup_menu_t* stat_settings[] =
{
  stat_settings1,
  NULL
};

setup_menu_t stat_settings1[] =  // Status Bar and HUD Settings screen
{
  {"STATUS BAR"        ,S_SKIP|S_TITLE,m_null,ST_X,ST_Y+ 1*8 },

  {"USE RED NUMBERS"   ,S_YESNO, m_null,ST_X,ST_Y+ 2*8, {"sts_always_red"}},
  {"GRAY %"            ,S_YESNO, m_null,ST_X,ST_Y+ 3*8, {"sts_pct_always_gray"}},
  {"SINGLE KEY DISPLAY",S_YESNO, m_null,ST_X,ST_Y+ 4*8, {"sts_traditional_keys"}},

  {"HEADS-UP DISPLAY"  ,S_SKIP|S_TITLE,m_null,ST_X,ST_Y+ 6*8},

  {"HIDE SECRETS"      ,S_YESNO     ,m_null,ST_X,ST_Y+ 7*8, {"hud_nosecrets"}},
  {"HEALTH LOW/OK"     ,S_NUM       ,m_null,ST_X,ST_Y+ 8*8, {"health_red"}},
  {"HEALTH OK/GOOD"    ,S_NUM       ,m_null,ST_X,ST_Y+ 9*8, {"health_yellow"}},
  {"HEALTH GOOD/EXTRA" ,S_NUM       ,m_null,ST_X,ST_Y+10*8, {"health_green"}},
  {"ARMOR LOW/OK"      ,S_NUM       ,m_null,ST_X,ST_Y+11*8, {"armor_red"}},
  {"ARMOR OK/GOOD"     ,S_NUM       ,m_null,ST_X,ST_Y+12*8, {"armor_yellow"}},
  {"ARMOR GOOD/EXTRA"  ,S_NUM       ,m_null,ST_X,ST_Y+13*8, {"armor_green"}},
  {"AMMO LOW/OK"       ,S_NUM       ,m_null,ST_X,ST_Y+14*8, {"ammo_red"}},
  {"AMMO OK/GOOD"      ,S_NUM       ,m_null,ST_X,ST_Y+15*8, {"ammo_yellow"}},
  {"\"A SECRET IS REVEALED!\" MESSAGE",S_YESNO,m_null,ST_X,ST_Y+16*8, {"hud_secret_message"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  // Final entry
  {0,S_SKIP|S_END,m_null}
};

// Setting up for the Status Bar / HUD screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_StatusBar(int choice)
{
  M_SetupNextMenu(&StatusHUDDef);

  setup_active = true;
  setup_screen = ss_stat;
  set_status_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = stat_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}


// The drawing part of the Status Bar / HUD Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawStatusHUD(void)

{
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(59,2,"M_STAT","STATUS BAR / HUD");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}


/////////////////////////////
//
// The Automap tables.

#define AU_X    250
#define AU_Y     31
#define AU_PREV KB_PREV
#define AU_NEXT KB_NEXT

setup_menu_t auto_settings1[];
setup_menu_t auto_settings2[];

setup_menu_t* auto_settings[] =
{
  auto_settings1,
  auto_settings2,
  NULL
};

// [FG] show level statistics and level time widgets
static const char *show_widgets_strings[] = {
  "Off", "On Automap", "Always", NULL
};

setup_menu_t auto_settings1[] =  // 1st AutoMap Settings screen
{
  {"background", S_COLOR, m_null, AU_X, AU_Y, {"mapcolor_back"}},
  {"grid lines", S_COLOR, m_null, AU_X, AU_Y + 1*8, {"mapcolor_grid"}},
  {"normal 1s wall", S_COLOR, m_null,AU_X,AU_Y+ 2*8, {"mapcolor_wall"}},
  {"line at floor height change", S_COLOR, m_null, AU_X, AU_Y+ 3*8, {"mapcolor_fchg"}},
  {"line at ceiling height change"      ,S_COLOR,m_null,AU_X,AU_Y+ 4*8, {"mapcolor_cchg"}},
  {"line at sector with floor = ceiling",S_COLOR,m_null,AU_X,AU_Y+ 5*8, {"mapcolor_clsd"}},
  {"red key"                            ,S_COLOR,m_null,AU_X,AU_Y+ 6*8, {"mapcolor_rkey"}},
  {"blue key"                           ,S_COLOR,m_null,AU_X,AU_Y+ 7*8, {"mapcolor_bkey"}},
  {"yellow key"                         ,S_COLOR,m_null,AU_X,AU_Y+ 8*8, {"mapcolor_ykey"}},
  {"red door"                           ,S_COLOR,m_null,AU_X,AU_Y+ 9*8, {"mapcolor_rdor"}},
  {"blue door"                          ,S_COLOR,m_null,AU_X,AU_Y+10*8, {"mapcolor_bdor"}},
  {"yellow door"                        ,S_COLOR,m_null,AU_X,AU_Y+11*8, {"mapcolor_ydor"}},

  {"AUTOMAP LEVEL TITLE COLOR"      ,S_CRITEM,m_null,AU_X,AU_Y+13*8, {"hudcolor_titl"}},
  {"AUTOMAP COORDINATES COLOR"      ,S_CRITEM,m_null,AU_X,AU_Y+14*8, {"hudcolor_xyco"}},

  {"Show Secrets only after entering",S_YESNO,m_null,AU_X,AU_Y+15*8, {"map_secret_after"}},

  {"Show coordinates of automap pointer",S_YESNO,m_null,AU_X,AU_Y+16*8, {"map_point_coord"}},  // killough 10/98

  // [FG] show level statistics and level time widgets
  {"Show player coords", S_CHOICE,m_null,AU_X,AU_Y+17*8, {"map_player_coords"},0,0,NULL,show_widgets_strings},
  {"Show level stats",   S_CHOICE,m_null,AU_X,AU_Y+18*8, {"map_level_stats"},0,0,NULL,show_widgets_strings},
  {"Show level time",    S_CHOICE,m_null,AU_X,AU_Y+19*8, {"map_level_time"},0,0,NULL,show_widgets_strings},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  {"NEXT ->",S_SKIP|S_NEXT,m_null,AU_NEXT,AU_Y+20*8, {auto_settings2}},

  // Final entry
  {0,S_SKIP|S_END,m_null}

};

setup_menu_t auto_settings2[] =  // 2nd AutoMap Settings screen
{
  {"teleporter line"                ,S_COLOR ,m_null,AU_X,AU_Y, {"mapcolor_tele"}},
  {"secret sector boundary"         ,S_COLOR ,m_null,AU_X,AU_Y+ 1*8, {"mapcolor_secr"}},
  //jff 4/23/98 add exit line to automap
  {"exit line"                      ,S_COLOR ,m_null,AU_X,AU_Y+ 2*8, {"mapcolor_exit"}},
  {"computer map unseen line"       ,S_COLOR ,m_null,AU_X,AU_Y+ 3*8, {"mapcolor_unsn"}},
  {"line w/no floor/ceiling changes",S_COLOR ,m_null,AU_X,AU_Y+ 4*8, {"mapcolor_flat"}},
  {"general sprite"                 ,S_COLOR ,m_null,AU_X,AU_Y+ 5*8, {"mapcolor_sprt"}},
  {"crosshair"                      ,S_COLOR ,m_null,AU_X,AU_Y+ 6*8, {"mapcolor_hair"}},
  {"single player arrow"            ,S_COLOR ,m_null,AU_X,AU_Y+ 7*8, {"mapcolor_sngl"}},
  {"player 1 arrow"                 ,S_COLOR ,m_null,AU_X,AU_Y+ 8*8, {"mapcolor_ply1"}},
  {"player 2 arrow"                 ,S_COLOR ,m_null,AU_X,AU_Y+ 9*8, {"mapcolor_ply2"}},
  {"player 3 arrow"                 ,S_COLOR ,m_null,AU_X,AU_Y+10*8, {"mapcolor_ply3"}},
  {"player 4 arrow"                 ,S_COLOR ,m_null,AU_X,AU_Y+11*8, {"mapcolor_ply4"}},

  {"friends"                        ,S_COLOR ,m_null,AU_X,AU_Y+12*8, {"mapcolor_frnd"}},        // killough 8/8/98

  {"<- PREV",S_SKIP|S_PREV,m_null,AU_PREV,AU_Y+20*8, {auto_settings1}},

  // Final entry

  {0,S_SKIP|S_END,m_null}

};


// Setting up for the Automap screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Automap(int choice)
{
  M_SetupNextMenu(&AutoMapDef);

  setup_active = true;
  setup_screen = ss_auto;
  set_auto_active = true;
  setup_select = false;
  colorbox_active = false;
  default_verify = false;
  setup_gather = false;
  set_menu_itemon = 0;
  mult_screens_index = 0;
  current_setup_menu = auto_settings[0];
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// Data used by the color palette that is displayed for the player to
// select colors.

int color_palette_x; // X position of the cursor on the color palette
int color_palette_y; // Y position of the cursor on the color palette
byte palette_background[16*(CHIP_SIZE+1)+8];

// M_DrawColPal() draws the color palette when the user needs to select a
// color.

// phares 4/1/98: now uses a single lump for the palette instead of
// building the image out of individual paint chips.

void M_DrawColPal()
{
  int i,cpx,cpy;
  byte *ptr;

  // Draw a background, border, and paint chips

  V_DrawPatchDirect (COLORPALXORIG-5,COLORPALYORIG-5,0,W_CacheLumpName("M_COLORS",PU_CACHE));

  // Draw the cursor around the paint chip
  // (cpx,cpy) is the upper left-hand corner of the paint chip

  ptr = colorblock;
  for (i = 0 ; i < CHIP_SIZE+2 ; i++)
    *ptr++ = PAL_WHITE;
  cpx = COLORPALXORIG+color_palette_x*(CHIP_SIZE+1)-1+WIDESCREENDELTA;
  cpy = COLORPALYORIG+color_palette_y*(CHIP_SIZE+1)-1;
  V_DrawBlock(cpx,cpy,0,CHIP_SIZE+2,1,colorblock);
  V_DrawBlock(cpx+CHIP_SIZE+1,cpy,0,1,CHIP_SIZE+2,colorblock);
  V_DrawBlock(cpx,cpy+CHIP_SIZE+1,0,CHIP_SIZE+2,1,colorblock);
  V_DrawBlock(cpx,cpy,0,1,CHIP_SIZE+2,colorblock);
}

// The drawing part of the Automap Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawAutoMap(void)

{
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(109,2,"M_AUTO","AUTOMAP");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If a color is being selected, need to show color paint chips

  if (colorbox_active)
    M_DrawColPal();

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  else if (default_verify)
    M_DrawDefVerify();
}


/////////////////////////////
//
// The Enemies table.

#define E_X 250
#define E_Y  31

setup_menu_t enem_settings1[];

setup_menu_t* enem_settings[] =
{
  enem_settings1,
  NULL
};

enum {
  enem_infighting,

  enem_remember = 1,

  enem_backing,
  enem_monkeys,
  enem_avoid_hazards,
  enem_friction,
  enem_help_friends,

  enem_helpers,

  enem_distfriend,

  enem_dog_jumping,

  enem_colored_blood,

  enem_flipcorpses,

  enem_end
};

setup_menu_t enem_settings1[] =  // Enemy Settings screen
{
  // killough 7/19/98
  {"Monster Infighting When Provoked",S_YESNO,m_null,E_X,E_Y+ enem_infighting*8, {"monster_infighting"}},

  {"Remember Previous Enemy",S_YESNO,m_null,E_X,E_Y+ enem_remember*8, {"monsters_remember"}},

  // killough 9/8/98
  {"Monster Backing Out",S_YESNO,m_null,E_X,E_Y+ enem_backing*8, {"monster_backing"}},

  {"Climb Steep Stairs", S_YESNO,m_null,E_X,E_Y+enem_monkeys*8, {"monkeys"}},

  // killough 9/9/98
  {"Intelligently Avoid Hazards",S_YESNO,m_null,E_X,E_Y+ enem_avoid_hazards*8, {"monster_avoid_hazards"}},

  // killough 10/98
  {"Affected by Friction",S_YESNO,m_null,E_X,E_Y+ enem_friction*8, {"monster_friction"}},

  {"Rescue Dying Friends",S_YESNO,m_null,E_X,E_Y+ enem_help_friends*8, {"help_friends"}},

  // killough 7/19/98
  {"Number Of Single-Player Helper Dogs",S_NUM|S_LEVWARN,m_null,E_X,E_Y+ enem_helpers*8, {"player_helpers"}},

  // killough 8/8/98
  {"Distance Friends Stay Away",S_NUM,m_null,E_X,E_Y+ enem_distfriend*8, {"friend_distance"}},

  {"Allow dogs to jump down",S_YESNO,m_null,E_X,E_Y+ enem_dog_jumping*8, {"dog_jumping"}},

  // [FG] colored blood and gibs
  {"Colored Blood",S_YESNO,m_null,E_X,E_Y+ enem_colored_blood*8, {"colored_blood"}},

  // [crispy] randomly flip corpse, blood and death animation sprites
  {"Randomly Mirrored Corpses",S_YESNO,m_null,E_X,E_Y+ enem_flipcorpses*8, {"flipcorpses"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  // Final entry
  {0,S_SKIP|S_END,m_null}

};

/////////////////////////////

// Setting up for the Enemies screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Enemy(int choice)
{
  M_SetupNextMenu(&EnemyDef);

  setup_active = true;
  setup_screen = ss_enem;
  set_enemy_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = enem_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// The drawing part of the Enemies Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawEnemy(void)

{
  inhelpscreens = true;

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(114,2,"M_ENEM","ENEMIES");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}


/////////////////////////////
//
// The General table.
// killough 10/10/98

extern int usejoystick, usemouse, default_mus_card, default_snd_card;
extern int realtic_clock_rate, tran_filter_pct;

setup_menu_t gen_settings1[], gen_settings2[];

setup_menu_t* gen_settings[] =
{
  gen_settings1,
  gen_settings2,
  NULL
};

enum {
  general_hires,
  general_pageflip,
  general_vsync,
  general_trans,
  general_transpct,
  general_diskicon,
  general_hom,
  // [FG] fullscreen mode menu toggle
  general_fullscreen,
  // [FG] uncapped rendering frame rate
  general_uncapped,
  // widescreen mode
  general_widescreen
};

enum {
  general_sndchan,
  general_pitch,
  // [FG] play sounds in full length
  general_fullsnd,
  // [FG] music backend
  general_musicbackend,
};

static const char *music_backend_strings[] = {
  "SDL", "OPL", NULL
};

#define G_X 250
#define G_Y  44
#define G_Y2 (G_Y+92+8) // [FG] remove sound and music card items
#define G_Y3 (G_Y+44)
#define G_Y4 (G_Y3+52)
#define GF_X 76

setup_menu_t gen_settings1[] = { // General Settings screen1

  {"Video"       ,S_SKIP|S_TITLE, m_null, G_X, G_Y - 12},

  {"High Resolution", S_YESNO, m_null, G_X, G_Y + general_hires*8,
   {"hires"}, 0, 0, I_ResetScreen},

  // [FG] page_flip = !force_software_renderer
  {"Use Hardware Acceleration", S_YESNO, m_null, G_X, G_Y + general_pageflip*8,
   {"page_flip"}, 0, 0, I_ResetScreen},

  {"Wait for Vertical Retrace", S_YESNO, m_null, G_X,
   G_Y + general_vsync*8, {"use_vsync"}, 0, 0, I_ResetScreen},

  {"Enable Translucency", S_YESNO, m_null, G_X,
   G_Y + general_trans*8, {"translucency"}, 0, 0, M_Trans},

  {"Translucency filter percentage", S_NUM, m_null, G_X,
   G_Y + general_transpct*8, {"tran_filter_pct"}, 0, 0, M_Trans},

  {"Flash Icon During Disk IO", S_YESNO, m_null, G_X,
   G_Y + general_diskicon*8, {"disk_icon"}},

  {"Flashing HOM indicator", S_YESNO, m_null, G_X,
   G_Y + general_hom*8, {"flashing_hom"}},

  // [FG] fullscreen mode menu toggle
  {"Fullscreen Mode", S_YESNO, m_null, G_X, G_Y + general_fullscreen*8,
   {"fullscreen"}, 0, 0, I_ToggleToggleFullScreen},

  // [FG] uncapped rendering frame rate
  {"Uncapped Rendering Frame Rate", S_YESNO, m_null, G_X, G_Y + general_uncapped*8,
   {"uncapped"}},

  {"Widescreen Mode", S_YESNO, m_null, G_X, G_Y + general_widescreen*8,
   {"widescreen"}, 0, 0, I_ResetScreen},

  {"Sound & Music", S_SKIP|S_TITLE, m_null, G_X, G_Y2 - 12},

  {"Number of Sound Channels", S_NUM|S_PRGWARN, m_null, G_X,
   G_Y2 + general_sndchan*8, {"snd_channels"}},

  {"Enable v1.1 Pitch Effects", S_YESNO, m_null, G_X,
   G_Y2 + general_pitch*8, {"pitched_sounds"}},

  // [FG] play sounds in full length
  {"play sounds in full length", S_YESNO, m_null, G_X,
   G_Y2 + general_fullsnd*8, {"full_sounds"}},

  // [FG] music backend
  {"music backend", S_CHOICE|S_PRGWARN, m_null, G_X,
   G_Y2 + general_musicbackend*8, {"music_backend"}, 0, 0, NULL, music_backend_strings},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  {"NEXT ->",S_SKIP|S_NEXT,m_null,KB_NEXT,KB_Y+20*8, {gen_settings2}},

  // Final entry
  {0,S_SKIP|S_END,m_null}
};

enum {
  general_mouse,
  general_joy,
  general_leds
};

enum {
  general_wad1,
  general_wad2,
  general_deh1,
  general_deh2
};

enum {
  general_corpse,
  general_realtic,
  general_comp,
  general_end
};

static const char *default_compatibility_strings[] = {
  "Vanilla", "Boom", "MBF", "MBF21", NULL
};

setup_menu_t gen_settings2[] = { // General Settings screen2

  {"Input Devices"     ,S_SKIP|S_TITLE, m_null, G_X, G_Y - 12},

  {"Enable Mouse", S_YESNO, m_null, G_X,
   G_Y + general_mouse*8, {"use_mouse"}},

  {"Enable Joystick", S_YESNO, m_null, G_X,
   G_Y + general_joy*8, {"use_joystick"}, 0, 0, I_InitJoystick},

  // [FG] double click acts as "use"
  {"Double Click acts as \"Use\"", S_YESNO, m_null, G_X,
   G_Y + general_leds*8, {"dclick_use"}},

#if 0
  {"Keyboard LEDs Always Off", S_YESNO, m_null, G_X,
   G_Y + general_leds*8, {"leds_always_off"}, 0, 0, I_ResetLEDs},
#endif

  {"Files Preloaded at Game Startup",S_SKIP|S_TITLE, m_null, G_X,
   G_Y3 - 12},

  {"WAD # 1", S_FILE, m_null, GF_X, G_Y3 + general_wad1*8, {"wadfile_1"}},

  {"WAD #2", S_FILE, m_null, GF_X, G_Y3 + general_wad2*8, {"wadfile_2"}},

  {"DEH/BEX # 1", S_FILE, m_null, GF_X, G_Y3 + general_deh1*8, {"dehfile_1"}},

  {"DEH/BEX #2", S_FILE, m_null, GF_X, G_Y3 + general_deh2*8, {"dehfile_2"}},

  {"Miscellaneous"  ,S_SKIP|S_TITLE, m_null, G_X, G_Y4 - 12},

  {"Maximum number of player corpses", S_NUM|S_PRGWARN, m_null, G_X,
   G_Y4 + general_corpse*8, {"max_player_corpse"}},

  {"Game speed, percentage of normal", S_NUM|S_PRGWARN, m_null, G_X,
   G_Y4 + general_realtic*8, {"realtic_clock_rate"}},

  {"Default compatibility", S_CHOICE|S_LEVWARN, m_null, G_X,
   G_Y4 + general_comp*8, {"default_complevel"}, 0, 0, NULL, default_compatibility_strings},

  {"<- PREV",S_SKIP|S_PREV, m_null, KB_PREV, KB_Y+20*8, {gen_settings1}},

  // Final entry

  {0,S_SKIP|S_END,m_null}
};

void M_Trans(void) // To reset translucency after setting it in menu
{
  if (general_translucency)
    R_InitTranMap(0);
}

// Setting up for the General screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_General(int choice)
{
  M_SetupNextMenu(&GeneralDef);

  setup_active = true;
  setup_screen = ss_gen;
  set_general_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = gen_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// The drawing part of the General Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawGeneral(void)
{
  inhelpscreens = true;

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(114,2,"M_GENERL","GENERAL");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}

/////////////////////////////
//
// The Compatibility table.
// killough 10/10/98

#define C_X  284
#define C_Y  32
#define COMP_SPC 12
#define C_NEXTPREV 131

setup_menu_t comp_settings1[], comp_settings2[];

setup_menu_t* comp_settings[] =
{
  comp_settings1,
  comp_settings2,
  NULL
};

enum
{
  compat_telefrag,
  compat_dropoff,
  compat_falloff,
  compat_staylift,
  compat_doorstuck,
  compat_pursuit,
  compat_vile,
  compat_pain,
  compat_skull,
  compat_blazing,
  compat_doorlight = 0,
  compat_god,
  compat_infcheat,
  compat_zombie,
  compat_skymap,
  compat_stairs,
  compat_floors,
  compat_model,
  compat_zerotags,
  compat_menu,
};

setup_menu_t comp_settings1[] =  // Compatibility Settings screen #1
{
  {"Any monster can telefrag on MAP30", S_YESNO, m_null, C_X,
   C_Y + compat_telefrag * COMP_SPC, {"comp_telefrag"}},

  {"Some objects never hang over tall ledges", S_YESNO, m_null, C_X,
   C_Y + compat_dropoff * COMP_SPC, {"comp_dropoff"}},

  {"Objects don't fall under their own weight", S_YESNO, m_null, C_X,
   C_Y + compat_falloff * COMP_SPC, {"comp_falloff"}},

  {"Monsters randomly walk off of moving lifts", S_YESNO, m_null, C_X,
   C_Y + compat_staylift * COMP_SPC, {"comp_staylift"}},

  {"Monsters get stuck on doortracks", S_YESNO, m_null, C_X,
   C_Y + compat_doorstuck * COMP_SPC, {"comp_doorstuck"}},

  {"Monsters don't give up pursuit of targets", S_YESNO, m_null, C_X,
   C_Y + compat_pursuit * COMP_SPC, {"comp_pursuit"}},

  {"Arch-Vile resurrects invincible ghosts", S_YESNO, m_null, C_X,
   C_Y + compat_vile * COMP_SPC, {"comp_vile"}},

  {"Pain Elemental limited to 20 lost souls", S_YESNO, m_null, C_X,
   C_Y + compat_pain * COMP_SPC, {"comp_pain"}},

  {"Lost souls get stuck behind walls", S_YESNO, m_null, C_X,
   C_Y + compat_skull * COMP_SPC, {"comp_skull"}},

  {"Blazing doors make double closing sounds", S_YESNO, m_null, C_X,
   C_Y + compat_blazing * COMP_SPC, {"comp_blazing"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  {"NEXT ->",S_SKIP|S_NEXT, m_null, KB_NEXT, C_Y+C_NEXTPREV, {comp_settings2}},

  // Final entry
  {0,S_SKIP|S_END,m_null}
};

setup_menu_t comp_settings2[] =  // Compatibility Settings screen #2
{
  {"Tagged doors don't trigger special lighting", S_YESNO, m_null, C_X,
   C_Y + compat_doorlight * COMP_SPC, {"comp_doorlight"}},

  {"God mode isn't absolute", S_YESNO, m_null, C_X,
   C_Y + compat_god * COMP_SPC, {"comp_god"}},

  {"Powerup cheats are not infinite duration", S_YESNO, m_null, C_X,
   C_Y + compat_infcheat * COMP_SPC, {"comp_infcheat"}},

  {"Zombie players can exit levels", S_YESNO, m_null, C_X,
   C_Y + compat_zombie * COMP_SPC, {"comp_zombie"}},

  {"Sky is unaffected by invulnerability", S_YESNO, m_null, C_X,
   C_Y + compat_skymap * COMP_SPC, {"comp_skymap"}},

  {"Use exactly Doom's stairbuilding method", S_YESNO, m_null, C_X,
   C_Y + compat_stairs * COMP_SPC, {"comp_stairs"}},

  {"Use exactly Doom's floor motion behavior", S_YESNO, m_null, C_X,
   C_Y + compat_floors * COMP_SPC, {"comp_floors"}},

  {"Use exactly Doom's linedef trigger model", S_YESNO, m_null, C_X,
   C_Y + compat_model * COMP_SPC, {"comp_model"}},

  {"Linedef effects work with sector tag = 0", S_YESNO, m_null, C_X,
   C_Y + compat_zerotags * COMP_SPC, {"comp_zerotags"}},

  {"Use Doom's main menu ordering", S_YESNO, m_null, C_X,
   C_Y + compat_menu * COMP_SPC, {"traditional_menu"}, 0, 0, M_ResetMenu},

  {"<- PREV", S_SKIP|S_PREV, m_null, KB_PREV, C_Y+C_NEXTPREV,{comp_settings1}},

  // Final entry

  {0,S_SKIP|S_END,m_null}
};


// Setting up for the Compatibility screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Compat(int choice)
{
  M_SetupNextMenu(&CompatDef);

  setup_active = true;
  setup_screen = ss_comp;
  set_general_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = comp_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// The drawing part of the Compatibility Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawCompat(void)
{
  inhelpscreens = true;

  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(52,2,"M_COMPAT","DOOM COMPATIBILITY");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}

/////////////////////////////
//
// The Messages table.

#define M_X 230
#define M_Y  39

// killough 11/98: enumerated

enum {
  mess_color_play,
  mess_timer,
  mess_color_chat,
  mess_chat_timer,
  mess_color_review,
  mess_timed,
  mess_hud_timer,
  mess_lines,
  mess_scrollup,
  mess_background,
};

setup_menu_t mess_settings1[];

setup_menu_t* mess_settings[] =
{
  mess_settings1,
  NULL
};

setup_menu_t mess_settings1[] =  // Messages screen
{
  {"Message Color During Play", S_CRITEM, m_null, M_X,
   M_Y + mess_color_play*8, {"hudcolor_mesg"}},

  {"Message Duration During Play (ms)", S_NUM, m_null, M_X,
   M_Y  + mess_timer*8, {"message_timer"}},

  {"Chat Message Color", S_CRITEM, m_null, M_X,
   M_Y + mess_color_chat*8, {"hudcolor_chat"}},

  {"Chat Message Duration (ms)", S_NUM, m_null, M_X,
   M_Y  + mess_chat_timer*8, {"chat_msg_timer"}},

  {"Message Review Color", S_CRITEM, m_null, M_X,
   M_Y + mess_color_review*8, {"hudcolor_list"}},

  {"Message Listing Review is Temporary",  S_YESNO,  m_null,  M_X,
   M_Y + mess_timed*8, {"hud_msg_timed"}},

  {"Message Review Duration (ms)", S_NUM, m_null, M_X,
   M_Y  + mess_hud_timer*8, {"hud_msg_timer"}},

  {"Number of Review Message Lines", S_NUM, m_null,  M_X,
   M_Y + mess_lines*8, {"hud_msg_lines"}},

  {"Message Listing Scrolls Upwards",  S_YESNO,  m_null,  M_X,
   M_Y + mess_scrollup*8, {"hud_msg_scrollup"}},

  {"Message Background",  S_YESNO,  m_null,  M_X,
   M_Y + mess_background*8, {"hud_list_bgon"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  // Final entry

  {0,S_SKIP|S_END,m_null}
};


// Setting up for the Messages screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Messages(int choice)
{
  M_SetupNextMenu(&MessageDef);

  setup_active = true;
  setup_screen = ss_mess;
  set_mess_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = mess_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}


// The drawing part of the Messages Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawMessages(void)

{
  inhelpscreens = true;
  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(103,2,"M_MESS","MESSAGES");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);
  if (default_verify)
    M_DrawDefVerify();
}


/////////////////////////////
//
// The Chat Strings table.

#define CS_X 20
#define CS_Y (31+8)

setup_menu_t chat_settings1[];

setup_menu_t* chat_settings[] =
{
  chat_settings1,
  NULL
};

setup_menu_t chat_settings1[] =  // Chat Strings screen
{
  {"1",S_CHAT,m_null,CS_X,CS_Y+ 1*8, {"chatmacro1"}},
  {"2",S_CHAT,m_null,CS_X,CS_Y+ 2*8, {"chatmacro2"}},
  {"3",S_CHAT,m_null,CS_X,CS_Y+ 3*8, {"chatmacro3"}},
  {"4",S_CHAT,m_null,CS_X,CS_Y+ 4*8, {"chatmacro4"}},
  {"5",S_CHAT,m_null,CS_X,CS_Y+ 5*8, {"chatmacro5"}},
  {"6",S_CHAT,m_null,CS_X,CS_Y+ 6*8, {"chatmacro6"}},
  {"7",S_CHAT,m_null,CS_X,CS_Y+ 7*8, {"chatmacro7"}},
  {"8",S_CHAT,m_null,CS_X,CS_Y+ 8*8, {"chatmacro8"}},
  {"9",S_CHAT,m_null,CS_X,CS_Y+ 9*8, {"chatmacro9"}},
  {"0",S_CHAT,m_null,CS_X,CS_Y+10*8, {"chatmacro0"}},

  // Button for resetting to defaults
  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON},

  // Final entry
  {0,S_SKIP|S_END,m_null}

};

// Setting up for the Chat Strings screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_ChatStrings(int choice)
{
  M_SetupNextMenu(&ChatStrDef);
  setup_active = true;
  setup_screen = ss_chat;
  set_chat_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  mult_screens_index = 0;
  current_setup_menu = chat_settings[0];
  set_menu_itemon = 0;
  while (current_setup_menu[set_menu_itemon++].m_flags & S_SKIP);
  current_setup_menu[--set_menu_itemon].m_flags |= S_HILITE;
}

// The drawing part of the Chat Strings Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawChatStrings(void)

{
  inhelpscreens = true;
  M_DrawBackground("FLOOR4_6", screens[0]); // Draw background
  M_DrawTitle(83,2,"M_CHAT","CHAT STRINGS");
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
}

/////////////////////////////
//
// General routines used by the Setup screens.
//

static boolean shiftdown = false; // phares 4/10/98: SHIFT key down or not

// phares 4/17/98:
// M_SelectDone() gets called when you have finished entering your
// Setup Menu item change.

void M_SelectDone(setup_menu_t* ptr)
{
  ptr->m_flags &= ~S_SELECT;
  ptr->m_flags |= S_HILITE;
  S_StartSound(NULL,sfx_itemup);
  setup_select = false;
  colorbox_active = false;
  if (print_warning_about_changes)     // killough 8/15/98
    print_warning_about_changes--;
}

// phares 4/21/98:
// Array of setup screens used by M_ResetDefaults()

static setup_menu_t **setup_screens[] =
{
  keys_settings,
  weap_settings,
  stat_settings,
  auto_settings,
  enem_settings,
  mess_settings,
  chat_settings,
  gen_settings,      // killough 10/98
  comp_settings,
};

// phares 4/19/98:
// M_ResetDefaults() resets all values for a setup screen to default values
//
// killough 10/98: rewritten to fix bugs and warn about pending changes

void M_ResetDefaults()
{
  default_t *dp;
  int warn = 0;

  // Look through the defaults table and reset every variable that
  // belongs to the group we're interested in.
  //
  // killough: However, only reset variables whose field in the
  // current setup screen is the same as in the defaults table.
  // i.e. only reset variables really in the current setup screen.

  for (dp = defaults; dp->name; dp++)
    if (dp->setupscreen == setup_screen)
      {
	setup_menu_t **l, *p;
	for (l = setup_screens[setup_screen-1]; *l; l++)
	  for (p = *l; !(p->m_flags & S_END); p++)
	    if (p->m_flags & S_HASDEFPTR ? p->var.def == dp :
		p->var.m_key == &dp->location->i ||
		p->m_mouse == &dp->location->i ||
		p->m_joy == &dp->location->i)
	      {
		if (dp->isstr)
		  free(dp->location->s),
		    dp->location->s = strdup(dp->defaultvalue.s);
		else
		  dp->location->i = dp->defaultvalue.i;

		if (p->m_flags & (S_LEVWARN | S_PRGWARN))
		  warn |= p->m_flags & (S_LEVWARN | S_PRGWARN);
		else
		  if (dp->current)
		  {
		    if (allow_changes())
		    {
		      if (dp->isstr)
		      dp->current->s = dp->location->s;
		      else
		      dp->current->i = dp->location->i;
		    }
		    else
		      warn |= S_LEVWARN;
		  }

		if (p->action)
		  p->action();

		goto end;
	      }
      end:;
      }

  if (warn)
    warn_about_changes(warn);
}

//
// M_InitDefaults()
//
// killough 11/98:
//
// This function converts all setup menu entries consisting of cfg
// variable names, into pointers to the corresponding default[]
// array entry. var.name becomes converted to var.def.
//

static void M_InitDefaults(void)
{
  setup_menu_t *const *p, *t;
  default_t *dp;
  int i;
  for (i = 0; i < ss_max-1; i++)
    for (p = setup_screens[i]; *p; p++)
      for (t = *p; !(t->m_flags & S_END); t++)
	if (t->m_flags & S_HASDEFPTR)
	{
	  if (!(dp = M_LookupDefault(t->var.name)))
	    I_Error("Could not find config variable \"%s\"", t->var.name);
	  else
	    (t->var.def = dp)->setup_menu = t;
	}
}

//
// End of Setup Screens.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Start of Extended HELP screens               // phares 3/30/98
//
// The wad designer can define a set of extended HELP screens for their own
// information display. These screens should be 320x200 graphic lumps
// defined in a separate wad. They should be named "HELP01" through "HELP99".
// "HELP01" is shown after the regular BOOM Dynamic HELP screen, and ENTER
// and BACKSPACE keys move the player through the HELP set.
//
// Rather than define a set of menu definitions for each of the possible
// HELP screens, one definition is used, and is altered on the fly
// depending on what HELPnn lumps the game finds.

// phares 3/30/98:
// Extended Help Screen variables

int extended_help_count;   // number of user-defined help screens found
int extended_help_index;   // index of current extended help screen

menuitem_t ExtHelpMenu[] =
{
  {1,"",M_ExtHelpNextScreen,0}
};

menu_t ExtHelpDef =
{
  1,             // # of menu items
  &ReadDef1,     // previous menu
  ExtHelpMenu,   // menuitem_t ->
  M_DrawExtHelp, // drawing routine ->
  330,181,       // x,y
  0              // lastOn
};

// M_ExtHelpNextScreen establishes the number of the next HELP screen in
// the series.

void M_ExtHelpNextScreen(int choice)
{
  choice = 0;
  if (++extended_help_index > extended_help_count)
    {

      // when finished with extended help screens, return to Main Menu

      extended_help_index = 1;
      M_SetupNextMenu(&MainDef);
    }
}

// phares 3/30/98:
// Routine to look for HELPnn screens and create a menu
// definition structure that defines extended help screens.

void M_InitExtendedHelp(void)

{
  int index,i;
  char namebfr[] = "HELPnn"; // haleyjd: can't write into constant

  extended_help_count = 0;
  for (index = 1 ; index < 100 ; index++)
    {
      namebfr[4] = index/10 + 0x30;
      namebfr[5] = index%10 + 0x30;
      i = W_CheckNumForName(namebfr);
      if (i == -1)
	{
	  if (extended_help_count)
	  {
	    // Restore extended help functionality
	    // for all game versions
	    ExtHelpDef.prevMenu  = &HelpDef; // previous menu
	    HelpMenu[0].routine = M_ExtHelp;

	    if (gamemode != commercial)
	      {
		ReadMenu2[0].routine = M_ExtHelp;
	      }
	  }
	  return;
	}
      extended_help_count++;
    }

}

// Initialization for the extended HELP screens.

void M_ExtHelp(int choice)
{
  choice = 0;
  extended_help_index = 1; // Start with first extended help screen
  M_SetupNextMenu(&ExtHelpDef);
}

// Initialize the drawing part of the extended HELP screens.

void M_DrawExtHelp(void)
{
  char namebfr[] = "HELPnn"; // [FG] char array!

  inhelpscreens = true;              // killough 5/1/98
  namebfr[4] = extended_help_index/10 + 0x30;
  namebfr[5] = extended_help_index%10 + 0x30;
  V_DrawPatchFullScreen(0,W_CacheLumpName(namebfr,PU_CACHE));
}

//
// End of Extended HELP screens               // phares 3/30/98
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// Dynamic HELP screen                     // phares 3/2/98
//
// Rather than providing the static HELP screens from DOOM and its versions,
// BOOM provides the player with a dynamic HELP screen that displays the
// current settings of major key bindings.
//
// The Dynamic HELP screen is defined in a manner similar to that used for
// the Setup Screens above.
//
// M_GetKeyString finds the correct string to represent the key binding
// for the current item being drawn.

int M_GetKeyString(int c,int offset)
{
  char* s;

  if (c >= 33 && c <= 126)
    {

      // The '=', ',', and '.' keys originally meant the shifted
      // versions of those keys, but w/o having to shift them in
      // the game. Any actions that are mapped to these keys will
      // still mean their shifted versions. Could be changed later
      // if someone can come up with a better way to deal with them.

      if (c == '=')      // probably means the '+' key?
	c = '+';
      else if (c == ',') // probably means the '<' key?
	c = '<';
      else if (c == '.') // probably means the '>' key?
	c = '>';
      menu_buffer[offset++] = c; // Just insert the ascii key
      menu_buffer[offset] = 0;
    }
  else
    {

      // Retrieve 4-letter (max) string representing the key

      switch(c)
	{
	case KEYD_TAB:
	  s = "TAB";
	  break;
	case KEYD_ENTER:
	  s = "ENTR";
	  break;
	case KEYD_ESCAPE:
	  s = "ESC";
	  break;
	case KEYD_SPACEBAR:
	  s = "SPAC";
	  break;
	case KEYD_BACKSPACE:
	  s = "BACK";
	  break;
	case KEYD_RCTRL:
	  s = "CTRL";
	  break;
	case KEYD_LEFTARROW:
	  s = "LARR";
	  break;
	case KEYD_UPARROW:
	  s = "UARR";
	  break;
	case KEYD_RIGHTARROW:
	  s = "RARR";
	  break;
	case KEYD_DOWNARROW:
	  s = "DARR";
	  break;
	case KEYD_RSHIFT:
	  s = "SHFT";
	  break;
	case KEYD_RALT:
	  s = "ALT";
	  break;
	case KEYD_CAPSLOCK:
	  s = "CAPS";
	  break;
	case KEYD_F1:
	  s = "F1";
	  break;
	case KEYD_F2:
	  s = "F2";
	  break;
	case KEYD_F3:
	  s = "F3";
	  break;
	case KEYD_F4:
	  s = "F4";
	  break;
	case KEYD_F5:
	  s = "F5";
	  break;
	case KEYD_F6:
	  s = "F6";
	  break;
	case KEYD_F7:
	  s = "F7";
	  break;
	case KEYD_F8:
	  s = "F8";
	  break;
	case KEYD_F9:
	  s = "F9";
	  break;
	case KEYD_F10:
	  s = "F10";
	  break;
	case KEYD_SCROLLLOCK:
	  s = "SCRL";
	  break;
	case KEYD_HOME:
	  s = "HOME";
	  break;
	case KEYD_PAGEUP:
	  s = "PGUP";
	  break;
	case KEYD_END:
	  s = "END";
	  break;
	case KEYD_PAGEDOWN:
	  s = "PGDN";
	  break;
	case KEYD_INSERT:
	  s = "INST";
	  break;
	case KEYD_F11:
	  s = "F11";
	  break;
	case KEYD_F12:
	  s = "F12";
	  break;
	case KEYD_PAUSE:
	  s = "PAUS";
	  break;
	// [FG] clear key bindings with the DEL key
	case KEYD_DEL:
	  s = "DEL";
	  break;
	case 0:
	  s = "NONE";
	  break;
	default:
	  s = "JUNK";
	  break;
	}
      strcpy(&menu_buffer[offset],s); // string to display
      offset += strlen(s);
    }
  return offset;
}

//
// The Dynamic HELP screen table.

#define KT_X1 283
#define KT_X2 172
#define KT_X3  87

#define KT_Y1   2
#define KT_Y2 118
#define KT_Y3 102

setup_menu_t helpstrings[] =  // HELP screen strings
{
  {"SCREEN"      ,S_SKIP|S_TITLE,m_null,KT_X1,KT_Y1},
  {"HELP"        ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 1*8,{&key_help}},
  {"MENU"        ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 2*8,{&key_escape}},
  {"SETUP"       ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 3*8,{&key_setup}},
  {"PAUSE"       ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 4*8,{&key_pause}},
  {"AUTOMAP"     ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 5*8,{&key_map}},
  {"SOUND VOLUME",S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 6*8,{&key_soundvolume}},
  {"HUD"         ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 7*8,{&key_hud}},
  {"MESSAGES"    ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 8*8,{&key_messages}},
  {"GAMMA FIX"   ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+ 9*8,{&key_gamma}},
  {"SPY"         ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+10*8,{&key_spy}},
  {"LARGER VIEW" ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+11*8,{&key_zoomin}},
  {"SMALLER VIEW",S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+12*8,{&key_zoomout}},
  {"SCREENSHOT"  ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y1+13*8,{&key_screenshot}},

  {"AUTOMAP"     ,S_SKIP|S_TITLE,m_null,KT_X1,KT_Y2},
  {"FOLLOW MODE" ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 1*8,{&key_map_follow}},
  {"ZOOM IN"     ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 2*8,{&key_map_zoomin}},
  {"ZOOM OUT"    ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 3*8,{&key_map_zoomout}},
  {"MARK PLACE"  ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 4*8,{&key_map_mark}},
  {"CLEAR MARKS" ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 5*8,{&key_map_clear}},
  {"FULL/ZOOM"   ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 6*8,{&key_map_gobig}},
  {"GRID"        ,S_SKIP|S_KEY,m_null,KT_X1,KT_Y2+ 7*8,{&key_map_grid}},

  {"WEAPONS"     ,S_SKIP|S_TITLE,m_null,KT_X3,KT_Y1},
  {"FIST"        ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 1*8,{&key_weapon1}},
  {"PISTOL"      ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 2*8,{&key_weapon2}},
  {"SHOTGUN"     ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 3*8,{&key_weapon3}},
  {"CHAINGUN"    ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 4*8,{&key_weapon4}},
  {"ROCKET"      ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 5*8,{&key_weapon5}},
  {"PLASMA"      ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 6*8,{&key_weapon6}},
  {"BFG 9000"    ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 7*8,{&key_weapon7}},
  {"CHAINSAW"    ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 8*8,{&key_weapon8}},
  {"SSG"         ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+ 9*8,{&key_weapon9}},
  {"BEST"        ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+10*8,{&key_weapontoggle}},
  {"FIRE"        ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y1+11*8,{&key_fire},&mousebfire,&joybfire},

  {"MOVEMENT"    ,S_SKIP|S_TITLE,m_null,KT_X3,KT_Y3},
  {"FORWARD"     ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 1*8,{&key_up},&mousebforward},
  {"BACKWARD"    ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 2*8,{&key_down},&mousebbackward},
  {"TURN LEFT"   ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 3*8,{&key_left},&mousebturnleft},
  {"TURN RIGHT"  ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 4*8,{&key_right},&mousebturnright},
  {"RUN"         ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 5*8,{&key_speed},0,&joybspeed},
  {"STRAFE LEFT" ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 6*8,{&key_strafeleft},0,&joybstrafeleft},
  {"STRAFE RIGHT",S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 7*8,{&key_straferight},0,&joybstraferight},
  {"STRAFE"      ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 8*8,{&key_strafe},&mousebstrafe,&joybstrafe},
  {"AUTORUN"     ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+ 9*8,{&key_autorun}},
  {"180 TURN"    ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+10*8,{&key_reverse}},
  {"USE"         ,S_SKIP|S_KEY,m_null,KT_X3,KT_Y3+11*8,{&key_use},&mousebuse,&joybuse},

  {"GAME"        ,S_SKIP|S_TITLE,m_null,KT_X2,KT_Y1},
  {"SAVE"        ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 1*8,{&key_savegame}},
  {"LOAD"        ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 2*8,{&key_loadgame}},
  {"QUICKSAVE"   ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 3*8,{&key_quicksave}},
  {"END GAME"    ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 4*8,{&key_endgame}},
  {"QUICKLOAD"   ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 5*8,{&key_quickload}},
  {"QUIT"        ,S_SKIP|S_KEY,m_null,KT_X2,KT_Y1+ 6*8,{&key_quit}},

  // Final entry

  {0,S_SKIP|S_END,m_null}
};

#define SPACEWIDTH 4

// M_DrawMenuString() draws the string in menu_buffer[]

void M_DrawStringCR(int cx, int cy, char *color, const char* ch)
{
  int   w;
  int   c;

  while (*ch)
    {
      c = *ch++;         // get next char
      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c> HU_FONTSIZE)
	{
	  cx += SPACEWIDTH;    // space
	  continue;
	}
      w = SHORT (hu_font[c]->width);
      if (cx + w > SCREENWIDTH)
	break;

      // V_DrawpatchTranslated() will draw the string in the
      // desired color, colrngs[color]

      V_DrawPatchTranslated(cx,cy,0,hu_font[c],color,0);

      // The screen is cramped, so trim one unit from each
      // character so they butt up against each other.
      cx += w - 1;
    }
}

void M_DrawString(int cx, int cy, int color, const char* ch)
{
  return M_DrawStringCR(cx, cy, colrngs[color], ch);
}

void M_DrawStringDisable(int cx, int cy, const char* ch)
{
  return M_DrawStringCR(cx, cy, (char *)&colormaps[0][256*15], ch);
}

// cph 2006/08/06 - M_DrawString() is the old M_DrawMenuString, except that it is not tied to menu_buffer

void M_DrawMenuString(int cx, int cy, int color)
{
  return M_DrawString(cx, cy, color, menu_buffer);
}

// M_GetPixelWidth() returns the number of pixels in the width of
// the string, NOT the number of chars in the string.

int M_GetPixelWidth(char* ch)
{
  int len = 0;
  int c;

  while (*ch)
    {
      c = *ch++;    // pick up next char
      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c > HU_FONTSIZE)
	{
	  len += SPACEWIDTH;   // space
	  continue;
	}
      len += SHORT (hu_font[c]->width);
      len--; // adjust so everything fits
    }
  len++; // replace what you took away on the last char only
  return len;
}

//
// M_DrawHelp
//
// This displays the help screen

void M_DrawHelp (void)
{
  // Display help screen from PWAD
  int helplump;
  if (gamemode == commercial)
    helplump = W_CheckNumForName("HELP");
  else
    helplump = W_CheckNumForName("HELP1");

  inhelpscreens = true;                        // killough 10/98
  if (helplump < 0 || W_IsIWADLump(helplump))
  {
  M_DrawBackground("FLOOR4_6", screens[0]);
  V_MarkRect (0,0,SCREENWIDTH,SCREENHEIGHT);
  M_DrawScreenItems(helpstrings);
  }
  else
  {
    V_DrawPatchFullScreen(0, W_CacheLumpNum(helplump, PU_CACHE));
  }
}

//
// End of Dynamic HELP screen                // phares 3/2/98
//
////////////////////////////////////////////////////////////////////////////

enum {
  prog,
  art,
  test, test_stub, test_stub2,
  canine,
  musicsfx, /*musicsfx_stub,*/
  woof, // [FG] shamelessly add myself to the Credits page ;)
  adcr, adcr_stub,
  special, special_stub, special_stub2,
};

enum {
  cr_prog=1,
  cr_art,
  cr_test,
  cr_canine,
  cr_musicsfx,
  cr_woof, // [FG] shamelessly add myself to the Credits page ;)
  cr_adcr,
  cr_special,
};

#define CR_S 9
#define CR_X 152
#define CR_X2 (CR_X+8)
#define CR_Y 31
#define CR_SH 2

setup_menu_t cred_settings[]={

  {"Programmer",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*prog + CR_SH*cr_prog},
  {"Lee Killough",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*prog + CR_SH*cr_prog},

  {"Artist",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*art + CR_SH*cr_art},
  {"Len Pitre",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*art + CR_SH*cr_art},

  {"PlayTesters",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*test + CR_SH*cr_test},
  {"Ky (Rez) Moffet",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*test + CR_SH*cr_test},
  {"Len Pitre",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*(test+1) + CR_SH*cr_test},
  {"James (Quasar) Haley",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*(test+2) + CR_SH*cr_test},

  {"Canine Consulting",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*canine + CR_SH*cr_canine},
  {"Longplain Kennels, Reg'd",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*canine + CR_SH*cr_canine},

  // haleyjd 05/12/09: changed Allegro credits to Team Eternity
  {"SDL Port By",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*musicsfx + CR_SH*cr_musicsfx},
  {"Team Eternity",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*musicsfx + CR_SH*cr_musicsfx},

  // [FG] shamelessly add myself to the Credits page ;)
  {"Woof! by",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*woof + CR_SH*cr_woof},
  {"Fabian Greffrath",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*woof + CR_SH*cr_woof},

  {"Additional Credit To",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*adcr + CR_SH*cr_adcr},
  {"id Software",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*adcr+CR_SH*cr_adcr},
  {"TeamTNT",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*(adcr+1)+CR_SH*cr_adcr},

  {"Special Thanks To",S_SKIP|S_CREDIT,m_null, CR_X, CR_Y + CR_S*special + CR_SH*cr_special},
  {"John Romero",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*(special+0)+CR_SH*cr_special},
  {"Joel Murdoch",S_SKIP|S_CREDIT|S_LEFTJUST,m_null, CR_X2, CR_Y + CR_S*(special+1)+CR_SH*cr_special},

  {0,S_SKIP|S_END,m_null}
};

void M_DrawCredits(void)     // killough 10/98: credit screen
{
  char mbftext_s[32];
  sprintf(mbftext_s, PROJECT_STRING);
  inhelpscreens = true;
  M_DrawBackground(gamemode==shareware ? "CEIL5_1" : "MFLR8_4", screens[0]);
  M_DrawTitle(42,9,"MBFTEXT",mbftext_s);
  V_MarkRect(0,0,SCREENWIDTH,SCREENHEIGHT);
  M_DrawScreenItems(cred_settings);
}

// [FG] support more joystick and mouse buttons

static inline int GetButtons(const unsigned int max, int data)
{
	int i;

	for (i = 0; i < max; ++i)
	{
		if (data & (1 << i))
		{
			return i;
		}
	}

	return -1;
}

//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int             ch;
    int             key;
    int             i;
    static  int     mousewait = 0;
    static  int     mousey = 0;
    static  int     lasty = 0;
    static  int     mousex = 0;
    static  int     lastx = 0;

    // In testcontrols mode, none of the function keys should do anything
    // - the only key is escape to quit.

    if (testcontrols)
    {
        if (ev->type == ev_quit
         || (ev->type == ev_keydown
          && (ev->data1 == key_menu_activate || ev->data1 == key_menu_quit)))
        {
            I_Quit();
            return true;
        }

        return false;
    }

    // "close" button pressed on window?
    if (ev->type == ev_quit)
    {
        // First click on close button = bring up quit confirm message.
        // Second click on close button = confirm quit

        if (menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
            M_QuitResponse(key_menu_confirm);
        }
        else
        {
            S_StartSound(NULL,sfx_swtchn);
            M_QuitDOOM(0);
        }

        return true;
    }

    // key is the key pressed, ch is the actual character typed

    ch = 0;
    key = -1;

    if (ev->type == ev_joystick)
    {
        // Simulate key presses from joystick events to interact with the menu.

        if (menuactive)
        {
            if (ev->data3 < 0)
            {
                key = key_menu_up;
                joywait = I_GetTime() + 5;
            }
            else if (ev->data3 > 0)
            {
                key = key_menu_down;
                joywait = I_GetTime() + 5;
            }
            if (ev->data2 < 0)
            {
                key = key_menu_left;
                joywait = I_GetTime() + 2;
            }
            else if (ev->data2 > 0)
            {
                key = key_menu_right;
                joywait = I_GetTime() + 2;
            }

#define JOY_BUTTON_MAPPED(x) ((x) >= 0)
#define JOY_BUTTON_PRESSED(x) (JOY_BUTTON_MAPPED(x) && (ev->data1 & (1 << (x))) != 0)

            if (JOY_BUTTON_PRESSED(joybfire))
            {
                // Simulate a 'Y' keypress when Doom show a Y/N dialog with Fire button.
                if (messageToPrint && messageNeedsInput)
                {
                    key = key_menu_confirm;
                }
                // Simulate pressing "Enter" when we are supplying a save slot name
                else if (saveStringEnter)
                {
                    key = KEY_ENTER;
                }
                else
                {
                    // if selecting a save slot via joypad, set a flag
                    if (currentMenu == &SaveDef)
                    {
                        joypadSave = true;
                    }
                    key = key_menu_forward;
                }
                joywait = I_GetTime() + 5;
            }
            if (JOY_BUTTON_PRESSED(joybuse))
            {
                // Simulate a 'N' keypress when Doom show a Y/N dialog with Use button.
                if (messageToPrint && messageNeedsInput)
                {
                    key = key_menu_abort;
                }
                // If user was entering a save name, back out
                else if (saveStringEnter)
                {
                    key = KEY_ESCAPE;
                }
                else
                {
                    key = key_menu_back;
                }
                joywait = I_GetTime() + 5;
            }
        }
        if (JOY_BUTTON_PRESSED(joybmenu))
        {
            key = key_menu_activate;
            joywait = I_GetTime() + 5;
        }
    }
    else
    {
	if (ev->type == ev_mouse && mousewait < I_GetTime())
	{
	    // [crispy] novert disables controlling the menus with the mouse
	    if (!novert)
	    {
	    mousey += ev->data3;
	    }
	    if (mousey < lasty-30)
	    {
		key = key_menu_down;
		mousewait = I_GetTime() + 5;
		mousey = lasty -= 30;
	    }
	    else if (mousey > lasty+30)
	    {
		key = key_menu_up;
		mousewait = I_GetTime() + 5;
		mousey = lasty += 30;
	    }

	    mousex += ev->data2;
	    if (mousex < lastx-30)
	    {
		key = key_menu_left;
		mousewait = I_GetTime() + 5;
		mousex = lastx -= 30;
	    }
	    else if (mousex > lastx+30)
	    {
		key = key_menu_right;
		mousewait = I_GetTime() + 5;
		mousex = lastx += 30;
	    }

	    if (ev->data1&1)
	    {
		key = key_menu_forward;
		mousewait = I_GetTime() + 15;
	    }

	    if (ev->data1&2)
	    {
		key = key_menu_back;
		mousewait = I_GetTime() + 15;
	    }

	    // [crispy] scroll menus with mouse wheel
	    if (mousebprevweapon >= 0 && ev->data1 & (1 << mousebprevweapon))
	    {
		key = key_menu_down;
		mousewait = I_GetTime() + 1;
	    }
	    else
	    if (mousebnextweapon >= 0 && ev->data1 & (1 << mousebnextweapon))
	    {
		key = key_menu_up;
		mousewait = I_GetTime() + 1;
	    }
	}
	else
	{
	    if (ev->type == ev_keydown)
	    {
		key = ev->data1;
		ch = ev->data2;
	    }
	}
    }

    if (key == -1)
	return false;

    // Save Game string input
    if (saveStringEnter)
    {
	switch(key)
	{
	  case KEY_BACKSPACE:
	    if (saveCharIndex > 0)
	    {
		saveCharIndex--;
		savegamestrings[saveSlot][saveCharIndex] = 0;
	    }
	    break;

          case KEY_ESCAPE:
            saveStringEnter = 0;
            I_StopTextInput();
            M_StringCopy(savegamestrings[saveSlot], saveOldString,
                         SAVESTRINGSIZE);
            break;

	  case KEY_ENTER:
	    saveStringEnter = 0;
            I_StopTextInput();
	    if (savegamestrings[saveSlot][0])
		M_DoSave(saveSlot);
	    break;

	  default:
            // Savegame name entry. This is complicated.
            // Vanilla has a bug where the shift key is ignored when entering
            // a savegame name. If vanilla_keyboard_mapping is on, we want
            // to emulate this bug by using ev->data1. But if it's turned off,
            // it implies the user doesn't care about Vanilla emulation:
            // instead, use ev->data3 which gives the fully-translated and
            // modified key input.

            if (ev->type != ev_keydown)
            {
                break;
            }
            if (vanilla_keyboard_mapping)
            {
                ch = ev->data1;
            }
            else
            {
                ch = ev->data3;
            }

            ch = toupper(ch);

            if (ch != ' '
             && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
            {
                break;
            }

	    if (ch >= 32 && ch <= 127 &&
		saveCharIndex < SAVESTRINGSIZE-1 &&
		M_StringWidth(savegamestrings[saveSlot]) <
		(SAVESTRINGSIZE-2)*8)
	    {
		savegamestrings[saveSlot][saveCharIndex++] = ch;
		savegamestrings[saveSlot][saveCharIndex] = 0;
	    }
	    break;
	}
	return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
	if (messageNeedsInput)
        {
            if (key != ' ' && key != KEY_ESCAPE
             && key != key_menu_confirm && key != key_menu_abort)
            {
                return false;
            }
	}

	menuactive = messageLastMenuActive;
	if (messageRoutine)
	    messageRoutine(key);

	// [crispy] stay in menu
	if (messageToPrint < 2)
	{
	menuactive = false;
	}
	messageToPrint = 0; // [crispy] moved here
	S_StartSound(NULL,sfx_swtchx);
	return true;
    }

    // [crispy] take screen shot without weapons and HUD
    if (key != 0 && key == key_menu_cleanscreenshot)
    {
	crispy->cleanscreenshot = (screenblocks > 10) ? 2 : 1;
    }

    if ((devparm && key == key_menu_help) ||
        (key != 0 && (key == key_menu_screenshot || key == key_menu_cleanscreenshot)))
    {
	G_ScreenShot ();
	return true;
    }

    // F-Keys
    if (!menuactive)
    {
	if (key == key_menu_decscreen)      // Screen size down
        {
	    if (automapactive || chat_on)
		return false;
	    M_SizeDisplay(0);
	    S_StartSound(NULL,sfx_stnmov);
	    return true;
	}
        else if (key == key_menu_incscreen) // Screen size up
        {
	    if (automapactive || chat_on)
		return false;
	    M_SizeDisplay(1);
	    S_StartSound(NULL,sfx_stnmov);
	    return true;
	}
        else if (key == key_menu_help)     // Help key
        {
	    M_StartControlPanel ();

	    if (gameversion >= exe_ultimate)
	      currentMenu = &ReadDef2;
	    else
	      currentMenu = &ReadDef1;

	    itemOn = 0;
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
        else if (key == key_menu_save)     // Save
        {
	    M_StartControlPanel();
	    S_StartSound(NULL,sfx_swtchn);
	    M_SaveGame(0);
	    return true;
        }
        else if (key == key_menu_load)     // Load
        {
	    M_StartControlPanel();
	    S_StartSound(NULL,sfx_swtchn);
	    M_LoadGame(0);
	    return true;
        }
        else if (key == key_menu_volume)   // Sound Volume
        {
	    M_StartControlPanel ();
	    currentMenu = &SoundDef;
	    itemOn = sfx_vol;
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
        else if (key == key_menu_detail)   // Detail toggle
        {
	    M_ChangeDetail(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
        }
        else if (key == key_menu_qsave)    // Quicksave
        {
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuickSave();
	    return true;
        }
        else if (key == key_menu_endgame)  // End game
        {
	    S_StartSound(NULL,sfx_swtchn);
	    M_EndGame(0);
	    return true;
        }
        else if (key == key_menu_messages) // Toggle messages
        {
	    M_ChangeMessages(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
        }
        else if (key == key_menu_qload)    // Quickload
        {
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuickLoad();
	    return true;
        }
        else if (key == key_menu_quit)     // Quit DOOM
        {
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuitDOOM(0);
	    return true;
        }
        else if (key == key_menu_gamma)    // gamma toggle
        {
	    usegamma++;
	    if (usegamma > 4+4) // [crispy] intermediate gamma levels
		usegamma = 0;
	    players[consoleplayer].message = DEH_String(gammamsg[usegamma]);
#ifndef CRISPY_TRUECOLOR
            I_SetPalette (W_CacheLumpName (DEH_String("PLAYPAL"),PU_CACHE));
#else
            {
		extern void R_InitColormaps (void);
		I_SetPalette (0);
		R_InitColormaps();
		inhelpscreens = true;
		R_FillBackScreen();
		viewactive = false;
            }
#endif
	    return true;
	}
        // [crispy] those two can be considered as shortcuts for the IDCLEV cheat
        // and should be treated as such, i.e. add "if (!netgame)"
        else if (!netgame && key != 0 && key == key_menu_reloadlevel)
        {
	    if (G_ReloadLevel())
		return true;
        }
        else if (!netgame && key != 0 && key == key_menu_nextlevel)
        {
	    if (G_GotoNextLevel())
		return true;
        }

    }

    // Pop-up menu?
    if (!menuactive)
    {
	if (key == key_menu_activate)
	{
	    M_StartControlPanel ();
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
	return false;
    }

    // Keys usable within menu

    if (key == key_menu_down)
    {
        // Move down to next item

        do
	{
	    if (itemOn+1 > currentMenu->numitems-1)
		itemOn = 0;
	    else itemOn++;
	    S_StartSound(NULL,sfx_pstop);
	} while(currentMenu->menuitems[itemOn].status==-1);

	return true;
    }
    else if (key == key_menu_up)
    {
        // Move back up to previous item

	do
	{
	    if (!itemOn)
		itemOn = currentMenu->numitems-1;
	    else itemOn--;
	    S_StartSound(NULL,sfx_pstop);
	} while(currentMenu->menuitems[itemOn].status==-1);

	return true;
    }
    else if (key == key_menu_left)
    {
        // Slide slider left

	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status == 2)
	{
	    S_StartSound(NULL,sfx_stnmov);
	    currentMenu->menuitems[itemOn].routine(0);
	}
	return true;
    }
    else if (key == key_menu_right)
    {
        // Slide slider right

	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status == 2)
	{
	    S_StartSound(NULL,sfx_stnmov);
	    currentMenu->menuitems[itemOn].routine(1);
	}
	return true;
    }
    else if (key == key_menu_forward)
    {
        // Activate menu item

	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status)
	{
	    currentMenu->lastOn = itemOn;
	    if (currentMenu->menuitems[itemOn].status == 2)
	    {
		currentMenu->menuitems[itemOn].routine(1);      // right arrow
		S_StartSound(NULL,sfx_stnmov);
	    }
	    else
	    {
		currentMenu->menuitems[itemOn].routine(itemOn);
		S_StartSound(NULL,sfx_pistol);
	    }
	}
	return true;
    }
    else if (key == key_menu_activate)
    {
        // Deactivate menu

	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	S_StartSound(NULL,sfx_swtchx);
	return true;
    }
    else if (key == key_menu_back)
    {
        // Go back to previous menu

	currentMenu->lastOn = itemOn;
	if (currentMenu->prevMenu)
	{
	    currentMenu = currentMenu->prevMenu;
	    itemOn = currentMenu->lastOn;
	    S_StartSound(NULL,sfx_swtchn);
	}
	return true;
    }
    // [crispy] delete a savegame
    else if (key == key_menu_del)
    {
	if (currentMenu == &LoadDef || currentMenu == &SaveDef)
	{
	    if (LoadMenu[itemOn].status)
	    {
		currentMenu->lastOn = itemOn;
		M_ConfirmDeleteGame();
		return true;
	    }
	    else
	    {
		S_StartSound(NULL,sfx_oof);
	    }
	}
    }
    // [crispy] next/prev Crispness menu
    else if (key == KEY_PGUP) // [Nugget] Add arrow support?
    {
	currentMenu->lastOn = itemOn;
	if (currentMenu == CrispnessMenus[crispness_cur])
	{
	    M_CrispnessPrev(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
    }
    else if (key == KEY_PGDN) // [Nugget] Add arrow support?
    {
	currentMenu->lastOn = itemOn;
	if (currentMenu == CrispnessMenus[crispness_cur])
	{
	    M_CrispnessNext(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
    }

    // Keyboard shortcut?
    // Vanilla Doom has a weird behavior where it jumps to the scroll bars
    // when the certain keys are pressed, so emulate this.

    else if (ch != 0 || IsNullKey(key))
    {
	for (i = itemOn+1;i < currentMenu->numitems;i++)
        {
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
		itemOn = i;
		S_StartSound(NULL,sfx_pstop);
		return true;
	    }
        }

	for (i = 0;i <= itemOn;i++)
        {
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
		itemOn = i;
		S_StartSound(NULL,sfx_pstop);
		return true;
	    }
        }
    }

    return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
    // intro might call this repeatedly
    if (menuactive)
	return;

    // [crispy] entering menus while recording demos pauses the game
    if (demorecording && !paused)
        sendpause = true;

    menuactive = 1;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC
}

// Display OPL debug messages - hack for GENMIDI development.

static void M_DrawOPLDev(void)
{
    extern void I_OPL_DevMessages(char *, size_t);
    char debug[1024];
    char *curr, *p;
    int line;

    I_OPL_DevMessages(debug, sizeof(debug));
    curr = debug;
    line = 0;

    for (;;)
    {
        p = strchr(curr, '\n');

        if (p != NULL)
        {
            *p = '\0';
        }

        M_WriteText(0, line * 8, curr);
        ++line;

        if (p == NULL)
        {
            break;
        }

        curr = p + 1;
    }
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
    static short	x;
    static short	y;
    unsigned int	i;
    unsigned int	max;
    char		string[80];
    const char          *name;
    int			start;

    inhelpscreens = false;

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
	// [crispy] draw a background for important questions
	if (messageToPrint == 2)
	{
	    M_DrawCrispnessBackground();
	}

	start = 0;
	y = ORIGHEIGHT/2 - M_StringHeight(messageString) / 2;
	while (messageString[start] != '\0')
	{
	    boolean foundnewline = false;

            for (i = 0; messageString[start + i] != '\0'; i++)
            {
                if (messageString[start + i] == '\n')
                {
                    M_StringCopy(string, messageString + start,
                                 sizeof(string));
                    if (i < sizeof(string))
                    {
                        string[i] = '\0';
                    }

                    foundnewline = true;
                    start += i + 1;
                    break;
                }
            }

            if (!foundnewline)
            {
                M_StringCopy(string, messageString + start, sizeof(string));
                start += strlen(string);
            }

	    x = ORIGWIDTH/2 - M_StringWidth(string) / 2;
	    M_WriteText(x > 0 ? x : 0, y, string); // [crispy] prevent negative x-coords
	    y += SHORT(hu_font[0]->height);
	}

	return;
    }

    if (opldev)
    {
        M_DrawOPLDev();
    }

    if (!menuactive)
	return;

    if (currentMenu->routine)
	currentMenu->routine();         // call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;

    // [crispy] check current menu for missing menu graphics lumps - only once
    if (currentMenu->lumps_missing == 0)
    {
        for (i = 0; i < max; i++)
            if (currentMenu->menuitems[i].name[0])
                if (W_CheckNumForName(currentMenu->menuitems[i].name) < 0)
                    currentMenu->lumps_missing++;

        // [crispy] no lump missing, no need to check again
        if (currentMenu->lumps_missing == 0)
            currentMenu->lumps_missing = -1;
    }

    for (i=0;i<max;i++)
    {
        const char *alttext = currentMenu->menuitems[i].alttext;
        name = DEH_String(currentMenu->menuitems[i].name);

	if (name[0] && (W_CheckNumForName(name) > 0 || alttext))
	{
	    if (W_CheckNumForName(name) > 0 && currentMenu->lumps_missing == -1)
	    V_DrawPatchDirect (x, y, W_CacheLumpName(name, PU_CACHE));
	    else if (alttext)
		M_WriteText(x, y+8-(M_StringHeight(alttext)/2), alttext);
	}
	y += LINEHEIGHT;
    }


    // DRAW SKULL
    if (currentMenu == CrispnessMenus[crispness_cur])
    {
	char item[4];
	M_snprintf(item, sizeof(item), "%s>", whichSkull ? crstr[CR_NONE] : crstr[CR_DARK]);
	M_WriteText(currentMenu->x - 8, currentMenu->y + CRISPY_LINEHEIGHT * itemOn, item);
	dp_translation = NULL;
    }
    else
    V_DrawPatchDirect(x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT,
		      W_CacheLumpName(DEH_String(skullName[whichSkull]),
				      PU_CACHE));
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
    menuactive = 0;

    // [crispy] entering menus while recording demos pauses the game
    if (demorecording && paused)
        sendpause = true;

    // if (!netgame && usergame && paused)
    //       sendpause = true;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
void M_Ticker (void)
{
    if (--skullAnimCounter <= 0)
    {
	whichSkull ^= 1;
	skullAnimCounter = 8;
    }
}


//
// M_Init
//
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive;
    quickSaveSlot = -1;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.

    // The same hacks were used in the original Doom EXEs.

    if (gameversion >= exe_ultimate)
    {
        MainMenu[readthis].routine = M_ReadThis2;
        ReadDef2.prevMenu = NULL;
    }

    if (gameversion >= exe_final && gameversion <= exe_final2)
    {
        ReadDef2.routine = M_DrawReadThisCommercial;
        // [crispy] rearrange Skull in Final Doom HELP screen
        ReadDef2.y -= 10;
    }

    if (gamemode == commercial)
    {
        MainMenu[readthis] = MainMenu[quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        NewDef.prevMenu = &MainDef;
        ReadDef1.routine = M_DrawReadThisCommercial;
        ReadDef1.x = 330;
        ReadDef1.y = 165;
        ReadMenu1[rdthsempty1].routine = M_FinishReadThis;
    }

    // [crispy] Sigil
    if (!crispy->haved1e5)
    {
        EpiDef.numitems = 4;
    }

    // Versions of doom.exe before the Ultimate Doom release only had
    // three episodes; if we're emulating one of those then don't try
    // to show episode four. If we are, then do show episode four
    // (should crash if missing).
    if (gameversion < exe_ultimate)
    {
        EpiDef.numitems--;
    }
    // chex.exe shows only one episode.
    else if (gameversion == exe_chex)
    {
        EpiDef.numitems = 1;
        // [crispy] never show the Episode menu
        NewDef.prevMenu = &MainDef;
    }

    // [crispy] NRFTL / The Master Levels
    if (crispy->havenerve || crispy->havemaster)
    {
        int i, j;

        NewDef.prevMenu = &EpiDef;
        EpisodeMenu[0].alphaKey = gamevariant == freedm ||
                                  gamevariant == freedoom ?
                                 'f' :
                                 'h';
        EpisodeMenu[0].alttext = gamevariant == freedm ?
                                 "FreeDM" :
                                 gamevariant == freedoom ?
                                 "Freedoom: Phase 2" :
                                 "Hell on Earth";
        EpiDef.numitems = 1;

        if (crispy->havenerve)
        {
            EpisodeMenu[EpiDef.numitems].alphaKey = 'n';
            EpisodeMenu[EpiDef.numitems].alttext = "No Rest for the Living";
            EpiDef.numitems++;

            i = W_CheckNumForName("M_EPI1");
            j = W_CheckNumForName("M_EPI2");

            // [crispy] render the episode menu with the HUD font ...
            // ... if the graphics are not available
            if (i != -1 && j != -1)
            {
                // ... or if the graphics are both from an IWAD
                if (W_IsIWADLump(lumpinfo[i]) && W_IsIWADLump(lumpinfo[j]))
                {
                    const patch_t *pi, *pj;

                    pi = W_CacheLumpNum(i, PU_CACHE);
                    pj = W_CacheLumpNum(j, PU_CACHE);

                    // ... and if the patch width for "Hell on Earth"
                    //     is longer than "No Rest for the Living"
                    if (SHORT(pi->width) > SHORT(pj->width))
                    {
                        EpiDef.lumps_missing = 1;
                    }
                }
            }
            else
            {
                EpiDef.lumps_missing = 1;
            }
        }

        if (crispy->havemaster)
        {
            EpisodeMenu[EpiDef.numitems].alphaKey = 't';
            EpisodeMenu[EpiDef.numitems].alttext = "The Master Levels";
            EpiDef.numitems++;

            i = W_CheckNumForName(EpiDef.numitems == 3 ? "M_EPI3" : "M_EPI2");

            // [crispy] render the episode menu with the HUD font
            // if the graphics are not available or not from a PWAD
            if (i == -1 || W_IsIWADLump(lumpinfo[i]))
            {
                EpiDef.lumps_missing = 1;
            }
        }
    }

    // [crispy] rearrange Load Game and Save Game menus
    {
	const patch_t *patchl, *patchs, *patchm;
	short captionheight, vstep;

	patchl = W_CacheLumpName(DEH_String("M_LOADG"), PU_CACHE);
	patchs = W_CacheLumpName(DEH_String("M_SAVEG"), PU_CACHE);
	patchm = W_CacheLumpName(DEH_String("M_LSLEFT"), PU_CACHE);

	LoadDef_x = (ORIGWIDTH - SHORT(patchl->width)) / 2 + SHORT(patchl->leftoffset);
	SaveDef_x = (ORIGWIDTH - SHORT(patchs->width)) / 2 + SHORT(patchs->leftoffset);
	LoadDef.x = SaveDef.x = (ORIGWIDTH - 24 * 8) / 2 + SHORT(patchm->leftoffset); // [crispy] see M_DrawSaveLoadBorder()

	captionheight = MAX(SHORT(patchl->height), SHORT(patchs->height));

	vstep = ORIGHEIGHT - 32; // [crispy] ST_HEIGHT
	vstep -= captionheight;
	vstep -= (load_end - 1) * LINEHEIGHT + SHORT(patchm->height);
	vstep /= 3;

	if (vstep > 0)
	{
		LoadDef_y = vstep + captionheight - SHORT(patchl->height) + SHORT(patchl->topoffset);
		SaveDef_y = vstep + captionheight - SHORT(patchs->height) + SHORT(patchs->topoffset);
		LoadDef.y = SaveDef.y = vstep + captionheight + vstep + SHORT(patchm->topoffset) - 7; // [crispy] see M_DrawSaveLoadBorder()
		MouseDef.y = LoadDef.y;
	}
    }

    // [crispy] remove DOS reference from the game quit confirmation dialogs
    if (!M_ParmExists("-nodeh"))
    {
	const char *string;
	char *replace;

	// [crispy] "i wouldn't leave if i were you.\ndos is much worse."
	string = doom1_endmsg[3];
	if (!DEH_HasStringReplacement(string))
	{
		replace = M_StringReplace(string, "dos", crispy->platform);
		DEH_AddStringReplacement(string, replace);
		free(replace);
	}

	// [crispy] "you're trying to say you like dos\nbetter than me, right?"
	string = doom1_endmsg[4];
	if (!DEH_HasStringReplacement(string))
	{
		replace = M_StringReplace(string, "dos", crispy->platform);
		DEH_AddStringReplacement(string, replace);
		free(replace);
	}

	// [crispy] "don't go now, there's a \ndimensional shambler waiting\nat the dos prompt!"
	string = doom2_endmsg[2];
	if (!DEH_HasStringReplacement(string))
	{
		replace = M_StringReplace(string, "dos", "command");
		DEH_AddStringReplacement(string, replace);
		free(replace);
	}
    }

    opldev = M_CheckParm("-opldev") > 0;
}

// [crispy] extended savegames
static char *savegwarning;
static void M_ForceLoadGameResponse(int key)
{
	free(savegwarning);
	free(savewadfilename);

	if (key != key_menu_confirm || !savemaplumpinfo)
	{
		// [crispy] no need to end game anymore when denied to load savegame
		//M_EndGameResponse(key_menu_confirm);
		savewadfilename = NULL;

		// [crispy] reload Load Game menu
		M_StartControlPanel();
		M_LoadGame(0);
		return;
	}

	savewadfilename = (char *)W_WadNameForLump(savemaplumpinfo);
	gameaction = ga_loadgame;
}

void M_ForceLoadGame()
{
	savegwarning =
	savemaplumpinfo ?
	M_StringJoin("This savegame requires the file\n",
	             crstr[CR_GOLD], savewadfilename, crstr[CR_NONE], "\n",
	             "to restore ", crstr[CR_GOLD], savemaplumpinfo->name, crstr[CR_NONE], " .\n\n",
	             "Continue to restore from\n",
	             crstr[CR_GOLD], W_WadNameForLump(savemaplumpinfo), crstr[CR_NONE], " ?\n\n",
	             PRESSYN, NULL) :
	M_StringJoin("This savegame requires the file\n",
	             crstr[CR_GOLD], savewadfilename, crstr[CR_NONE], "\n",
	             "to restore a map that is\n",
	             "currently not available!\n\n",
	             PRESSKEY, NULL) ;

	M_StartMessage(savegwarning, M_ForceLoadGameResponse, savemaplumpinfo != NULL);
	messageToPrint = 2;
	S_StartSound(NULL,sfx_swtchn);
}

static void M_ConfirmDeleteGameResponse (int key)
{
	free(savegwarning);

	if (key == key_menu_confirm)
	{
		char name[256];

		M_StringCopy(name, P_SaveGameFile(itemOn), sizeof(name));
		remove(name);

		M_ReadSaveStrings();
	}
}

void M_ConfirmDeleteGame ()
{
	savegwarning =
	M_StringJoin("delete savegame\n\n",
	             crstr[CR_GOLD], savegamestrings[itemOn], crstr[CR_NONE], " ?\n\n",
	             PRESSYN, NULL);

	M_StartMessage(savegwarning, M_ConfirmDeleteGameResponse, true);
	messageToPrint = 2;
	S_StartSound(NULL,sfx_swtchn);
}

// [crispy] indicate game version mismatch
void M_LoadGameVerMismatch ()
{
	M_StartMessage("Game Version Mismatch\n\n"PRESSKEY, NULL, false);
	messageToPrint = 2;
	S_StartSound(NULL,sfx_swtchn);
}
