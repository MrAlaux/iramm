//
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014 Fabian Greffrath
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
//
// Parses [CODEPTR] sections in BEX files
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "info.h"

#include "deh_io.h"
#include "deh_main.h"

extern void A_Light0();
extern void A_WeaponReady();
extern void A_Lower();
extern void A_Raise();
extern void A_Punch();
extern void A_ReFire();
extern void A_FirePistol();
extern void A_Light1();
extern void A_FireShotgun();
extern void A_Light2();
extern void A_FireShotgun2();
extern void A_CheckReload();
extern void A_OpenShotgun2();
extern void A_LoadShotgun2();
extern void A_CloseShotgun2();
extern void A_FireCGun();
extern void A_GunFlash();
extern void A_FireMissile();
extern void A_Saw();
extern void A_FirePlasma();
extern void A_BFGsound();
extern void A_FireBFG();
extern void A_BFGSpray();
extern void A_Explode();
extern void A_Pain();
extern void A_PlayerScream();
extern void A_Fall();
extern void A_XScream();
extern void A_Look();
extern void A_Chase();
extern void A_FaceTarget();
extern void A_PosAttack();
extern void A_Scream();
extern void A_SPosAttack();
extern void A_VileChase();
extern void A_VileStart();
extern void A_VileTarget();
extern void A_VileAttack();
extern void A_StartFire();
extern void A_Fire();
extern void A_FireCrackle();
extern void A_Tracer();
extern void A_SkelWhoosh();
extern void A_SkelFist();
extern void A_SkelMissile();
extern void A_FatRaise();
extern void A_FatAttack1();
extern void A_FatAttack2();
extern void A_FatAttack3();
extern void A_BossDeath();
extern void A_CPosAttack();
extern void A_CPosRefire();
extern void A_TroopAttack();
extern void A_SargAttack();
extern void A_HeadAttack();
extern void A_BruisAttack();
extern void A_SkullAttack();
extern void A_Metal();
extern void A_SpidRefire();
extern void A_BabyMetal();
extern void A_BspiAttack();
extern void A_Hoof();
extern void A_CyberAttack();
extern void A_PainAttack();
extern void A_PainDie();
extern void A_KeenDie();
extern void A_BrainPain();
extern void A_BrainScream();
extern void A_BrainDie();
extern void A_BrainAwake();
extern void A_BrainSpit();
extern void A_SpawnSound();
extern void A_SpawnFly();
extern void A_BrainExplode();
// [crispy] additional BOOM and MBF states, sprites and code pointers
extern void A_Stop();
extern void A_Die();
extern void A_FireOldBFG();
extern void A_Detonate();
extern void A_Mushroom();
extern void A_BetaSkullAttack();
// [crispy] more MBF code pointers
extern void A_Spawn();
extern void A_Turn();
extern void A_Face();
extern void A_Scratch();
extern void A_PlaySound();
extern void A_RandomJump();
extern void A_LineEffect();

// [Nugget] Add MBF21 (taken from Woof!) (1)
// [XA] New mbf21 codepointers

extern void A_SpawnObject();
extern void A_MonsterProjectile();
extern void A_MonsterBulletAttack();
extern void A_MonsterMeleeAttack();
extern void A_RadiusDamage();
extern void A_NoiseAlert();
extern void A_HealChase();
extern void A_SeekTracer();
extern void A_FindTracer();
extern void A_ClearTracer();
extern void A_JumpIfHealthBelow();
extern void A_JumpIfTargetInSight();
extern void A_JumpIfTargetCloser();
extern void A_JumpIfTracerInSight();
extern void A_JumpIfTracerCloser();
extern void A_JumpIfFlagsSet();
extern void A_AddFlags();
extern void A_RemoveFlags();
extern void A_WeaponProjectile();
extern void A_WeaponBulletAttack();
extern void A_WeaponMeleeAttack();
extern void A_WeaponSound();
extern void A_WeaponAlert();
extern void A_WeaponJump();
extern void A_ConsumeAmmo();
extern void A_CheckAmmo();
extern void A_RefireTo();
extern void A_GunFlashTo();

typedef struct {
  actionf_t cptr;  // actual pointer to the subroutine
  char *lookup;  // mnemonic lookup string to be specified in BEX
  // mbf21
  int argcount;  // [XA] number of mbf21 args this action uses, if any
  long default_args[MAXSTATEARGS]; // default values for mbf21 args
} bex_codeptr_t;

static const bex_codeptr_t bex_codeptrtable[] = {
  {A_Light0,         "A_Light0"},
  {A_WeaponReady,    "A_WeaponReady"},
  {A_Lower,          "A_Lower"},
  {A_Raise,          "A_Raise"},
  {A_Punch,          "A_Punch"},
  {A_ReFire,         "A_ReFire"},
  {A_FirePistol,     "A_FirePistol"},
  {A_Light1,         "A_Light1"},
  {A_FireShotgun,    "A_FireShotgun"},
  {A_Light2,         "A_Light2"},
  {A_FireShotgun2,   "A_FireShotgun2"},
  {A_CheckReload,    "A_CheckReload"},
  {A_OpenShotgun2,   "A_OpenShotgun2"},
  {A_LoadShotgun2,   "A_LoadShotgun2"},
  {A_CloseShotgun2,  "A_CloseShotgun2"},
  {A_FireCGun,       "A_FireCGun"},
  {A_GunFlash,       "A_GunFlash"},
  {A_FireMissile,    "A_FireMissile"},
  {A_Saw,            "A_Saw"},
  {A_FirePlasma,     "A_FirePlasma"},
  {A_BFGsound,       "A_BFGsound"},
  {A_FireBFG,        "A_FireBFG"},
  {A_BFGSpray,       "A_BFGSpray"},
  {A_Explode,        "A_Explode"},
  {A_Pain,           "A_Pain"},
  {A_PlayerScream,   "A_PlayerScream"},
  {A_Fall,           "A_Fall"},
  {A_XScream,        "A_XScream"},
  {A_Look,           "A_Look"},
  {A_Chase,          "A_Chase"},
  {A_FaceTarget,     "A_FaceTarget"},
  {A_PosAttack,      "A_PosAttack"},
  {A_Scream,         "A_Scream"},
  {A_SPosAttack,     "A_SPosAttack"},
  {A_VileChase,      "A_VileChase"},
  {A_VileStart,      "A_VileStart"},
  {A_VileTarget,     "A_VileTarget"},
  {A_VileAttack,     "A_VileAttack"},
  {A_StartFire,      "A_StartFire"},
  {A_Fire,           "A_Fire"},
  {A_FireCrackle,    "A_FireCrackle"},
  {A_Tracer,         "A_Tracer"},
  {A_SkelWhoosh,     "A_SkelWhoosh"},
  {A_SkelFist,       "A_SkelFist"},
  {A_SkelMissile,    "A_SkelMissile"},
  {A_FatRaise,       "A_FatRaise"},
  {A_FatAttack1,     "A_FatAttack1"},
  {A_FatAttack2,     "A_FatAttack2"},
  {A_FatAttack3,     "A_FatAttack3"},
  {A_BossDeath,      "A_BossDeath"},
  {A_CPosAttack,     "A_CPosAttack"},
  {A_CPosRefire,     "A_CPosRefire"},
  {A_TroopAttack,    "A_TroopAttack"},
  {A_SargAttack,     "A_SargAttack"},
  {A_HeadAttack,     "A_HeadAttack"},
  {A_BruisAttack,    "A_BruisAttack"},
  {A_SkullAttack,    "A_SkullAttack"},
  {A_Metal,          "A_Metal"},
  {A_SpidRefire,     "A_SpidRefire"},
  {A_BabyMetal,      "A_BabyMetal"},
  {A_BspiAttack,     "A_BspiAttack"},
  {A_Hoof,           "A_Hoof"},
  {A_CyberAttack,    "A_CyberAttack"},
  {A_PainAttack,     "A_PainAttack"},
  {A_PainDie,        "A_PainDie"},
  {A_KeenDie,        "A_KeenDie"},
  {A_BrainPain,      "A_BrainPain"},
  {A_BrainScream,    "A_BrainScream"},
  {A_BrainDie,       "A_BrainDie"},
  {A_BrainAwake,     "A_BrainAwake"},
  {A_BrainSpit,      "A_BrainSpit"},
  {A_SpawnSound,     "A_SpawnSound"},
  {A_SpawnFly,       "A_SpawnFly"},
  {A_BrainExplode,   "A_BrainExplode"},
  {A_Detonate,       "A_Detonate"},       // killough 8/9/98
  {A_Mushroom,       "A_Mushroom"},       // killough 10/98
  {A_Die,            "A_Die"},            // killough 11/98
  {A_Spawn,          "A_Spawn"},          // killough 11/98
  {A_Turn,           "A_Turn"},           // killough 11/98
  {A_Face,           "A_Face"},           // killough 11/98
  {A_Scratch,        "A_Scratch"},        // killough 11/98
  {A_PlaySound,      "A_PlaySound"},      // killough 11/98
  {A_RandomJump,     "A_RandomJump"},     // killough 11/98
  {A_LineEffect,     "A_LineEffect"},     // killough 11/98

  // [XA] New mbf21 codepointers
  {A_SpawnObject,         "A_SpawnObject", 8},
  {A_MonsterProjectile,   "A_MonsterProjectile", 5},
  {A_MonsterBulletAttack, "A_MonsterBulletAttack", 5, {0, 0, 1, 3, 5}},
  {A_MonsterMeleeAttack,  "A_MonsterMeleeAttack", 4, {3, 8, 0, 0}},
  {A_RadiusDamage,        "A_RadiusDamage", 2},
  {A_NoiseAlert,          "A_NoiseAlert", 0},
  {A_HealChase,           "A_HealChase", 2},
  {A_SeekTracer,          "A_SeekTracer", 2},
  {A_FindTracer,          "A_FindTracer", 2, {0, 10}},
  {A_ClearTracer,         "A_ClearTracer", 0},
  {A_JumpIfHealthBelow,   "A_JumpIfHealthBelow", 2},
  {A_JumpIfTargetInSight, "A_JumpIfTargetInSight", 2},
  {A_JumpIfTargetCloser,  "A_JumpIfTargetCloser", 2},
  {A_JumpIfTracerInSight, "A_JumpIfTracerInSight", 2},
  {A_JumpIfTracerCloser,  "A_JumpIfTracerCloser", 2},
  {A_JumpIfFlagsSet,      "A_JumpIfFlagsSet", 3},
  {A_AddFlags,            "A_AddFlags", 2},
  {A_RemoveFlags,         "A_RemoveFlags", 2},
  {A_WeaponProjectile,    "A_WeaponProjectile", 5},
  {A_WeaponBulletAttack,  "A_WeaponBulletAttack", 5, {0, 0, 1, 5, 3}},
  {A_WeaponMeleeAttack,   "A_WeaponMeleeAttack", 5, {2, 10, 1 * FRACUNIT, 0, 0}},
  {A_WeaponSound,         "A_WeaponSound", 2},
  {A_WeaponAlert,         "A_WeaponAlert", 0},
  {A_WeaponJump,          "A_WeaponJump", 2},
  {A_ConsumeAmmo,         "A_ConsumeAmmo", 1},
  {A_CheckAmmo,           "A_CheckAmmo", 2},
  {A_RefireTo,            "A_RefireTo", 2},
  {A_GunFlashTo,          "A_GunFlashTo", 2},

  // This NULL entry must be the last in the list
  {NULL,             "A_NULL"},  // Ty 05/16/98
};

static byte *defined_codeptr_args;

extern actionf_t codeptrs[NUMSTATES];

static void *DEH_BEXPtrStart(deh_context_t *context, char *line)
{
    char s[10];

    if (sscanf(line, "%9s", s) == 0 || strcmp("[CODEPTR]", s))
    {
	DEH_Warning(context, "Parse error on section start");
    }

    return NULL;
}

static void DEH_BEXPtrParseLine(deh_context_t *context, char *line, void *tag)
{
    state_t *state;
    char *variable_name, *value, frame_str[6];
    int frame_number, i;

    // parse "FRAME nn = mnemonic", where
    // variable_name = "FRAME nn" and value = "mnemonic"
    if (!DEH_ParseAssignment(line, &variable_name, &value))
    {
	DEH_Warning(context, "Failed to parse assignment: %s", line);
	return;
    }

    // parse "FRAME nn", where frame_number = "nn"
    if (sscanf(variable_name, "%5s %32d", frame_str, &frame_number) != 2 ||
        strcasecmp(frame_str, "FRAME"))
    {
	DEH_Warning(context, "Failed to parse assignment: %s", variable_name);
	return;
    }

    if (frame_number < 0 || frame_number >= NUMSTATES)
    {
	DEH_Warning(context, "Invalid frame number: %i", frame_number);
	return;
    }

    state = (state_t *) &states[frame_number];

    for (i = 0; i < arrlen(bex_codeptrtable); i++)
    {
	if (!strcasecmp(bex_codeptrtable[i].mnemonic, value))
	{
	    state->action = bex_codeptrtable[i].pointer;
	    return;
	}
    }

    DEH_Warning(context, "Invalid mnemonic '%s'", value);
}

deh_section_t deh_section_bexptr =
{
    "[CODEPTR]",
    NULL,
    DEH_BEXPtrStart,
    DEH_BEXPtrParseLine,
    NULL,
    NULL,
};
