/* File: wizard1.c */

/* Generation of object, artifact, and monster spoilers.
 *
 * This file has been updated to work with Oangband 0.5.1.
 *
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "cmds.h"

#ifdef ALLOW_SPOILERS


/*
 * The spoiler file being created
 */
static FILE *fff = NULL;




/*
 * Describe the kind
 */
static void kind_info(char *buf, char *dam, char *wgt, int *lev, s32b *val, int k)
{
        object_kind *k_ptr;

  object_type *i_ptr;
  object_type object_type_body;
  
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Prepare a fake item */
  object_prep(i_ptr, k);
  
  /* Obtain the "kind" info */
  k_ptr = &k_info[i_ptr->k_idx];
  
  /* It is known */
  i_ptr->ident |= (IDENT_KNOWN);
  
  /* Cancel bonuses */
  i_ptr->pval = 0;
  i_ptr->to_a = 0;
  i_ptr->to_h = 0;
  i_ptr->to_d = 0;
  
  
  /* Level */
  (*lev) = k_ptr->level;
  
  /* Value */
  (*val) = object_value(i_ptr);
  
  
  /* Hack */
  if (!buf || !dam || !wgt) return;
  
  
  /* Description (too brief) */
  object_desc_store(buf, i_ptr, FALSE, 0);
  
  
  /* Misc info */
  strcpy(dam, "");
  
  /* Damage */
  switch (i_ptr->tval)
    {
      /* Bows */
    case TV_BOW:
      {
			break;
      }
      
      /* Ammo */
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
      {
	sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
	break;
      }
      
      /* Weapons */
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING:
      {
	sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
	break;
      }
      
      /* Armour */
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
		case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
      {
	sprintf(dam, "%d", i_ptr->ac);
	break;
      }
    }
  
  
  /* Weight */
  if (use_metric) sprintf(wgt, "%3d.%d", make_metric(i_ptr->weight) / 10, 
			  make_metric(i_ptr->weight) % 10);
  else sprintf(wgt, "%3d.%d", i_ptr->weight / 10, i_ptr->weight % 10);
  
}


/*
 * Create a spoiler file for items
 */
static void spoil_obj_desc(cptr fname)
{
  int i, k, s, t, n = 0;
  
  u16b who[200];
  
  char buf[1024];
  
  char wgt[80];
  char dam[80];
  
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);

  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
  if (!fff)
    {
      msg_print("Cannot create spoiler file.");
      return;
    }
  
  
  /* Header */
  fprintf(fff, "Spoiler File -- Basic Items (2.?.?)\n\n\n");
  
  /* More Header */
  fprintf(fff, "%-45s     %8s%7s%5s%9s\n",
	  "Description", "Dam/AC", "Wgt", "Lev", "Cost");
  fprintf(fff, "%-45s     %8s%7s%5s%9s\n",
	  "----------------------------------------",
	  "------", "---", "---", "----");
  
  /* List the groups */
  for (i = 0; TRUE; i++)
    {
      /* Write out the group title */
      if (group_item[i].name)
	{
	  /* Hack -- bubble-sort by cost and then level */
	  for (s = 0; s < n - 1; s++)
	    {
	      for (t = 0; t < n - 1; t++)
		{
		  int i1 = t;
		  int i2 = t + 1;
		  
		  int e1;
		  int e2;
		  
		  s32b t1;
		  s32b t2;
		  
		  kind_info(NULL, NULL, NULL, &e1, &t1, who[i1]);
		  kind_info(NULL, NULL, NULL, &e2, &t2, who[i2]);
		  
		  if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
		    {
		      int tmp = who[i1];
		      who[i1] = who[i2];
		      who[i2] = tmp;
		    }
		}
	    }
	  
	  /* Spoil each item */
	  for (s = 0; s < n; s++)
	    {
	      int e;
	      s32b v;
	      
	      /* Describe the kind */
	      kind_info(buf, dam, wgt, &e, &v, who[s]);
	      
	      /* Dump it */
	      fprintf(fff, "     %-45s%8s%7s%5d%9ld\n",
		      buf, dam, wgt, e, (long)(v));
	    }
	  
	  /* Start a new set */
	  n = 0;
	  
	  /* Notice the end */
	  if (!group_item[i].tval) break;
	  
	  /* Start a new set */
	  fprintf(fff, "\n\n%s\n\n", group_item[i].name);
                }

                /* Acquire legal item types */
      for (k = 1; k < z_info->k_max; k++)
                {
                        object_kind *k_ptr = &k_info[k];

	  /* Skip wrong tval's */
	  if (k_ptr->tval != group_item[i].tval) continue;
	  
	  /* Hack -- Skip instant-artifacts */
	  if (k_ptr->flags3 & (TR3_INSTA_ART)) continue;
	  
	  /* Save the index */
	  who[n++] = k;
	}
    }
  
  
  /* Check for errors */
  if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
      return;
    }
  
  /* Message */
  msg_print("Successfully created a spoiler file.");
}



/*
 * Artifact Spoilers by: randy@PICARD.tamu.edu (Randy Hutson)
 */


/*
 * Returns a "+" string if a number is non-negative and an empty
 * string if negative
 */
#define POSITIZE(v) (((v) >= 0) ? "+" : "")

/*
 * These are used to format the artifact spoiler file. INDENT1 is used
 * to indent all but the first line of an artifact spoiler. INDENT2 is
 * used when a line "wraps". (Bladeturner's resistances cause this.)
 */
#define INDENT1 "    "
#define INDENT2 "      "

/*
 * MAX_LINE_LEN specifies when a line should wrap.
 */
#define MAX_LINE_LEN 75


/*
 * The artifacts categorized by type
 */
static grouper group_artifact[] =
{
  { TV_SWORD,	"Edged Weapons" },
  { TV_POLEARM,	"Polearms" },
  { TV_HAFTED,	"Hafted Weapons" },
  { TV_BOW,	"Bows" },

  { TV_SOFT_ARMOR,	"Body Armor" },
  { TV_HARD_ARMOR,	 NULL },
  { TV_DRAG_ARMOR,	 NULL },

  { TV_CLOAK,	"Cloaks" },
  { TV_SHIELD,	"Shields" },
  { TV_HELM,	"Helms/Crowns" },
  { TV_CROWN,	 NULL },
  { TV_GLOVES,	"Gloves" },
  { TV_BOOTS,	"Boots" },
  
  { TV_LITE,	"Light Sources" },
  { TV_AMULET,	"Amulets" },
  { TV_RING,	"Rings" },

  { 0, NULL }
};



/*
 * Pair together a constant flag with a textual description.
 *
 * Used by both "init.c" and "wiz-spo.c".
 *
 * Note that it sometimes more efficient to actually make an array
 * of textual names, where entry 'N' is assumed to be paired with
 * the flag whose value is "1L << N", but that requires hard-coding.
 */

typedef struct flag_desc flag_desc;

struct flag_desc
{
  const u32b flag;
  const char *const desc;
};



/*
 * These are used for "+3 to STR, DEX", etc. These are separate from
 * the other pval affected traits to simplify the case where an object
 * affects all stats.  In this case, "All stats" is used instead of
 * listing each stat individually.
 */

static flag_desc stat_flags_desc[] =
{
  { TR1_STR,	  "STR" },
  { TR1_INT,	  "INT" },
  { TR1_WIS,	  "WIS" },
  { TR1_DEX,	  "DEX" },
  { TR1_CON,	  "CON" },
  { TR1_CHR,	  "CHR" }
};

/*
 * Besides stats, these are the other player traits
 * which may be affected by an object's pval
 */

static flag_desc pval_flags1_desc[] =
{
  { TR1_MAGIC_MASTERY,  "Magical Item Skill" },
  { TR1_STEALTH,	 "Stealth" },
  { TR1_SEARCH,	         "Searching" },
  { TR1_INFRA,	         "Infravision" },
  { TR1_TUNNEL,	         "Tunneling" },
  { TR1_SPEED,	         "Speed" },
};

/*
 * Missile weapon attributes.
 */
static flag_desc launcher_flags_desc[] = 
{
  { TR1_MIGHT2,	     "+2 Extra Might" },
  { TR1_SHOTS,	     "Extra Shots" },
  { TR1_MIGHT1,	     "+1 Extra Might" },
};


/*
 * Slaying preferences for weapons
 */

static flag_desc slay_flags_desc[] =
{
  { TR1_SLAY_ANIMAL,	  "Animal" },
  { TR1_SLAY_EVIL,	  "Evil" },
  { TR1_SLAY_UNDEAD,	  "Undead" },
  { TR1_SLAY_DEMON,	  "Demon" },
  { TR1_SLAY_ORC,	  "Orc" },
  { TR1_SLAY_TROLL,	  "Troll" },
  { TR1_SLAY_GIANT,	  "Giant" },
  { TR1_SLAY_DRAGON,	  "Dragon" },
};

/*
 * Elemental brands for weapons
 */
static flag_desc brand_flags_desc[] =
{
  { TR1_BRAND_ACID,	  "Acid Brand" },
  { TR1_BRAND_ELEC,	  "Lightning Brand" },
  { TR1_BRAND_FIRE,	  "Flame Tongue" },
  { TR1_BRAND_COLD,	  "Frost Brand" },
  { TR1_BRAND_POIS,	  "Poison Brand" },
};

/*
 * The basic resistances
 */

static const flag_desc resist_flags_desc[] =
{
  { TR2_RES_ACID,   "Acid" },
  { TR2_RES_ELEC,   "Lightning" },
        { TR2_RES_FIRE,   "Fire" },
        { TR2_RES_COLD,   "Cold" },
        { TR2_RES_POIS,   "Poison" },
        { TR2_RES_FEAR,   "Fear" },
        { TR2_RES_LITE,   "Light" },
        { TR2_RES_DARK,   "Dark" },
        { TR2_RES_BLIND,  "Blindness" },
        { TR2_RES_CONFU,  "Confusion" },
        { TR2_RES_SOUND,  "Sound" },
        { TR2_RES_SHARD,  "Shards" },
  { TR2_RES_NEXUS,  "Nexus" },
  { TR2_RES_NETHR,  "Nether" },
  { TR2_RES_CHAOS,  "Chaos" },
  { TR2_RES_DISEN,  "Disenchantment" },
};

/*
 * Elemental immunities (along with poison)
 */

static const flag_desc immune_flags_desc[] =
{
  { TR2_IM_ACID,  "Acid" },
  { TR2_IM_ELEC,  "Lightning" },
  { TR2_IM_FIRE,  "Fire" },
  { TR2_IM_COLD,  "Cold" },
};

/*
 * Sustain stats -  these are given their "own" line in the
 * spoiler file, mainly for simplicity
 */
static const flag_desc sustain_flags_desc[] =
{
  { TR2_SUST_STR,  "STR" },
  { TR2_SUST_INT,  "INT" },
  { TR2_SUST_WIS,  "WIS" },
  { TR2_SUST_DEX,  "DEX" },
  { TR2_SUST_CON,  "CON" },
  { TR2_SUST_CHR,  "CHR" },
};

/*
 * Miscellaneous magic given by an object's "flags3" field
 *
 * Note that cursed artifacts and objects with permanent light
 * are handled "directly" -- see analyze_misc_magic()
 */

static const flag_desc misc_flags3_desc[] =
{
  { TR3_SLOW_DIGEST,	  "Slow Digestion" },
  { TR3_FEATHER,	  "Feather Falling" },
  { TR3_LITE,		  "Permanent Light" },
  { TR3_REGEN,		  "Regeneration" },
  { TR3_TELEPATHY,	  "ESP" },
  { TR3_SEE_INVIS,        "See Invisible" },
  { TR3_FREE_ACT,         "Free Action" },
  { TR3_HOLD_LIFE,        "Hold Life" },
  { TR3_BLESSED,          "Blessed Blade" },
  { TR3_IMPACT,           "Earthquake impact on hit" },
  { TR3_TELEPORT,         "Induces random teleportation" },
  { TR3_AGGRAVATE,	  "Aggravates" },
  { TR3_DRAIN_EXP,	  "Drains Experience" },
};

/*
 * A special type used just for deailing with pvals
 */

typedef struct
{
  /*
   * This will contain a string such as "+2", "-10", etc.
   */
  char pval_desc[12];
  
  /*
   * A list of various player traits affected by an object's pval such
   * as stats, speed, stealth, etc.
   *
   * Note that room need only be reserved for the number of stats - 1
   * since the description "All stats" is used if an object affects all
   * all stats. Also, room must be reserved for a sentinel NULL pointer.
   *
   * This will be a list such as ["STR", "DEX", "Stealth", NULL] etc.
   */
  cptr pval_affects[N_ELEMENTS(stat_flags_desc) - 1 +
		    N_ELEMENTS(pval_flags1_desc) + 1];
  
} pval_info_type;


/*
 * An "object analysis structure"
 *
 * It will be filled with descriptive strings detailing an object's
 * various magical powers. The "ignore X" traits are not noted since
 * all artifacts ignore "normal" destruction.
 */

typedef struct
{
  /* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
  char description[160];
  
  /* Description of what is affected by an object's pval */
  pval_info_type pval_info;
  
  /* A list of an object's slaying preferences */
  cptr slays[N_ELEMENTS(slay_flags_desc) + 1];
  
  /* A list of an object's missile weapon qualities. */
  cptr launcher[N_ELEMENTS(launcher_flags_desc) + 1];
  
  /* A list if an object's elemental brands */
  cptr brands[N_ELEMENTS(brand_flags_desc) + 1];
  
  /* A list of immunities granted by an object */
  cptr immunities[N_ELEMENTS(immune_flags_desc) + 1];
  
  /* A list of resistances granted by an object */
  cptr resistances[N_ELEMENTS(resist_flags_desc) + 1];
  
  /* A list of stats sustained by an object */
  cptr sustains[N_ELEMENTS(sustain_flags_desc)  - 1 + 1];
  
  /* A list of various magical qualities an object may have */
  cptr misc_magic[N_ELEMENTS(misc_flags3_desc)
		  + 1	  /* Permanent Light */
		  + 1	  /* type of curse */
		  + 1];	  /* sentinel NULL */
  
  /* A string describing an artifact's activation */
  cptr activation;
  
  /* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
  char misc_desc[80];
  
} obj_desc_list;



/*
 * Write out `n' of the character `c' to the spoiler file
 */
static void spoiler_out_n_chars(int n, char c)
{
  while (--n >= 0) fputc(c, fff);
}

/*
 * Write out `n' blank lines to the spoiler file
 */
static void spoiler_blanklines(int n)
{
  spoiler_out_n_chars(n, '\n');
}

/*
 * Write a line to the spoiler file and then "underline" it with hypens
 */
static void spoiler_underline(cptr str)
{
  fprintf(fff, "%s\n", str);
  spoiler_out_n_chars(strlen(str), '-');
  fprintf(fff, "\n");
}



/*
 * This function does most of the actual "analysis". Given a set of bit flags
 * (which will be from one of the flags fields from the object in question),
 * a "flag description structure", a "description list", and the number of
 * elements in the "flag description structure", this function sets the
 * "description list" members to the appropriate descriptions contained in
 * the "flag description structure".
 *
 * The possibly updated description pointer is returned.
 */

static cptr *spoiler_flag_aux(const u32b art_flags, const flag_desc *flag_x_ptr,
                              cptr *desc_x_ptr, const int n_elmnts)
{
        int i;
  
  for (i = 0; i < n_elmnts; ++i)
    {
      if (art_flags & flag_x_ptr[i].flag)
	{
	  *desc_x_ptr++ = flag_x_ptr[i].desc;
	}
    }
  
  return desc_x_ptr;
}


/*
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 */
static void analyze_general (object_type *o_ptr, char *desc_x_ptr)
{
  /* Get a "useful" description of the object */
  object_desc_store(desc_x_ptr, o_ptr, TRUE, 1);
}

/*
 * List "player traits" altered by an artifact's pval. These include stats,
 * speed, infravision, tunneling, stealth, searching, and extra attacks.
 */
static void analyze_pval (object_type *o_ptr, pval_info_type *pval_x_ptr)
{
  const u32b all_stats = (TR1_STR | TR1_INT | TR1_WIS | 
			  TR1_DEX | TR1_CON | TR1_CHR);
  
  u32b f1, f2, f3;
  
  cptr *affects_list;
  
  /* If pval == 0, there is nothing to do. */
  if (!o_ptr->pval)
    {
      /* An "empty" pval description indicates that pval == 0 */
      pval_x_ptr->pval_desc[0] = '\0';
      return;
    }
  
  /* Extract the flags */
  object_flags(o_ptr, &f1, &f2, &f3);
  
  affects_list = pval_x_ptr->pval_affects;

  /* Create the "+N" string */
  sprintf(pval_x_ptr->pval_desc, "%s%d", POSITIZE(o_ptr->pval), o_ptr->pval);
  
  /* First, check to see if the pval affects all stats */
  if ((f1 & all_stats) == all_stats)
    {
      *affects_list++ = "All stats";
    }
  
  /* Are any stats affected? */
  else if (f1 & all_stats)
    {
      affects_list = spoiler_flag_aux(f1, stat_flags_desc,
				      affects_list,
				      N_ELEMENTS(stat_flags_desc));
    }
  
  /* And now the "rest" */
  affects_list = spoiler_flag_aux(f1, pval_flags1_desc,
				  affects_list,
				  N_ELEMENTS(pval_flags1_desc));
  
  /* Terminate the description list */
  *affects_list = NULL;
}

/* Note the slaying specialties of a weapon */
static void analyze_slay (object_type *o_ptr, cptr *slay_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  slay_list = spoiler_flag_aux(f1, slay_flags_desc, slay_list,
			       N_ELEMENTS(slay_flags_desc));
  
  /* Terminate the description list */
  *slay_list = NULL;
}

/* Note the launcher attributes of a weapon */
static void analyze_launcher (object_type *o_ptr, cptr *launcher_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  launcher_list = spoiler_flag_aux(f1, launcher_flags_desc, launcher_list,
				   N_ELEMENTS(launcher_flags_desc));
  
  /* Terminate the description list */
  *launcher_list = NULL;
}

/* Note an object's elemental brands */
static void analyze_brand (object_type *o_ptr, cptr *brand_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  brand_list = spoiler_flag_aux(f1, brand_flags_desc, brand_list,
				N_ELEMENTS(brand_flags_desc));
  
  /* Terminate the description list */
  *brand_list = NULL;
}


/* Note the resistances granted by an object */

static void analyze_resist (object_type *o_ptr, cptr *resist_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  resist_list = spoiler_flag_aux(f2, resist_flags_desc,
				 resist_list, N_ELEMENTS(resist_flags_desc));
  
  /* Terminate the description list */
  *resist_list = NULL;
}

/* Note the immunities granted by an object */

static void analyze_immune (object_type *o_ptr, cptr *immune_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  immune_list = spoiler_flag_aux(f2, immune_flags_desc,
				 immune_list, N_ELEMENTS(immune_flags_desc));
  
  /* Terminate the description list */
  *immune_list = NULL;

}

/* Note which stats an object sustains */

static void analyze_sustains (object_type *o_ptr, cptr *sustain_list)
{
  const u32b all_sustains = (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS |
			     TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);
  
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  /* Simplify things if an item sustains all stats */
  if ((f2 & all_sustains) == all_sustains)
    {
      *sustain_list++ = "All stats";
    }
  
        /* Should we bother? */
        else if ((f2 & all_sustains))
        {
                sustain_list = spoiler_flag_aux(f2, sustain_flags_desc,
                                                sustain_list,
                                                N_ELEMENTS(sustain_flags_desc));
        }

  /* Terminate the description list */
  *sustain_list = NULL;
}


/*
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 */

static void analyze_misc_magic (object_type *o_ptr, cptr *misc_list)
{
  u32b f1, f2, f3;
  
  object_flags(o_ptr, &f1, &f2, &f3);
  
  /*
   * Special flags
   */
  misc_list = spoiler_flag_aux(f3, misc_flags3_desc, misc_list,
			       N_ELEMENTS(misc_flags3_desc));
  
  /*
   * Artifact lights -- large radius light.
   */
  if ((o_ptr->tval == TV_LITE) && artifact_p(o_ptr))
    {
      *misc_list++ = "Permanent Light(3)";
    }
  
  /*
   * Handle cursed objects here to avoid redundancies such as noting
   * that a permanently cursed object is heavily cursed as well as
   * being "lightly cursed".
   */
  
  if (cursed_p(o_ptr))
    {
      if (f3 & (TR3_PERMA_CURSE))
	{
	  *misc_list++ = "Permanently Cursed";
	}
      else if (f3 & (TR3_HEAVY_CURSE))
	{
	  *misc_list++ = "Heavily Cursed";
	}
      else
	{
	  *misc_list++ = "Cursed";
	}
    }
  
  /* Terminate the description list */
  *misc_list = NULL;
}




/*
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 */

static void analyze_misc (object_type *o_ptr, char *misc_desc)
{
  artifact_type *a_ptr = &a_info[o_ptr->name1];

        if (use_metric) sprintf(misc_desc, "Level %u, Rarity %u, %d.%d kgs, "
                "%ld Gold", a_ptr->level, a_ptr->rarity,
                make_metric(a_ptr->weight) / 10, make_metric(a_ptr->weight) % 10,
                a_ptr->cost);

        else sprintf(misc_desc, "Level %u, Rarity %u, %d.%d lbs, "
                "%ld Gold", a_ptr->level, a_ptr->rarity,
	       a_ptr->weight / 10, a_ptr->weight % 10, a_ptr->cost);
}

/*
 * Fill in an object description structure for a given object
 */

static void object_analyze(object_type *o_ptr, obj_desc_list *desc_x_ptr)
{
  artifact_type *a_ptr = &a_info[o_ptr->name1];
  
  /* Oangband requires that activations be transferred to the object. */
  if (a_ptr->activation)
    {
      o_ptr->xtra1 = OBJECT_XTRA_TYPE_ACTIVATION;
      o_ptr->xtra2 = a_ptr->activation;
    }
  
  analyze_general(o_ptr, desc_x_ptr->description);
  
  analyze_pval(o_ptr, &desc_x_ptr->pval_info);
  
  analyze_brand(o_ptr, desc_x_ptr->brands);
  
  analyze_slay(o_ptr, desc_x_ptr->slays);
  
  analyze_launcher(o_ptr, desc_x_ptr->launcher);
  
  analyze_immune(o_ptr, desc_x_ptr->immunities);
  
  analyze_resist(o_ptr, desc_x_ptr->resistances);
  
  analyze_sustains(o_ptr, desc_x_ptr->sustains);
  
  analyze_misc_magic(o_ptr, desc_x_ptr->misc_magic);
  
  analyze_misc(o_ptr, desc_x_ptr->misc_desc);
  
  desc_x_ptr->activation = item_activation(o_ptr);
}


static void print_header(void)
{
        char buf[80];

        sprintf(buf, "Artifact Spoilers for Oangband Version %d.%d.%d",
                O_VERSION_MAJOR, O_VERSION_MINOR, O_VERSION_PATCH);
        spoiler_underline(buf);
}

/*
 * This is somewhat ugly.
 *
 * Given a header ("Resist", e.g.), a list ("Fire", "Cold", Acid", e.g.),
 * and a separator character (',', e.g.), write the list to the spoiler file
 * in a "nice" format, such as:
 *
 *	Resist Fire, Cold, Acid
 *
 * That was a simple example, but when the list is long, a line wrap
 * should occur, and this should induce a new level of indention if
 * a list is being spread across lines. So for example, Bladeturner's
 * list of resistances should look something like this
 *
 *     Resist Acid, Lightning, Fire, Cold, Poison, Light, Dark, Blindness,
 *	 Confusion, Sound, Shards, Nether, Nexus, Chaos, Disenchantment
 *
 * However, the code distinguishes between a single list of many items vs.
 * many lists. (The separator is used to make this determination.) A single
 * list of many items will not cause line wrapping (since there is no
 * apparent reason to do so). So the lists of Ulmo's miscellaneous traits
 * might look like this:
 *
 *     Free Action; Hold Life; See Invisible; Slow Digestion; Regeneration
 *     Blessed Blade
 *
 * So comparing the two, "Regeneration" has no trailing separator and
 * "Blessed Blade" was not indented. (Also, Ulmo's lists have no headers,
 * but that's not relevant to line wrapping and indention.)
 */

/* ITEM_SEP separates items within a list */
#define ITEM_SEP ','


/* LIST_SEP separates lists */
#define LIST_SEP ';'


static void spoiler_outlist(cptr header, cptr *list, char separator)
{
  int line_len, buf_len;
  char line[MAX_LINE_LEN+1], buf[80];
  
  /* Ignore an empty list */
  if (*list == NULL) return;
  
  /* This function always indents */
  strcpy(line, INDENT1);
  
  /* Create header (if one was given) */
  if (header && (header[0]))
    {
      strcat(line, header);
      strcat(line, " ");
    }
  
  line_len = strlen(line);
  
  /* Now begin the tedious task */
  while (1)
    {
      /* Copy the current item to a buffer */
      strcpy(buf, *list);
      
      /* Note the buffer's length */
      buf_len = strlen(buf);
      
      /*
       * If there is an item following this one, pad with separator and
       * a space and adjust the buffer length
       */
      
      if (list[1])
	{
	  sprintf(buf + buf_len, "%c ", separator);
	  buf_len += 2;
	}
      
      /*
       * If the buffer will fit on the current line, just append the
       * buffer to the line and adjust the line length accordingly.
       */
      
      if (line_len + buf_len <= MAX_LINE_LEN)
	{
	  strcat(line, buf);
	  line_len += buf_len;
	}
      
      /* Apply line wrapping and indention semantics described above */
      else
	{
	  /*
	   * Don't print a trailing list separator but do print a trailing
	   * item separator.
	   */
	  if (line_len > 1 && line[line_len - 1] == ' '
	      && line[line_len - 2] == LIST_SEP)
	    {
	      /* Ignore space and separator */
	      line[line_len - 2] = '\0';
	      
	      /* Write to spoiler file */
	      fprintf(fff, "%s\n", line);
	      
	      /* Begin new line at primary indention level */
	      sprintf(line, "%s%s", INDENT1, buf);
	    }
	  
	  else
	    {
	      /* Write to spoiler file */
	      fprintf(fff, "%s\n", line);
	      
	      /* Begin new line at secondary indention level */
	      sprintf(line, "%s%s", INDENT2, buf);
	    }
	  
	  line_len = strlen(line);
	}
      
      /* Advance, with break */
      if (!*++list) break;
    }

  /* Write what's left to the spoiler file */
  fprintf(fff, "%s\n", line);
}


/* Create a spoiler file entry for an artifact */

static void spoiler_print_art(obj_desc_list *art_ptr)
{
  pval_info_type *pval_ptr = &art_ptr->pval_info;
  
  char buf[80];
  
  /* Don't indent the first line */
  fprintf(fff, "%s\n", art_ptr->description);
  
  /* An "empty" pval description indicates that the pval affects nothing */
  if (pval_ptr->pval_desc[0])
    {
      /* Mention the effects of pval */
      sprintf(buf, "%s to", pval_ptr->pval_desc);
      spoiler_outlist(buf, pval_ptr->pval_affects, ITEM_SEP);
    }
  
  /* Now deal with the description lists */
  
  spoiler_outlist("Slay", art_ptr->slays, ITEM_SEP);
  
  spoiler_outlist("", art_ptr->launcher, LIST_SEP);
  
  spoiler_outlist("", art_ptr->brands, LIST_SEP);
  
  spoiler_outlist("Immunity to", art_ptr->immunities, ITEM_SEP);
  
  spoiler_outlist("Resist", art_ptr->resistances, ITEM_SEP);
  
  spoiler_outlist("Sustain", art_ptr->sustains, ITEM_SEP);
  
  spoiler_outlist("", art_ptr->misc_magic, LIST_SEP);
  
  
  /* Write out the possible activation at the primary indention level */
  if (art_ptr->activation)
    {
      fprintf(fff, "%sActivates for %s\n", INDENT1, art_ptr->activation);
    }
  
  /* End with the miscellaneous facts */
  fprintf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}


/*
 * Hack -- Create a "forged" artifact
 */
bool make_fake_artifact(object_type *o_ptr, int name1)
{
        int i;

  artifact_type *a_ptr = &a_info[name1];
  
  
  /* Ignore "empty" artifacts */
  if (!a_ptr->name) return FALSE;
  
  /* Acquire the "kind" index */
  i = lookup_kind(a_ptr->tval, a_ptr->sval);
  
  /* Oops */
  if (!i) return (FALSE);
  
  /* Create the artifact */
  object_prep(o_ptr, i);
  
  /* Save the name */
  o_ptr->name1 = name1;
  
  /* Extract the fields */
  o_ptr->pval = a_ptr->pval;
  o_ptr->ac = a_ptr->ac;
  o_ptr->dd = a_ptr->dd;
  o_ptr->ds = a_ptr->ds;
  o_ptr->to_a = a_ptr->to_a;
  o_ptr->to_h = a_ptr->to_h;
  o_ptr->to_d = a_ptr->to_d;
  o_ptr->weight = a_ptr->weight;
   
  /* Success */
  return (TRUE);
}





/*
 * Show what object kinds appear on the current level
 */
static void spoil_obj_gen(cptr fname)
{
  int i;
  
        /* Storage */
        u32b artifacts = 0L;
        u32b egoitems = 0L;
#ifdef _WIN32_WCE
  u32b *object = malloc(z_info->k_max * sizeof(*object));
#else
  u32b object[z_info->k_max];
#endif
        u32b depth[MAX_DEPTH];

        object_type *i_ptr;
  object_type object_type_body;
  char o_name[120];
  
  char buf[1024];
  
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);
  
  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
  if (!fff)
    {
      msg_print("Cannot create spoiler file.");
#ifdef _WIN32_WCE
      free(object);
#endif
      return;
    }
  
  
  
  msg_print("This may take a while...");
  if (!fresh_after) Term_fresh();



        fprintf(fff, "Object Generation Spoiler for Oangband Version %d.%d.%d\n",
                O_VERSION_MAJOR, O_VERSION_MINOR, O_VERSION_PATCH);

        /* Clear storage. */
  for (i = 0; i < z_info->k_max; i++)
        {
                object[i] = 0L;
        }
  
  /* Clear storage. */
  for (i = 0; i < MAX_DEPTH; i++)
    {
      depth[i] = 0L;
    }
  
  /* Make a lot of objects, and print their names out. */
  for (i = 0L; i < 1000000L; i++)
    {
      if (i % 10000 == 0) 
	{
	  prt(format("%ld objects created", (long)i), 0, 0);
	  if (!fresh_after) Term_fresh();
	}
      
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Create an object - no special conditions */
      make_object(i_ptr, FALSE, FALSE, FALSE);
      
      /* Count artifacts. */
      if (i_ptr->name1) artifacts += 1L;
      
      /* Count ego-items. */
      if (i_ptr->name2) egoitems += 1L;
      
      /* Count object kinds. */
      object[i_ptr->k_idx] += 1L;
      
      /* Count objects of that level. */
                depth[k_info[i_ptr->k_idx].level] += 1L;

                /* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
      if (artifact_p(i_ptr)) a_info[i_ptr->name1].creat_turn = 0;
        }

        /* Print to file. */
  fprintf(fff, "\n\n\n");
  fprintf(fff, "artifacts:  %ld\n", (long)artifacts);
  fprintf(fff, "ego-items:  %ld\n", (long)egoitems);
  fprintf(fff, "\n\n");
        fprintf(fff, "Number of objects created (1,000,000 total)\n");
        fprintf(fff, "         Generation Level:  %d\n\n", p_ptr->depth);

  for (i = 1; i < z_info->k_max; i++)
        {
                if (object[i])
                {
	  object_kind *k_ptr = &k_info[i];
                        char *t;
                        cptr str = (k_name + k_ptr->name);

          if (strlen(str) == 0) continue;

                        /* Skip past leading characters */
                        while ((*str == ' ') || (*str == '&')) str++;
	  
	  /* Copy useful chars */
	  for (t = o_name; *str; str++)
	    {
	      if (*str != '~') *t++ = *str;
	    }
	  
	  /* Terminate the new name */
	  *t = '\0';
	  
	  fprintf(fff, "%-40s:%6ld\n", o_name, (long)object[i]);
		}
    }
  
  fprintf(fff, "\n\n\n");
  fprintf(fff, "Object distribution by depth\n\n");
  
  for (i = 0; i < MAX_DEPTH; i++)
    {
      if (depth[i]) fprintf(fff, "Level %3d:%6ld\n", i, (long)depth[i]);
    }
  
#ifdef _WIN32_WCE
  free(object);
#endif
  
  
  /* Check for errors */
  if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
      return;
    }
  
  /* Message */
  msg_print("Successfully created a spoiler file.");
}





/*
 * Show what monster races appear on the current level
 */
static void spoil_mon_gen(cptr fname)
{
        int i, num;

        /* Storage */
#ifdef _WIN32_WCE
  u32b *monster = malloc(z_info->r_max * sizeof(*monster));
#else
  u32b monster[z_info->r_max];
#endif
        u32b depth[MAX_DEPTH];

        bool quick;

  
  char buf[1024];
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);
  
  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
        if (!fff)
        {
                msg_print("Cannot create spoiler file.");
#ifdef _WIN32_WCE
      free(monster);
#endif
                return;
        }

        fprintf(fff, "Monster Generation Spoiler for Oangband Version %d.%d.%d\n",
                O_VERSION_MAJOR, O_VERSION_MINOR, O_VERSION_PATCH);

        /* Clear storage. */
  for (i = 0; i < z_info->r_max; i++)
        {
                monster[i] = 0L;
        }
  
  /* Clear storage. */
  for (i = 0; i < MAX_DEPTH; i++)
    {
      depth[i] = 0L;
    }
  
  /* Clear screen */
  Term_clear();
  
  /* Info */
        prt("Use the quick monster generator?.", 2, 5);

        /* Prompt for a monster selection method */
        prt("Monster generation uses a three-level table:  The base probability, ", 5, 5);
        prt("gotten from r_info.txt, the modified probability as adjusted by , ", 6, 5);
        prt("special monster restrictions (like only dragons), and the final , ", 7, 5);
        prt("probability, which depends on values in the first two levels plus , ", 8, 5);
        prt("the current generation depth.  , ", 9, 5);

        prt("The quick monster generator builds the third probability level only ,", 11, 5);
        prt("once, and then draws monsters from it.  The standard method is to , ", 12, 5);
        prt("rebuild the third probability level for each monster", 13, 5);


  /* Prompt */
  prt("Command: ", 15, 0);
  
  /* Get a choice */
  i = inkey();
  
  if (i == 'y') quick = TRUE;
  else quick = FALSE;
  
  
  msg_print("This may take a while...");
  if (!fresh_after) Term_fresh();
  
  /* Initialize monster generation. */
  if (quick) (void)get_mon_num(p_ptr->depth);
  
  /* Make a lot of monsters, and print their names out. */
  for (i = 0L; i < 1000000L; i++)
    {
      if (i % 10000 == 0) 
	{
	  prt(format("%ld monsters created", (long)i), 0, 0);
	  if (!fresh_after) Term_fresh();
	}
      
      
      /* Get a monster index */
      if (quick) num = get_mon_num_quick(p_ptr->depth);
      else num = get_mon_num(p_ptr->depth);
      
      /* Count monster races. */
      monster[num] += 1L;
      
      /* Count monsters of that level. */
      depth[r_info[num].level] += 1L;
    }
  
  /* Print to file. */
  fprintf(fff, "\n\n\n");
        fprintf(fff, "Number of monsters of various kinds (1,000,000 total)\n");
        fprintf(fff, "         Generation Level:  %d\n\n", p_ptr->depth);

  for (i = 1; i < z_info->r_max; i++)
        {
                monster_race *r_ptr = &r_info[i];

      cptr name = (r_name + r_ptr->name);
      
      if (monster[i])
	{
	  fprintf(fff, "%-45s:%6ld\n", name, (long)monster[i]);
	}
    }

  fprintf(fff, "\n\n\n");
  fprintf(fff, "Monster distribution by depth\n\n");
  
  for (i = 0; i < MAX_DEPTH; i++)
    {
      if (depth[i]) fprintf(fff, "Level %3d:%6ld\n", i, (long)depth[i]);
    }
  
#ifdef _WIN32_WCE
  free(monster);
#endif
  
  /* Check for errors */
  if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
      return;
    }

  /* Message */
  msg_print("Successfully created a spoiler file.");
}






/*
 * Create a spoiler file for artifacts
 */
static void spoil_artifact(cptr fname)
{
  int i, j;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  obj_desc_list artifact;
  
  char buf[1024];
  
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);
  
  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
  if (!fff)
    {
      msg_print("Cannot create spoiler file.");
      return;
    }
  
  /* Dump the header */
  print_header();
  
  /* List the artifacts by tval */
  for (i = 0; group_artifact[i].tval; i++)
    {
      /* Write out the group title */
      if (group_artifact[i].name)
	{
	  spoiler_blanklines(2);
	  spoiler_underline(group_artifact[i].name);
	  spoiler_blanklines(1);
                }

                /* Now search through all of the artifacts */
      for (j = 1; j < z_info->a_max; ++j)
                {
                        artifact_type *a_ptr = &a_info[j];

	  /* We only want objects in the current group */
	  if (a_ptr->tval != group_artifact[i].tval) continue;
	  
	  /* Get local object */
	  i_ptr = &object_type_body;
	  
	  /* Wipe the object */
	  object_wipe(i_ptr);
	  
	  /* Attempt to "forge" the artifact */
	  if (!make_fake_artifact(i_ptr, j)) continue;
	  
	  /* Analyze the artifact */
	  object_analyze(i_ptr, &artifact);
	  
	  /* Write out the artifact description to the spoiler file */
	  spoiler_print_art(&artifact);
	}
    }
  
  /* Check for errors */
  if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
      return;
	}
  
  /* Message */
  msg_print("Successfully created a spoiler file.");
}





/*
 * Create a spoiler file for monsters
 */
static void spoil_mon_desc(cptr fname)
{
  int i, n = 0;
  
  char buf[1024];
  
  char nam[80];
  char lev[80];
  char rar[80];
  char spd[80];
  char ac[80];
  char hp[80];
  char exp[80];
  
  u16b *who;
  u16b why = 2;
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);

  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
  if (!fff)
    {
      msg_print("Cannot create spoiler file.");
      return;
        }

        /* Dump the header */
        fprintf(fff, "Monster Spoilers for Oangband Version %d.%d.%d\n",
                O_VERSION_MAJOR, O_VERSION_MINOR, O_VERSION_PATCH);
        fprintf(fff, "------------------------------------------\n\n");

        /* Dump the header */
  fprintf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
	  "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
  fprintf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
	  "----", "---", "---", "---", "--", "--", "-----------");


        /* Allocate the "who" array */
  C_MAKE(who, z_info->r_max, u16b);

        /* Scan the monsters */
  for (i = 1; i < z_info->r_max; i++)
        {
                monster_race *r_ptr = &r_info[i];

      /* Use that monster */
      if (r_ptr->name) who[n++] = i;
    }
  
  /* Select the sort method */
  ang_sort_comp = ang_sort_comp_hook;
  ang_sort_swap = ang_sort_swap_hook;
  
  /* Sort the array by dungeon depth of monsters */
  ang_sort(who, &why, n);
  
  /* Scan again */
  for (i = 0; i < n; i++)
    {
      monster_race *r_ptr = &r_info[who[i]];
      
      cptr name = (r_name + r_ptr->name);
      
      /* Get the "name" */
      if (r_ptr->flags1 & (RF1_QUESTOR))
	{
	  sprintf(nam, "[Q] %s", name);
	}
      else if (r_ptr->flags1 & (RF1_UNIQUE))
	{
	  sprintf(nam, "[U] %s", name);
	}
      else
	{
	  sprintf(nam, "The %s", name);
	}
      
      
      /* Level */
      sprintf(lev, "%d", r_ptr->level);
      
      /* Rarity */
      sprintf(rar, "%d", r_ptr->rarity);
      
      /* Speed */
      if (r_ptr->speed >= 110)
	{
	  sprintf(spd, "+%d", (r_ptr->speed - 110));
	}
      else
	{
	  sprintf(spd, "-%d", (110 - r_ptr->speed));
	}
      
      /* Armor Class */
      sprintf(ac, "%d", r_ptr->ac);
      
      /* Hitpoints */
      if ((r_ptr->flags1 & (RF1_FORCE_MAXHP)) || (r_ptr->hside == 1))
	{
	  sprintf(hp, "%d", r_ptr->hdice * r_ptr->hside);
	}
      else
	{
	  sprintf(hp, "%dd%d", r_ptr->hdice, r_ptr->hside);
	}
      
      
      /* Experience */
      sprintf(exp, "%ld", (long)(r_ptr->mexp));

      /* Hack -- use visual instead */
      sprintf(exp, "%s '%c'", attr_to_text(r_ptr->d_attr), r_ptr->d_char);
      
      /* Dump the info */
      fprintf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
	      nam, lev, rar, spd, hp, ac, exp);
    }
  
  /* End it */
        fprintf(fff, "\n");

        /* Free the "who" array */
  FREE(who);

        /* Check for errors */
        if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
		return;
    }
  
  /* Worked */
  msg_print("Successfully created a spoiler file.");
}




/*
 * Monster spoilers by: smchorse@ringer.cs.utsa.edu (Shawn McHorse)
 *
 * Adapted from the "monster_desc()" code in "monster1.c"
 */

/*
 * Pronoun arrays
 */
static cptr wd_che[3] =
  { "It", "He", "She" };
static cptr wd_lhe[3] =
  { "it", "he", "she" };

/*
 * Buffer text to the given file. (-SHAWN-)
 * This is basically c_roff() from util.c with a few changes.
 */
static void spoil_out(cptr str)
{
  cptr r;

  /* Line buffer */
  static char roff_buf[256];
  
  /* Current pointer into line roff_buf */
  static char *roff_p = roff_buf;
  
  /* Last space saved into roff_buf */
  static char *roff_s = NULL;
  
  /* Special handling for "new sequence" */
  if (!str)
    {
      if (roff_p != roff_buf) roff_p--;
      while (*roff_p == ' ' && roff_p != roff_buf) roff_p--;
      if (roff_p == roff_buf) fprintf(fff, "\n");
      else
	{
	  *(roff_p + 1) = '\0';
	  fprintf(fff, "%s\n\n", roff_buf);
	}
      roff_p = roff_buf;
      roff_s = NULL;
      roff_buf[0] = '\0';
      return;
    }
  
  /* Scan the given string, character at a time */
  for (; *str; str++)
	{
	  char ch = *str;
	  int wrap = (ch == '\n');
	  
	  if (!isprint(ch)) ch = ' ';
	  if (roff_p >= roff_buf + 75) wrap = 1;
	  if ((ch == ' ') && (roff_p + 2 >= roff_buf + 75)) wrap = 1;
	  
	  /* Handle line-wrap */
	  if (wrap)
	    {
	      *roff_p = '\0';
	      r = roff_p;
	      if (roff_s && (ch != ' '))
		{
		  *roff_s = '\0';
		  r = roff_s + 1;
		}
	      fprintf(fff, "%s\n", roff_buf);
	      roff_s = NULL;
	      roff_p = roff_buf;
	      while (*r) *roff_p++ = *r++;
	    }
	  
	  /* Save the char */
	  if ((roff_p > roff_buf) || (ch != ' '))
	    {
	      if (ch == ' ') roff_s = roff_p;
			*roff_p++ = ch;
	    }
	}
}


/*
 * Create a spoiler file for monsters (-SHAWN-)
 */
static void spoil_mon_info(cptr fname)
{
  char buf[1024];
  int msex, vn, i, j, k, n;
  bool breath, magic, sin;
  cptr p, q;
  cptr vp[64];
  u32b flags1, flags2, flags3, flags4, flags5, flags6, flags7;

  int spower;
  
  cptr name;
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, fname);
  
  /* File type is "TEXT" */
  FILE_TYPE(FILE_TYPE_TEXT);
  
  /* Open the file */
  fff = my_fopen(buf, "w");
  
  /* Oops */
  if (!fff)
    {
      msg_print("Cannot create spoiler file.");
      return;
    }


        /* Dump the header */
        sprintf(buf, "Monster Spoilers for Oangband Version %d.%d.%d\n",
                O_VERSION_MAJOR, O_VERSION_MINOR, O_VERSION_PATCH);
        spoil_out(buf);
        spoil_out("------------------------------------------\n\n");

        /*
         * List all monsters in order.
         */
  for (n = 1; n < z_info->r_max; n++)
        {
                monster_race *r_ptr = &r_info[n];

      /* Extract the flags */
      flags1 = r_ptr->flags1;
      flags2 = r_ptr->flags2;
      flags3 = r_ptr->flags3;
      flags4 = r_ptr->flags4;
      flags5 = r_ptr->flags5;
      flags6 = r_ptr->flags6;
      flags7 = r_ptr->flags7;
      breath = FALSE;
      magic = FALSE;
      spower = r_ptr->spell_power;
      
      /* Get the monster name */
      name = (r_name + r_ptr->name);
      
      /* Extract a gender (if applicable) */
      if (flags1 & (RF1_FEMALE)) msex = 2;
      else if (flags1 & (RF1_MALE)) msex = 1;
      else msex = 0;
      

      /* Prefix */
      if (flags1 & (RF1_QUESTOR))
	{
	  spoil_out("[Q] ");
	}
      else if (flags1 & (RF1_UNIQUE))
	{
	  spoil_out("[U] ");
	}
      else
	{
	  spoil_out("The ");
	}
      
      /* Name */
      sprintf(buf, "%s  (", (r_name + r_ptr->name));	/* ---)--- */
      spoil_out(buf);
      
      /* Color */
      spoil_out(attr_to_text(r_ptr->d_attr));
      
      /* Symbol --(-- */
      sprintf(buf, " '%c')\n", r_ptr->d_char);
      spoil_out(buf);
      
      
      /* Indent */
      sprintf(buf, "=== ");
      spoil_out(buf);
      
      /* Number */
      sprintf(buf, "Num:%d  ", n);
      spoil_out(buf);
      
      /* Level */
      sprintf(buf, "Lev:%d  ", r_ptr->level);
      spoil_out(buf);
      
      /* Rarity */
      sprintf(buf, "Rar:%d  ", r_ptr->rarity);
      spoil_out(buf);
      
      /* Speed */
      if (r_ptr->speed >= 110)
	{
	  sprintf(buf, "Spd:+%d  ", (r_ptr->speed - 110));
	}
      else
	{
	  sprintf(buf, "Spd:-%d  ", (110 - r_ptr->speed));
	}
      spoil_out(buf);

      /* Hitpoints */
      if ((flags1 & (RF1_FORCE_MAXHP)) || (r_ptr->hside == 1))
	{
	  sprintf(buf, "Hp:%d  ", r_ptr->hdice * r_ptr->hside);
	}
      else
	{
	  sprintf(buf, "Hp:%dd%d  ", r_ptr->hdice, r_ptr->hside);
	}
      spoil_out(buf);
      
      /* Armor Class */
      sprintf(buf, "Ac:%d  ", r_ptr->ac);
      spoil_out(buf);
      
      /* Experience */
      sprintf(buf, "Exp:%ld\n", (long)(r_ptr->mexp));
      spoil_out(buf);
      
      
      /* Describe */
      spoil_out(r_text + r_ptr->text);
      spoil_out("  ");
      
      
      spoil_out("This");
      
      if (flags3 & (RF3_ANIMAL)) spoil_out(" natural");
      if (flags3 & (RF3_EVIL)) spoil_out(" evil");
      if (flags3 & (RF3_UNDEAD)) spoil_out(" undead");
      
      if (flags3 & (RF3_DRAGON)) spoil_out(" dragon");
      else if (flags3 & (RF3_DEMON)) spoil_out(" demon");
      else if (flags3 & (RF3_GIANT)) spoil_out(" giant");
      else if (flags3 & (RF3_TROLL)) spoil_out(" troll");
      else if (flags3 & (RF3_ORC)) spoil_out(" orc");
      else spoil_out(" creature");
      
      spoil_out(" moves");
      
      if ((flags1 & (RF1_RAND_50)) && (flags1 & (RF1_RAND_25)))
	{
	  spoil_out(" extremely erratically");
	}
      else if (flags1 & (RF1_RAND_50))
	{
	  spoil_out(" somewhat erratically");
	}
      else if (flags1 & (RF1_RAND_25))
	{
	  spoil_out(" a bit erratically");
	}
      else
	{
	  spoil_out(" normally");
	}
      
      if (flags1 & (RF1_NEVER_MOVE))
	{
	  spoil_out(", but does not deign to chase intruders");
	}
      
                spoil_out(".  ");


#if 0

                if (!r_ptr->level || (flags1 & (RF1_FORCE_DEPTH)))
                {
                        sprintf(buf, "%s is never found out of depth.  ", wd_che[msex]);
                }

                if (flags1 & (RF1_FORCE_SLEEP))
                {
                        sprintf(buf, "%s is always created asleep.  ", wd_che[msex]);
                }

#endif

                if (flags1 & (RF1_ESCORT))
                {
                        sprintf(buf, "%s usually appears with ", wd_che[msex]);
	  spoil_out(buf);
	  if (flags1 & (RF1_ESCORTS)) spoil_out("escorts.  ");
	  else spoil_out("an escort.  ");
	}
      
      if ((flags1 & (RF1_FRIEND)) || (flags1 & (RF1_FRIENDS)))
	{
	  sprintf(buf, "%s usually appears in groups.  ", wd_che[msex]);
	  spoil_out(buf);
	}
      
      
      /* Collect inate attacks */
      vn = 0;
      if (flags4 & (RF4_SHRIEK)) vp[vn++] = "shriek for help";
      if (flags4 & (RF4_LASH)) vp[vn++] = "lash you if nearby";
      if (flags4 & (RF4_BOULDER))
	{
	  if (spower < 10) vp[vn++] = "throw rocks";
	  else vp[vn++] = "throw boulders";
	}
      if (flags4 & (RF4_SHOT))
	{
	  if (spower < 5) vp[vn++] = "sling pebbles";
	  else if (spower < 15) vp[vn++] = "sling leaden pellets";
	  else vp[vn++] = "sling seeker shot";
	}
      if (flags4 & (RF4_ARROW))
	{
	  if (spower < 8) vp[vn++] = "shoot little arrows";
	  else if (spower < 15) vp[vn++] = "shoot arrows";
	  else vp[vn++] = "shoot seeker arrows";
	}
      if (flags4 & (RF4_BOLT))
	{
	  if (spower < 8) vp[vn++] = "fire bolts";
	  else if (spower < 15) vp[vn++] = "fire crossbow quarrels";
	  else vp[vn++] = "fire seeker bolts";
	}
      if (flags4 & (RF4_MISSL))
	{
	  if (spower < 8) vp[vn++] = "fire little missiles";
	  else if (spower < 15) vp[vn++] = "fire missiles";
	  else vp[vn++] = "fire heavy missiles";
	}
      if (flags4 & (RF4_PMISSL))
	{
	  if (flags2 & (RF2_MORGUL_MAGIC)) vp[vn++] = "hurl black darts";
	  else vp[vn++] = "whip poisoned darts";
	}
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  
	  for (i = 0; i < vn; i++)
                        {
                                if (!i)
                                {
                  spoil_out(" may ");
                  if (flags2 & (RF2_ARCHER)) spoil_out("frequently ");
                                }
                                else if (i < vn-1) spoil_out(", ");
                                else spoil_out(", or ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      /* Collect breaths */
      vn = 0;
      if (flags4 & (RF4_BRTH_ACID))	vp[vn++] = "acid";
      if (flags4 & (RF4_BRTH_ELEC))	vp[vn++] = "lightning";
      if (flags4 & (RF4_BRTH_FIRE))	vp[vn++] = "fire";
      if (flags4 & (RF4_BRTH_COLD))	vp[vn++] = "frost";
      if (flags4 & (RF4_BRTH_POIS))	vp[vn++] = "poison";
      if (flags4 & (RF4_BRTH_PLAS))	vp[vn++] = "plasma";
      if (flags4 & (RF4_BRTH_LITE))	vp[vn++] = "light";
      if (flags4 & (RF4_BRTH_DARK))
	{
	  if (flags2 & (RF2_MORGUL_MAGIC)) vp[vn++] = "Night";
	  else vp[vn++] = "darkness";
	}
      if (flags4 & (RF4_BRTH_CONFU))	vp[vn++] = "confusion";
      if (flags4 & (RF4_BRTH_SOUND))	vp[vn++] = "sound";
      if (flags4 & (RF4_BRTH_SHARD))	vp[vn++] = "shards";
      if (flags4 & (RF4_BRTH_INER))	vp[vn++] = "inertia";
      if (flags4 & (RF4_BRTH_GRAV))	vp[vn++] = "gravity";
      if (flags4 & (RF4_BRTH_FORCE))	vp[vn++] = "force";
      
      if (flags4 & (RF4_BRTH_NEXUS))	vp[vn++] = "nexus";
      if (flags4 & (RF4_BRTH_NETHR))	vp[vn++] = "nether";
      if (flags4 & (RF4_BRTH_CHAOS))	vp[vn++] = "chaos";
                if (flags4 & (RF4_BRTH_DISEN))  vp[vn++] = "disenchantment";
                if (flags4 & (RF4_BRTH_TIME))           vp[vn++] = "time";

                if (flags4 & (RF4_XXX2))                vp[vn++] = "something";
                if (flags4 & (RF4_XXX3))                vp[vn++] = "something";
                if (flags4 & (RF4_XXX4))                vp[vn++] = "something";
                if (flags4 & (RF4_XXX5))                vp[vn++] = "something";
                if (flags4 & (RF4_XXX6))                vp[vn++] = "something";

      if (vn)
	{
	  breath = TRUE;
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" may breathe ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" or ");
	      spoil_out(vp[i]);
	    }
	  if (flags2 & (RF2_POWERFUL)) spoil_out(" powerfully");
	}
      
      /* Collect spells */
      vn = 0;
      
      if (flags5 & (RF5_BALL_ACID))
	{
	  if (spower < 70) vp[vn++] = "produce acid balls";
	  else vp[vn++] = "produce acid storms";
	}
      if (flags5 & (RF5_BALL_ELEC))
	{
	  if (spower < 70) vp[vn++] = "produce lightning balls";
	  else vp[vn++] = "produce lightning storms";
	}
      if (flags5 & (RF5_BALL_FIRE))	
	{
	  if (flags2 & (RF2_UDUN_MAGIC))
	    {
	      if (spower < 70) vp[vn++] = "produce balls of hellfire";
	      else if (spower < 110) vp[vn++] = "invoke storms of Udun-fire";
	      else vp[vn++] = "call upon the fires of Udun";
	    }
	  else
	    {
	      if (spower < 70) vp[vn++] = "produce fire balls";
	      else vp[vn++] = "produce fire storms";
	    }
	}
      if (flags5 & (RF5_BALL_COLD))
	{
	  if (flags2 & (RF2_MORGUL_MAGIC))
	    {
	      if (spower < 70) vp[vn++] = "produce spheres of deadly cold";
	      else vp[vn++] = "invoke storms of deadly cold";
	    }
	  else
	    {
	      if (spower < 70) vp[vn++] = "produce frost balls";
	      else vp[vn++] = "produce frost storms";
	    }
	}
      if (flags5 & (RF5_BALL_POIS))
	{
	  if (flags2 & (RF2_MORGUL_MAGIC))
	    {
	      if (spower < 15) vp[vn++] = "produce clouds of venom";
	      else if (spower < 70) vp[vn++] = "produce venomous balls";
	      else vp[vn++] = "raise storms of venom";
	    }
	  else
	    {
	      if (spower < 15) vp[vn++] = "produce stinking clouds";
	      else if (spower < 70) vp[vn++] = "produce poison balls";
	      else vp[vn++] = "produce storms of poison";
	    }
	}
      if (flags5 & (RF5_BALL_LITE))
	{
	  if (spower < 15) vp[vn++] = "produce spheres of light";
	  else if (spower < 70) vp[vn++] = "produce explosions of light";
	  else vp[vn++] = "invoke starbursts";
	}
      if (flags5 & (RF5_BALL_DARK))
	{
	  if (flags2 & (RF2_MORGUL_MAGIC))
	    {
	      if (spower < 70) vp[vn++] = "produce spheres of Night";
	      else vp[vn++] = "conjure up storms of Night";
	    }
	  else
	    {
	      if (spower < 70) vp[vn++] = "produce balls of darkness";
	      else vp[vn++] = "produce storms of darkness";
	    }
	}
      if (flags5 & (RF5_BALL_CONFU))
	{
	  if (spower < 70) vp[vn++] = "produce confusion balls";
	  else vp[vn++] = "produce storms of confusion";
	}
      if (flags5 & (RF5_BALL_SOUND))
	{
	  if (spower < 15) vp[vn++] = "produce explosions of sound";
	  else if (spower < 70) vp[vn++] = "produce thunderclaps";
	  else vp[vn++] = "unleash storms of sound";
	}
      if (flags5 & (RF5_BALL_SHARD))
	{
	  if (spower < 15) vp[vn++] = "produce blasts of shards";
	  else if (spower < 90) vp[vn++] = "produce whirlwinds of shards";
	  else vp[vn++] = "call up shardstorms";
	}
      if (flags5 & (RF5_BALL_STORM))
	{
	  if (spower < 30) vp[vn++] = "produce little storms";
	  else if (spower < 70) vp[vn++] = "produce whirlpools";
	  else vp[vn++] = "call up raging storms";
	}
      if (flags5 & (RF5_BALL_NETHR))
	{
	  if (spower < 30) vp[vn++] = "produce nether orbs";
	  else if (spower < 70) vp[vn++] = "produce nether balls";
	  else vp[vn++] = "invoke nether storms";
	}
      if (flags5 & (RF5_BALL_CHAOS))
	{
	  if (spower < 20) vp[vn++] = "produce spheres of chaos";
	  else if (spower < 70) vp[vn++] = "produce explosions of chaos";
	  else vp[vn++] = "call up maelstroms of raw chaos";
	}
      if (flags5 & (RF5_BALL_MANA))
	{
	  if (spower < 40) vp[vn++] = "produce manabursts";
	  else if (spower < 90) vp[vn++] = "produce balls of mana";
	  else vp[vn++] = "invoke mana storms";
	}
      if (flags5 & (RF5_BOLT_ACID))	vp[vn++] = "produce acid bolts";
      if (flags5 & (RF5_BOLT_ELEC))	vp[vn++] = "produce lightning bolts";
      if (flags5 & (RF5_BOLT_FIRE))	vp[vn++] = "produce fire bolts";
      if (flags5 & (RF5_BOLT_COLD))	vp[vn++] = "produce frost bolts";
      if (flags5 & (RF5_BOLT_POIS))	vp[vn++] = "produce poison bolts";
      if (flags5 & (RF5_BOLT_PLAS))	vp[vn++] = "produce plasma bolts";
      if (flags5 & (RF5_BOLT_ICE))	vp[vn++] = "produce ice bolts";
      if (flags5 & (RF5_BOLT_WATER))	vp[vn++] = "produce water bolts";
      if (flags5 & (RF5_BOLT_NETHR))	vp[vn++] = "produce nether bolts";
      if (flags5 & (RF5_BOLT_MANA))	vp[vn++] = "produce mana bolts";
      if (flags5 & (RF5_BEAM_ELEC))	vp[vn++] = "shoot sparks of lightning";
      if (flags5 & (RF5_BEAM_ICE))	vp[vn++] = "cast lances of ice";
      if (flags5 & (RF5_BEAM_NETHR))
	{
                        if (spower < 40) vp[vn++] = "cast lances of nether";
                        else if (spower < 90) vp[vn++] = "shoot rays of death";
                }
                if (flags5 & (RF5_ARC__HFIRE))
                {
                        if (flags2 & (RF2_UDUN_MAGIC))
                        {
	      if (spower < 50) vp[vn++] = "produce a column of hellfire";
	      else if (spower < 100) vp[vn++] = "envelop you in hellfire";
	      else vp[vn++] = "breath like the Balrog";
	    }
	  else
	    {
	      if (spower < 50) vp[vn++] = "produce a column of fire";
                                else if (spower < 100) vp[vn++] = "envelop you in fire";
                        }
                }
                if (flags5 & (RF5_ARC__FORCE))
                {
                        if (spower < 50) vp[vn++] = "thrust you away";
                        else if (spower < 100) vp[vn++] = "hurls you away";
	  else vp[vn++] = "snatches you up, and throws you away";
	}
      if (flags6 & (RF6_HASTE))		vp[vn++] = "haste-self";
      if (flags6 & (RF6_ADD_MANA))	vp[vn++] = "restore mana";
      if (flags6 & (RF6_HEAL))		vp[vn++] = "heal-self";
      if (flags6 & (RF6_CURE))		vp[vn++] = "cure what ails it";
      if (flags6 & (RF6_BLINK))		vp[vn++] = "blink-self";
      if (flags6 & (RF6_TPORT))		vp[vn++] = "teleport-self";
      if (flags6 & (RF6_TELE_TO))	vp[vn++] = "teleport to";
      if (flags6 & (RF6_TELE_AWAY))	vp[vn++] = "teleport away";
      if (flags6 & (RF6_TELE_LEVEL))	vp[vn++] = "teleport level";
      if (flags6 & (RF6_DARKNESS))	vp[vn++] = "create darkness";
      if (flags6 & (RF6_TRAPS))		vp[vn++] = "create traps";
      if (flags6 & (RF6_FORGET))	vp[vn++] = "cause amnesia";
      if (flags6 & (RF6_DRAIN_MANA))	vp[vn++] = "drain mana";
      if (flags6 & (RF6_DISPEL))	vp[vn++] = "dispel magic";
      if (flags6 & (RF6_MIND_BLAST))	vp[vn++] = "cause mind blasting";
      if (flags6 & (RF6_BRAIN_SMASH))	vp[vn++] = "cause brain smashing";
      if (flags6 & (RF6_WOUND))
	{
	  if (spower < 7) vp[vn++] = "cause light wounds";
	  else if (spower < 15) vp[vn++] = "cause medium wounds";
	  else if (spower < 30) vp[vn++] = "cause serious wounds";
          else if (spower < 50) vp[vn++] = "cause critical wounds";
          else vp[vn++] = "cause mortal wounds";
        }
      if (flags6 & (RF6_SCARE))         vp[vn++] = "terrify";
      if (flags6 & (RF6_BLIND))         vp[vn++] = "blind";
      if (flags6 & (RF6_CONF))          vp[vn++] = "confuse";
      if (flags6 & (RF6_SLOW))		vp[vn++] = "slow";
      if (flags6 & (RF6_HOLD))		vp[vn++] = "paralyze";
      
      if (flags7 & (RF7_S_MONSTER))	vp[vn++] = "summon a monster";
      if (flags7 & (RF7_S_MONSTERS))	vp[vn++] = "summon monsters";
      if (flags7 & (RF7_S_ANT))		vp[vn++] = "summon ants";
      if (flags7 & (RF7_S_SPIDER))	vp[vn++] = "summon spiders";
                if (flags7 & (RF7_S_HOUND))             vp[vn++] = "summon hounds";
                if (flags7 & (RF7_S_ANIMAL))            vp[vn++] = "summon natural creatures";
                if (flags7 & (RF7_S_THIEF))             vp[vn++] = "summon thieves";
                if (flags7 & (RF7_S_BERTBILLTOM))       vp[vn++] = "summon his friends";
                if (flags7 & (RF7_S_DRAGON))            vp[vn++] = "summon a dragon";
                if (flags7 & (RF7_S_HI_DRAGON)) vp[vn++] = "summon Ancient Dragons";
                if (flags7 & (RF7_S_DEMON))             vp[vn++] = "summon a demon";
                if (flags7 & (RF7_S_HI_DEMON))  vp[vn++] = "summon Greater Demons";
                if (flags7 & (RF7_S_UNDEAD))            vp[vn++] = "summon an undead";
                if (flags7 & (RF7_S_HI_UNDEAD)) vp[vn++] = "summon Greater Undead";
                if (flags7 & (RF7_S_WRAITH))            vp[vn++] = "summon the Ringwraiths";
                if (flags7 & (RF7_S_UNIQUE))            vp[vn++] = "summon Unique Monsters";

                if (vn)
	{
	  magic = TRUE;
	  if (breath)
	    {
	      spoil_out(", and is also");
	    }
	  else
	    {
	      spoil_out(wd_che[msex]);
	      spoil_out(" is");
	    }
	  spoil_out(" magical, casting ");
	  
	  /* Describe magic */
	  if ((flags2 & (RF2_UDUN_MAGIC)) && (flags2 & (RF2_MORGUL_MAGIC))) 
	    spoil_out(" perilous spells of Udun and of Morgul");
	  if (flags2 & (RF2_MORGUL_MAGIC)) 
	    spoil_out(" Morgul-spells");
	  else if (flags2 & (RF2_UDUN_MAGIC)) 
	    spoil_out(" spells of Udun");
	  
	  else spoil_out(" spells");
	  
	  if (flags2 & (RF2_SMART)) spoil_out(" intelligently");
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" which ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" or ");
	      spoil_out(vp[i]);
	    }
	}
      
      if (breath || magic)
	{
	  sprintf(buf, "; 1 time in %d.  ",
		  100 / (r_ptr->freq_ranged));
	  spoil_out(buf);
	}
      
      /* Collect special abilities. */
      vn = 0;
      if (flags2 & (RF2_OPEN_DOOR)) vp[vn++] = "open doors";
      if (flags2 & (RF2_BASH_DOOR)) vp[vn++] = "bash down doors";
      if (flags2 & (RF2_PASS_WALL)) vp[vn++] = "pass through walls";
      if (flags2 & (RF2_KILL_WALL)) vp[vn++] = "bore through walls";
      if (flags2 & (RF2_MOVE_BODY)) vp[vn++] = "push past weaker monsters";
      if (flags2 & (RF2_KILL_BODY)) vp[vn++] = "destroy weaker monsters";
      if (flags2 & (RF2_TAKE_ITEM)) vp[vn++] = "pick up objects";
      if (flags2 & (RF2_KILL_ITEM)) vp[vn++] = "destroy objects";
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" can ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" and ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      if (flags2 & (RF2_PLAYER_GHOST))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" is a player ghost template.  ");
	}
      
      if (flags2 & (RF2_INVISIBLE))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" is invisible.  ");
	}
      if (flags2 & (RF2_COLD_BLOOD))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" is cold blooded.  ");
	}
      if (flags2 & (RF2_EMPTY_MIND))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" is not detected by telepathy.  ");
	}
      if (flags2 & (RF2_WEIRD_MIND))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" is rarely detected by telepathy.  ");
	}
      if (flags2 & (RF2_MULTIPLY))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" breeds explosively.  ");
	}
      if (flags2 & (RF2_REGENERATE))
	{
	  spoil_out(wd_che[msex]);
	  spoil_out(" regenerates quickly.  ");
	}
      
      /* Collect susceptibilities */
      vn = 0;
      if (flags3 & (RF3_HURT_ROCK)) vp[vn++] = "rock remover";
      if (flags3 & (RF3_HURT_LITE)) vp[vn++] = "bright light";
      if (flags3 & (RF3_HURT_FIRE)) vp[vn++] = "fire";
      if (flags3 & (RF3_HURT_COLD)) vp[vn++] = "cold";
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" is hurt by ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" and ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      /* Collect immunities */
      vn = 0;
      if (flags3 & (RF3_IM_ACID)) vp[vn++] = "acid";
      if (flags3 & (RF3_IM_ELEC)) vp[vn++] = "lightning";
      if (flags3 & (RF3_IM_FIRE)) vp[vn++] = "fire";
      if (flags3 & (RF3_IM_COLD)) vp[vn++] = "cold";
      if (flags3 & (RF3_IM_POIS)) vp[vn++] = "poison";
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" resists ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" and ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      /* Collect resistances */
      vn = 0;
      if (r_ptr->flags4 & (RF4_BRTH_LITE)) vp[vn++] = "light";
      if ((r_ptr->flags4 & (RF4_BRTH_DARK)) || 
	  (r_ptr->flags2 & (RF2_MORGUL_MAGIC)) || 
	  (r_ptr->flags3 & (RF3_ORC))) vp[vn++] = "darkness";
      
      if (flags4 & (RF4_BRTH_CONFU)) vp[vn++] = "confusion";
      if (flags4 & (RF4_BRTH_SOUND)) vp[vn++] = "sound";
      if (flags4 & (RF4_BRTH_SHARD)) vp[vn++] = "shards";
      if (flags4 & (RF4_BRTH_INER))  vp[vn++] = "inertia";
      if (flags4 & (RF4_BRTH_GRAV))  vp[vn++] = "gravity";
      if (flags4 & (RF4_BRTH_FORCE)) vp[vn++] = "force";
      if ((flags3 & (RF3_RES_WATE)) || (prefix(name, "Water"))) 
	vp[vn++] = "water";
      
      if ((r_ptr->flags4 & (RF4_BRTH_PLAS)) || (r_ptr->flags3 & (RF3_RES_PLAS)) || 
	  ((vn) && ((flags3 & (RF3_IM_ELEC)) || (flags3 & (RF3_IM_FIRE)))) || 
	  prefix(name, "Plasma")) vp[vn++] = "plasma";
      
      if ((r_ptr->flags3 & (RF3_RES_NEXU)) || prefix(name, "Nexus") || 
	  (r_ptr->flags4 & (RF4_BRTH_NEXUS))) vp[vn++] = "nexus";
      if ((r_ptr->flags3 & (RF3_UNDEAD)) || (r_ptr->flags3 & (RF3_RES_NETH)) || 
	  (r_ptr->flags4 & (RF4_BRTH_NETHR))) vp[vn++] = "nether";
      if ((flags3 & (RF3_RES_DISE)) || (r_ptr->flags4 & (RF4_BRTH_DISEN)) ||
	  prefix(name, "Disen")) vp[vn++] = "disenchantment";
      if (flags4 & (RF4_BRTH_TIME)) vp[vn++] = "time";
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" resists ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" and ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      /* Collect non-effects */
      vn = 0;
      if ((flags3 & (RF3_NO_STUN)) || (r_ptr->flags4 & (RF4_BRTH_SOUND)) ||
	  (r_ptr->flags4 & (RF4_BRTH_FORCE))) vp[vn++] = "stunned";
      if (flags3 & (RF3_NO_FEAR)) vp[vn++] = "frightened";
      if ((flags3 & (RF3_NO_CONF)) || (r_ptr->flags4 & (RF4_BRTH_CONFU)) || 
	  (r_ptr->flags4 & (RF4_BRTH_CHAOS))) vp[vn++] = "confused";
      if (flags3 & (RF3_NO_SLEEP)) vp[vn++] = "slept";
      
      if (vn)
	{
	  spoil_out(wd_che[msex]);
	  for (i = 0; i < vn; i++)
	    {
	      if (!i) spoil_out(" cannot be ");
	      else if (i < vn-1) spoil_out(", ");
	      else spoil_out(" or ");
	      spoil_out(vp[i]);
	    }
	  spoil_out(".  ");
	}
      
      spoil_out(wd_che[msex]);
      if (r_ptr->sleep > 200)	  spoil_out(" prefers to ignore");
      else if (r_ptr->sleep > 95) spoil_out(" pays very little attention to");
      else if (r_ptr->sleep > 75) spoil_out(" pays little attention to");
      else if (r_ptr->sleep > 45) spoil_out(" tends to overlook");
      else if (r_ptr->sleep > 25) spoil_out(" takes quite a while to see");
      else if (r_ptr->sleep > 10) spoil_out(" takes a while to see");
      else if (r_ptr->sleep > 5)  spoil_out(" is fairly observant of");
      else if (r_ptr->sleep > 3)  spoil_out(" is observant of");
      else if (r_ptr->sleep > 1)  spoil_out(" is very observant of");
      else if (r_ptr->sleep > 0)  spoil_out(" is vigilant for");
                else spoil_out(" is ever vigilant for");

                sprintf(buf, " intruders, which %s may notice from %d feet.  ",
                        wd_lhe[msex], (adult_small_device ? 5 : 10) * r_ptr->aaf);
                spoil_out(buf);

                i = 0;
                j = 0;
                if (flags1 & (RF1_DROP_40)) i += 1;
                if (flags1 & (RF1_DROP_60)) i += 1;
                if (flags1 & (RF1_DROP_ONE)) i += 1;
                if (flags1 & (RF1_DROP_TWO)) i += 2;
                if (flags1 & (RF1_DROP_PLUS_5)) j += 5;
                if (flags1 & (RF1_DROP_PLUS_10)) j += 10;

                /*
                 * Mega-Hack - the Living Dungeon hack
                 * Creatures with enhanced drop depth and only gold,
                 * actually get more drops as well.
                 */
                if (flags1 & (RF1_ONLY_GOLD))
                {
                        if (flags1 & (RF1_DROP_PLUS_5)) i += 3;
                        if (flags1 & (RF1_DROP_PLUS_10)) i += 6;
                }

                /* Drops gold and/or items */
                if (i)
	{
	  sin = FALSE;
	  spoil_out(wd_che[msex]);
	  spoil_out(" will carry");
	  
	  if (i == 1)
	    {
	      spoil_out(" a");
	      sin = TRUE;
	    }
	  else if (i == 2)
	    {
	      spoil_out(" one or two");
	    }
	  else
	    {
	      sprintf(buf, " up to %u", i);
	      spoil_out(buf);
	    }
	  
	  if (flags1 & (RF1_DROP_GREAT))
	    {
	      if (sin) spoil_out("n");
	      spoil_out(" exceptional object");
	    }
	  else if (flags1 & (RF1_DROP_GOOD))
	    {
	      spoil_out(" good object");
	    }
	  else if (flags1 & (RF1_DROP_CHEST))
	    {
	      spoil_out(" chest");
	    }
	  else if (flags1 & (RF1_ONLY_ITEM))
	    {
	      if (sin) spoil_out("n");
	      spoil_out(" object");
	    }
	  else if (flags1 & (RF1_ONLY_GOLD))
	    {
	      spoil_out(" treasure");
	    }
	  else
	    {
	      if (sin) spoil_out("n");
	      spoil_out(" object");
	      if (i > 1) spoil_out("s");
	      spoil_out(" or treasure");
                        }
                        if (i > 1) spoil_out("s");

                        if (j > 0)
                        {
                                sprintf(buf, " %u levels out of depth", i);
                                spoil_out(buf);
                        }

                        if (flags1 & (RF1_DROP_CHOSEN))
                        {
                                spoil_out(", in addition to chosen objects");
	    }
	  
	  spoil_out(".  ");
	}
      
      /* Count the actual attacks */
      for (i = 0, j = 0; j < 4; j++)
	{
	  if (r_ptr->blow[j].method) i++;
	}
      
      /* Examine the actual attacks */
      for (k = 0, j = 0; j < 4; j++)
	{
	  if (!r_ptr->blow[j].method) continue;
	  
	  /* No method yet */
	  p = "???";
	  
	  /* Acquire the method */
	  switch (r_ptr->blow[j].method)
	    {
	    case RBM_HIT:    p = "hit"; break;
	    case RBM_TOUCH:  p = "touch"; break;
	    case RBM_PUNCH:  p = "punch"; break;
	    case RBM_KICK:   p = "kick"; break;
	    case RBM_CLAW:   p = "claw"; break;
	    case RBM_BITE:   p = "bite"; break;
	    case RBM_STING:  p = "sting"; break;
	    case RBM_XXX1:   break;
	    case RBM_BUTT:   p = "butt"; break;
	    case RBM_CRUSH:  p = "crush"; break;
	    case RBM_ENGULF: p = "engulf"; break;
	    case RBM_XXX2:   break;
	    case RBM_CRAWL:  p = "crawl on you"; break;
	    case RBM_DROOL:  p = "drool on you"; break;
	    case RBM_SPIT:   p = "spit"; break;
	    case RBM_XXX3:   break;
	    case RBM_GAZE:   p = "gaze"; break;
	    case RBM_WAIL:   p = "wail"; break;
	    case RBM_SPORE:  p = "release spores"; break;
	    case RBM_XXX4:   break;
	    case RBM_BEG:    p = "beg"; break;
	    case RBM_INSULT: p = "insult"; break;
	    case RBM_SNEER:  p = "moan"; break;
	    case RBM_REQUEST:p = "offer to trade"; break;
	    }
	  
	  
	  /* Default effect */
	  q = "???";
	  
	  /* Acquire the effect */
	  switch (r_ptr->blow[j].effect)
	    {
	    case RBE_HURT:	q = "attack"; break;
	    case RBE_POISON:	q = "poison"; break;
	    case RBE_UN_BONUS:	q = "disenchant"; break;
	    case RBE_UN_POWER:	q = "drain charges"; break;
	    case RBE_EAT_GOLD:	q = "steal gold"; break;
	    case RBE_EAT_ITEM:	q = "steal items"; break;
	    case RBE_EAT_FOOD:	q = "eat your food"; break;
	    case RBE_EAT_LITE:	q = "absorb light"; break;
	    case RBE_ACID:	q = "shoot acid"; break;
	    case RBE_ELEC:	q = "electrify"; break;
	    case RBE_FIRE:	q = "burn"; break;
	    case RBE_COLD:	q = "freeze"; break;
	    case RBE_BLIND:	q = "blind"; break;
	    case RBE_CONFUSE:	q = "confuse"; break;
	    case RBE_TERRIFY:	q = "terrify"; break;
	    case RBE_PARALYZE:	q = "paralyze"; break;
	    case RBE_LOSE_STR:	q = "reduce strength"; break;
	    case RBE_LOSE_INT:	q = "reduce intelligence"; break;
	    case RBE_LOSE_WIS:	q = "reduce wisdom"; break;
	    case RBE_LOSE_DEX:	q = "reduce dexterity"; break;
	    case RBE_LOSE_CON:	q = "reduce constitution"; break;
	    case RBE_LOSE_CHR:	q = "reduce charisma"; break;
	    case RBE_LOSE_ALL:	q = "reduce all stats"; break;
	    case RBE_SHATTER:	q = "shatter"; break;
	    case RBE_EXP_10:	q = "lower experience (by 10d6+)"; break;
	    case RBE_EXP_20:	q = "lower experience (by 20d6+)"; break;
	    case RBE_EXP_40:	q = "lower experience (by 40d6+)"; break;
	    case RBE_EXP_80:	q = "lower experience (by 80d6+)"; break;
	    }
	  
	  
	  if (!k)
	    {
	      spoil_out(wd_che[msex]);
	      spoil_out(" can ");
	    }
	  else if (k < i-1)
	    {
	      spoil_out(", ");
	    }
	  else
	    {
	      spoil_out(", and ");
	    }
	  
	  /* Describe the method */
	  spoil_out(p);
	  
	  /* Describe the effect, if any */
	  if (r_ptr->blow[j].effect)
	    {
	      spoil_out(" to ");
	      spoil_out(q);
	      if (r_ptr->blow[j].d_dice && r_ptr->blow[j].d_side)
		{
		  spoil_out(" with damage");
		  if (r_ptr->blow[j].d_side == 1)
		    sprintf(buf, " %d", r_ptr->blow[j].d_dice);
		  else
		    sprintf(buf, " %dd%d",
			    r_ptr->blow[j].d_dice, r_ptr->blow[j].d_side);
		  spoil_out(buf);
		}
	    }
	  
	  k++;
	}
      
      if (k)
	{
	  spoil_out(".  ");
	}
      else if (flags1 & (RF1_NEVER_BLOW))
	{
	  sprintf(buf, "%s has no physical attacks.  ", wd_che[msex]);
	  spoil_out(buf);
	}
      
      spoil_out(NULL);
    }
  
  /* Check for errors */
  if (ferror(fff) || my_fclose(fff))
    {
      msg_print("Cannot close spoiler file.");
      return;
    }
  
  msg_print("Successfully created a spoiler file.");
}






/*
 * Forward declare
 */
extern void do_cmd_spoilers(void);

/*
 * Create Spoiler files
 */
void do_cmd_spoilers(void)
{
  int ch;
  
  
  /* Save screen */
  screen_save();
  
  
  /* Drop priv's */
  safe_setuid_drop();
  
  
  /* Interact */
  while (1)
    {
      /* Clear screen */
      Term_clear();
      
      /* Info */
      prt("Create a spoiler file.", 2, 0);
      
      /* Prompt for a file */
      prt("(1) Brief Object Info   (obj-desc.spo)", 5, 5);
      prt("(2) Brief Artifact Info (artifact.spo)", 6, 5);
      prt("(3) Brief Monster Info  (mon-desc.spo)", 7, 5);
      prt("(4) Full Monster Info   (mon-info.spo)", 8, 5);
      
      prt("(5) See what objects appear on this level  (obj-gen.spo)", 10, 5);
      prt("(6) See what monsters appear on this level (mon-gen.spo)", 11, 5);
      
      /* Prompt */
      prt("Command: ", 12, 0);
      
      /* Get a choice */
      ch = inkey();
      
      /* Escape */
      if (ch == ESCAPE)
	{
	  break;
	}
      
      /* Option (1) */
      else if (ch == '1')
	{
	  spoil_obj_desc("obj-desc.spo");
	}
      
      /* Option (2) */
      else if (ch == '2')
	{
	  spoil_artifact("artifact.spo");
	}
      
      /* Option (3) */
      else if (ch == '3')
	{
	  spoil_mon_desc("mon-desc.spo");
	}
      
      /* Option (4) */
      else if (ch == '4')
	{
	  spoil_mon_info("mon-info.spo");
	}
      
      /* Option (5) */
      else if (ch == '5')
	{
	  spoil_obj_gen("obj-gen.spo");
	}
      
      /* Option (6) */
      else if (ch == '6')
	{
	  spoil_mon_gen("mon-gen.spo");
	}
      
      /* Oops */
      else
	{
	  bell("Illegal command for spoilers!");
	}
      
      /* Flush messages */
      msg_print(NULL);
    }
  
  
  /* Grab priv's */
  safe_setuid_grab();
  
  
  /* Load screen */
  screen_load();
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif


