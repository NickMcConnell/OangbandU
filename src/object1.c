/* File: object1.c */

/* Object flavors, colors, easy-know, display of modifiers (eg. "(+2 to
 * strength)"), naming, puralizations, etc., what items
 * go where in equipment, equipment-related strings, etc., and inventory
 * management and display functions.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"


/*
 * Max sizes of the following arrays.  Changes for FAangband 0.2.2 from
 * NPPangband.
 */
#define MAX_SYLLABLES 164       /* Used with scrolls (see below) */

/*
 * Syllables for scrolls (must be 1-4 letters each)
 */

static cptr syllables[MAX_SYLLABLES] =
{
	"a", "ab", "ag", "aks", "ala", "an", "ankh", "app", "arg", "arze",
	"ash", "aus", "ban", "bar", "bat", "bek", "bie", "bin", "bit", "bjor",
	"blu", "bot", "bu", "byt", "can", "comp", "con", "cos", "cre", "czra",
	"dalf", "dan", "den", "der", "doe", "dok", "dora", "eep", "el", "eng",
	"er", "ere", "erk", "esh", "evs", "fa", "fid", "fin", "flit", "for",
	"fri", "fu", "gan", "gar", "glen", "gop", "gre", "ha", "he", "hyd",
	"i", "ing", "ion", "ip", "ish", "it", "ite", "iv", "jo", "kho",
	"kli", "klis", "la", "lech", "man", "mar", "me", "mi", "mic", "mik",
	"mon", "mung", "mur", "nag", "nej", "nelg", "nep", "ner", "nes", "nis",
	"nih", "nin", "o", "od", "ood", "org", "orn", "ox", "oxy", "pay",
	"pet","ple", "plu", "po", "pot", "prok", "qua", "qur", "re", "rea",
	"rhov", "ri", "ro", "rog", "rok", "rol", "sa", "san", "sat","see",
	"sef", "seh", "shu", "ski", "sna", "sne", "snik","sno", "so", "sol",
	"sri", "sta", "sun", "ta", "tab", "tem", "ther", "ti", "tox", "trol",
	"tue", "turs", "u", "ulk", "um", "un", "uni", "ur", "val", "viv",
	"vly", "vom", "wah", "wed", "werg", "wex", "whon", "wun", "x", "yerg",
	"yp", "zun", "tri", "blaa"
};


/*
 * Hold the titles of scrolls, 6 to 14 characters each
 * Also keep an array of scroll colors (always WHITE for now)
 */

static char scroll_adj[MAX_TITLES][16];

static void flavor_assign_fixed(void)
{
	int i, j;
  
	for (i = 0; i < z_info->flavor_max; i++)
	{
		flavor_type *flavor_ptr = &flavor_info[i];
      
		/* Skip random flavors */
		if (flavor_ptr->sval == SV_UNKNOWN) continue;
      
		for (j = 0; j < z_info->k_max; j++)
		{
			/* Skip other objects */
			if ((k_info[j].tval == flavor_ptr->tval) &&
			    (k_info[j].sval == flavor_ptr->sval))
			{
				/* Store the flavor index */
				k_info[j].flavor = i;
			}
		}
	}
}


static void flavor_assign_random(byte tval)
{
	int i, j;
	int flavor_count = 0;
	int choice;
  
	/* Count the random flavors for the given tval */
	for (i = 0; i < z_info->flavor_max; i++)
        {
		if ((flavor_info[i].tval == tval) &&
		    (flavor_info[i].sval == SV_UNKNOWN))
		{
			flavor_count++;
		}
        }
  
	for (i = 0; i < z_info->k_max; i++)
	{
		/* Skip other object types */
		if (k_info[i].tval != tval) continue;

		/* Skip objects that already are flavored */
		if (k_info[i].flavor != 0) continue;
      
		/* HACK - Ordinary food is "boring" */
		if ((tval == TV_FOOD) && (k_info[i].sval >= SV_FOOD_MIN_FOOD))
			continue;
      
		if (!flavor_count) quit_fmt("Not enough flavors for tval %d.", tval);
      
		/* Select a flavor */
		choice = rand_int(flavor_count);
      
		/* Find and store the flavor */
		for (j = 0; j < z_info->flavor_max; j++)
		{
			/* Skip other tvals */
			if (flavor_info[j].tval != tval) continue;
          
			/* Skip assigned svals */
			if (flavor_info[j].sval != SV_UNKNOWN) continue;
          
			if (choice == 0)
			{
				/* Store the flavor index */
				k_info[i].flavor = j;
              
				/* Mark the flavor as used */
				flavor_info[j].sval = k_info[i].sval;
              
				/* One less flavor to choose from */
				flavor_count--;
              
				break;
			}
          
			choice--;
		}
	}
}

/*
 * Certain items, if aware, are known instantly
 * This function is used only by "flavor_init()"
 *
 * Add "EASY_KNOW" flag to "k_info.txt" file.  XXX XXX XXX
 */
static bool object_easy_know(int i)
{
	object_kind *k_ptr = &k_info[i];

	/* Analyze the "tval" */
	switch (k_ptr->tval)
	{
		/* Spellbooks */
	case TV_MAGIC_BOOK:
	case TV_PRAYER_BOOK:
	case TV_DRUID_BOOK:
	case TV_NECRO_BOOK:

	{
		return (TRUE);
	}

	/* Simple items */
	case TV_FLASK:
	case TV_JUNK:
	case TV_BOTTLE:
	case TV_SKELETON:
	case TV_SPIKE:
	{
		return (TRUE);
	}

	/* All Food, Potions, Scrolls, Rods */
	case TV_FOOD:
	case TV_POTION:
	case TV_SCROLL:
	case TV_ROD:
	{
		return (TRUE);
	}

	/* Some Rings, Amulets, Lites */
	case TV_RING:
	case TV_AMULET:
	case TV_LITE:
	{
		if (k_ptr->flags3 & (TR3_EASY_KNOW)) return (TRUE);
		return (FALSE);
	}
	}

	/* Nope */
	return (FALSE);
}


/*
 * Prepare the "variable" part of the "k_info" array.
 *
 * The "color"/"metal"/"type" of an item is its "flavor".
 * For the most part, flavors are assigned randomly each game.
 *
 * Initialize descriptions for the "colored" objects, including:
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.
 *
 * The first 4 entries for potions are fixed (Water, Apple Juice,
 * Slime Mold Juice, Unused Potion).
 *
 * Scroll titles are always between 6 and 14 letters long.  This is
 * ensured because every title is composed of whole words, where every
 * word is from 1 to 8 letters long (one or two syllables of 1 to 4
 * letters each), and that no scroll is finished until it attempts to
 * grow beyond 15 letters.  The first time this can happen is when the
 * current title has 6 letters and the new word has 8 letters, which
 * would result in a 6 letter scroll title.
 *
 * Duplicate titles are avoided by requiring that no two scrolls share
 * the same first four letters (not the most efficient method, and not
 * the least efficient method, but it will always work).
 *
 * Hack -- make sure everything stays the same for each saved game
 * This is accomplished by the use of a saved "random seed", as in
 * "town_gen()".  Since no other functions are called while the special
 * seed is in effect, so this function is pretty "safe".
 */
void flavor_init(void)
{
        int i, j;


        /* Hack -- Use the "simple" RNG */
        Rand_quick = TRUE;

	/* Hack -- Induce consistant flavors */
	Rand_value = seed_flavor;
  
  
	flavor_assign_fixed();
  
	flavor_assign_random(TV_RING);
	flavor_assign_random(TV_AMULET);
	flavor_assign_random(TV_STAFF);
	flavor_assign_random(TV_WAND);
	flavor_assign_random(TV_ROD);
	flavor_assign_random(TV_FOOD);
	flavor_assign_random(TV_POTION);
	flavor_assign_random(TV_SCROLL);
  
	/* Scrolls (random titles, always white) */
	for (i = 0; i < MAX_TITLES; i++)
	{
		/* Get a new title */
		while (TRUE)
		{
			char buf[80];

			bool okay;

			/* Start a new title */
			buf[0] = '\0';

			/* Collect words until done */
			while (1)
			{
				int q, s;

				char tmp[80];

				/* Start a new word */
				tmp[0] = '\0';

				/* Choose one or two syllables */
				s = ((rand_int(100) < 30) ? 1 : 2);

				/* Add a one or two syllable word */
				for (q = 0; q < s; q++)
				{
					/* Add the syllable */
					strcat(tmp, syllables[rand_int(MAX_SYLLABLES)]);
				}

				/* Stop before getting too long */
				if (strlen(buf) + 1 + strlen(tmp) > 15) break;

				/* Add a space */
				strcat(buf, " ");

				/* Add the word */
				strcat(buf, tmp);
			}
          
			/* Save the title */
			my_strcpy(scroll_adj[i], buf+1, sizeof(scroll_adj[0]));
          
			/* Assume okay */
			okay = TRUE;

			/* Check for "duplicate" scroll titles */
			for (j = 0; j < i; j++)
			{
				cptr hack1 = scroll_adj[j];
				cptr hack2 = scroll_adj[i];

				/* Compare first four characters */
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;

				/* Not okay */
				okay = FALSE;

				/* Stop looking */
				break;
			}

                        /* Break when done */
                        if (okay) break;
                }
        }


	/* Hack -- Use the "complex" RNG */
	Rand_quick = FALSE;
  
	/* Analyze every object */
	for (i = 1; i < z_info->k_max; i++)
	{
      
		object_kind *k_ptr = &k_info[i];
      
		/* Skip "empty" objects */
		if (!k_ptr->name) continue;
      
		/* No flavor yields aware */
		if (!k_ptr->flavor) k_ptr->aware = TRUE;
      
		/* Check for "easily known" */
		k_ptr->easy_know = object_easy_know(i);
	}
}


/*
 * Get the inventory color for an object.
 *
 * Hack - set the listing color of a spellbook with no useable spells 
 * to grey -LM-
 */
byte proc_list_color_hack(object_type *o_ptr)
{
	/* Hack -- Spellbooks with no useable spells are grey. */
	if ((mp_ptr->spell_book == o_ptr->tval)
	    && (mp_ptr->book_start_index[o_ptr->sval] ==
		mp_ptr->book_start_index[o_ptr->sval + 1]))
	{
		return (TERM_SLATE);
	}

	/* Otherwise, get the color normally. */
	else return(tval_to_attr[o_ptr->tval & 0x7F]);
}


/*
 * Reset the "visual" lists
 *
 * This involves resetting various things to their "default" state.
 *
 * If the "prefs" flag is TRUE, then we will also load the appropriate
 * "user pref file" based on the current setting of the "use_graphics"
 * flag.  This is useful for switching "graphics" on/off.
 *
 * The features, objects, and monsters, should all be encoded in the
 * relevant "font.pref" and/or "graf.prf" files.  XXX XXX XXX
 *
 * The "prefs" parameter is no longer meaningful.  XXX XXX XXX
 */
void reset_visuals(bool unused)
{
	int i;
  
  
	/* Extract default attr/char code for features */
	for (i = 0; i < z_info->f_max; i++)
	{
		feature_type *f_ptr = &f_info[i];
      
		/* Assume we will use the underlying values */
		f_ptr->x_attr = f_ptr->d_attr;
		f_ptr->x_char = f_ptr->d_char;
	}
  
	/* Extract default attr/char code for objects */
	for (i = 0; i < z_info->k_max; i++)
	{
		object_kind *k_ptr = &k_info[i];
      
		/* Default attr/char */
		k_ptr->x_attr = k_ptr->d_attr;
		k_ptr->x_char = k_ptr->d_char;
	}
  
	/* Extract default attr/char code for monsters */
	for (i = 0; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];
      
		/* Default attr/char */
		r_ptr->x_attr = r_ptr->d_attr;
		r_ptr->x_char = r_ptr->d_char;
	}
  
  
	/* Extract default attr/char code for flavors */
	for (i = 0; i < z_info->flavor_max; i++)
	{
		flavor_type *flavor_ptr = &flavor_info[i];
      
		/* Default attr/char */
		flavor_ptr->x_attr = flavor_ptr->d_attr;
		flavor_ptr->x_char = flavor_ptr->d_char;
	}

	/* Extract attr/chars for inventory objects (by tval) */
	for (i = 0; i < 128; i++)
	{
		/* Default to white */
		tval_to_attr[i] = TERM_WHITE;
	}


	/* Graphic symbols */
	if (use_graphics)
	{
		/* Process "graf.prf" */
		process_pref_file("graf.prf");
	}

	/* Normal symbols */
	else
	{
		/* Process "font.prf" */
		process_pref_file("font.prf");
	}
}


/*
 * Puts a very stripped-down version of an object's name into buf.
 * If easy_know is TRUE, then the IDed names are used, otherwise
 * flavours, scroll names, etc will be used.
 *
 * Just truncates if the buffer isn't big enough.
 */
void object_kind_name(char *buf, size_t max, int k_idx, bool easy_know)
{
	char *t;
  
	object_kind *k_ptr = &k_info[k_idx];
  
	/* If not aware, use flavor */
	if (!easy_know && !k_ptr->aware && k_ptr->flavor)
	{
		if (k_ptr->tval == TV_SCROLL)
		{
			strnfmt(buf, max, "\"%s\"", scroll_adj[k_ptr->sval]);
		}
		else if (k_ptr->tval == TV_FOOD && k_ptr->sval < SV_FOOD_MIN_FOOD)
		{
			strnfmt(buf, max, "%s Mushroom", 
				flavor_text + flavor_info[k_ptr->flavor].text);
		}
		else
		{
			/* Plain flavour (e.g. Copper) will do. */
			my_strcpy(buf, flavor_text + flavor_info[k_ptr->flavor].text, max);
		}
	}
  
	/* Use proper name (Healing, or whatever) */
	else
	{
		cptr str = (k_name + k_ptr->name);
      
		if (k_ptr->tval == TV_FOOD && k_ptr->sval < SV_FOOD_MIN_FOOD)
		{
			my_strcpy(buf, "Mushroom of ", max);
			max -= strlen(buf);
			t = buf + strlen(buf);
		}
		else
		{
			t = buf;
		}
      
		/* Skip past leading characters */
		while ((*str == ' ') || (*str == '&')) str++;
      
		/* Copy useful chars */
		for (; *str && max > 1; str++)
		{
			/* Pluralizer for irregular plurals */
			/* Useful for languages where adjective changes for plural */
			if (*str == '|')
			{
				/* Process singular part */
				for (str++; *str != '|' && max > 1; str++) 
				{
					*t++ = *str;
					max--;
				}
              
				/* Process plural part */
				for (str++; *str != '|'; str++) ;
			}
          
			/* English plural indicator can simply be skipped */
			else if (*str != '~')
			{
				*t++ = *str;
				max--;
			}
		}
      
		/* Terminate the new name */
		*t = '\0';
	}
}





/*
 * Obtain the "flags" for an item
 */
void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Base object */
	(*f1) = k_ptr->flags1;
	(*f2) = k_ptr->flags2;
	(*f3) = k_ptr->flags3;

	/* Artifact */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		(*f1) = a_ptr->flags1;
		(*f2) = a_ptr->flags2;
		(*f3) = a_ptr->flags3;
	}

	/* Ego-item */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		(*f1) |= e_ptr->flags1;
		(*f2) |= e_ptr->flags2;
		(*f3) |= e_ptr->flags3;
	}

	/* Extra powers.  Special throwing weapon ability added. */
	switch (o_ptr->xtra1)
	{
	case OBJECT_XTRA_TYPE_SUSTAIN:
	{
		/* OBJECT_XTRA_WHAT_SUSTAIN == 2 */
		(*f2) |= (OBJECT_XTRA_BASE_SUSTAIN << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_RESIST:
	{
		/* OBJECT_XTRA_WHAT_RESIST == 2 */
		(*f2) |= (OBJECT_XTRA_BASE_RESIST << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_POWER:
	{
		/* OBJECT_XTRA_WHAT_POWER == 3 */
		(*f3) |= (OBJECT_XTRA_BASE_POWER << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_BALANCE:
	{
		/* OBJECT_XTRA_WHAT_BALANCE == 1 */
		(*f1) |= (OBJECT_XTRA_BASE_BALANCE << o_ptr->xtra2);
		break;
	}
	}
}



/*
 * Obtain the "flags" for an item which is known to the player
 */
void object_flags_known(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3)
{
	bool spoil = FALSE;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Clear */
	(*f1) = (*f2) = (*f3) = 0L;

	/* Must be identified */
	if (!object_known_p(o_ptr)) return;

	/* Base object */
	(*f1) = k_ptr->flags1;
	(*f2) = k_ptr->flags2;
	(*f3) = k_ptr->flags3;

	/* Ego-item */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		(*f1) |= e_ptr->flags1;
		(*f2) |= e_ptr->flags2;
		(*f3) |= e_ptr->flags3;
	}

#ifdef SPOIL_ARTIFACTS
	/* Full knowledge for some artifacts */
	if (artifact_p(o_ptr)) spoil = TRUE;
#endif

#ifdef SPOIL_EGO_ITEMS
	/* Full knowledge for some ego-items */
	if (ego_item_p(o_ptr)) spoil = TRUE;
#endif

	/* Need full knowledge or spoilers */
	if (!spoil && !(o_ptr->ident & IDENT_MENTAL)) return;

	/* Artifact */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		(*f1) = a_ptr->flags1;
		(*f2) = a_ptr->flags2;
		(*f3) = a_ptr->flags3;
	}

	/* Full knowledge for *identified* objects */
	if (!(o_ptr->ident & IDENT_MENTAL)) return;

	/* Extra powers.  Special throwing weapon ability added. */
	switch (o_ptr->xtra1)
	{
	case OBJECT_XTRA_TYPE_SUSTAIN:
	{
		/* OBJECT_XTRA_WHAT_SUSTAIN == 2 */
		(*f2) |= (OBJECT_XTRA_BASE_SUSTAIN << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_RESIST:
	{
		/* OBJECT_XTRA_WHAT_RESIST == 2 */
		(*f2) |= (OBJECT_XTRA_BASE_RESIST << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_POWER:
	{
		/* OBJECT_XTRA_WHAT_POWER == 3 */
		(*f3) |= (OBJECT_XTRA_BASE_POWER << o_ptr->xtra2);
		break;
	}

	case OBJECT_XTRA_TYPE_BALANCE:
	{
		/* OBJECT_XTRA_WHAT_BALANCE == 1 */
		(*f1) |= (OBJECT_XTRA_BASE_BALANCE << o_ptr->xtra2);
		break;
	}
	}
}



/*
 * Efficient version of '(T) += strfmt((T), "%c", (C))'
 */
#define object_desc_chr_macro(T,C) do {		\
						\
		/* Copy the char */		\
		*(T)++ = (C);			\
						\
	} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%s", (S))'
 */
#define object_desc_str_macro(T,S) do {		\
						\
		cptr s = (S);			\
						\
		/* Copy the string */		\
		while (*s) *(T)++ = *s++;	\
						\
	} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%u", (N))'
 */
#define object_desc_num_macro(T,N) do {					\
									\
		uint n = (N);						\
									\
		uint p;							\
									\
		/* Find "size" of "n" */				\
		for (p = 1; n >= p * 10; p = p * 10) /* loop */;	\
									\
		/* Dump each digit */					\
		while (p >= 1)						\
		{							\
			/* Dump the digit */				\
			*(T)++ = I2D(n / p);				\
									\
			/* Remove the digit */				\
			n = n % p;					\
									\
			/* Process next digit */			\
			p = p / 10;					\
		}							\
									\
	} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%+d", (I))'
 */
#define object_desc_int_macro(T,I) do {			\
							\
		sint i = (I);				\
							\
		/* Negative */				\
		if (i < 0)				\
		{					\
			/* Take the absolute value */	\
			i = 0 - i;			\
							\
			/* Use a "minus" sign */	\
			*(T)++ = '-';			\
		}					\
							\
		/* Positive (or zero) */		\
		else					\
		{					\
			/* Use a "plus" sign */		\
			*(T)++ = '+';			\
		}					\
							\
		/* Dump the number itself */		\
		object_desc_num_macro(T, i);		\
							\
	} while (0)


/*
 * Creates a description of the item "o_ptr", and stores it in "out_val".
 *
 * One can choose the "verbosity" of the description, including whether
 * or not the "number" of items should be described, and how much detail
 * should be used when describing the item.
 *
 * The given "buf" must be 80 chars long to hold the longest possible
 * description, which can get pretty long, including incriptions, such as:
 * "no more Maces of Disruption (Defender) (+10,+10) [+5] (+3 to stealth)".
 * Note that the inscription will be clipped to keep the total description
 * under 79 chars (plus a terminator).
 *
 * Note the use of "object_desc_int_macro()" and "object_desc_num_macro()"
 * and "object_desc_str_macro()" and "object_desc_chr_macro()" as extremely
 * efficient, portable, versions of some common "strfmt()" commands (without
 * the bounds checking or termination writing), which allow a pointer to
 * efficiently move through a buffer while modifying it in various ways.
 *
 * Various improper uses and/or placements of "&" or "~" characters can
 * easily induce out-of-bounds memory accesses.   Some of these could be
 * easily checked for, if efficiency was not a concern.
 *
 * Note that all ego-items (when known) append an "Ego-Item Name", unless
 * the item is also an artifact, which should NEVER happen.
 *
 * Note that all artifacts (when known) append an "Artifact Name", so we
 * have special processing for "Specials" (artifact Lites, Rings, Amulets).
 * The "Specials" never use "modifiers" if they are "known", since they
 * have special "descriptions", such as "The Necklace of the Dwarves".
 *
 * Special Lite's use the "k_info" base-name (Phial, Star, or Arkenstone),
 * plus the artifact name, just like any other artifact, if known.
 *
 * Special Rings and Amulets, if not "aware", use the same code as normal
 * rings and amulets, and if "aware", use the "k_info" base-name (Ring or
 * Amulet or Necklace).   They will NEVER "append" the "k_info" name.  But,
 * they will append the artifact name, just like any artifact, if known.
 *
 * Special Wands, Rods, and Staffs will be identified even if they are
 * only aware, not fully identified.  Of course, charges will not be shown.
 * -LM-
 *
 * None of the Special Rings/Amulets are "EASY_KNOW", though they could be,
 * at least, those which have no "pluses", such as the three artifact lites.
 *
 * Hack -- Display "The One Ring" as "a Plain Gold Ring" until aware.
 *
 * The "pluralization" rules are extremely hackish, in fact, for efficiency,
 * we only handle things like "torch"/"torches" and "cutlass"/"cutlasses",
 * and we would not handle "box"/"boxes", or "knife"/"knives", correctly.
 *
 * If "pref" is true then a "numeric" prefix will be pre-pended, else is is
 * assumed that a string such as "The" or "Your" will be pre-pended later.
 *
 * Modes ("pref" is TRUE):
 *   0 -- Chain Mail of Death
 *   1 -- A Cloak of Death [1,+3]
 *   2 -- An Amulet of Death [1,+3] (+2 to Stealth)
 *   3 -- 5 Rings of Death [1,+3] (+2 to Stealth) {nifty}
 *
 * Modes ("pref" is FALSE):
 *   0 -- Chain Mail of Death
 *   1 -- Cloak of Death [1,+3]
 *   2 -- Amulet of Death [1,+3] (+2 to Stealth)
 *   3 -- Rings of Death [1,+3] (+2 to Stealth) {nifty}
 *
 *  New mode - 4 - is like 3 but for small screen
 */
void object_desc(char *buf, object_type *o_ptr, int pref, int mode)
{
	cptr basenm, modstr;

	int power;

	bool easy_know;
	bool aware;
	bool known;

	bool flavor;

	bool append_name;

	bool show_weapon;
	bool show_armour;

	bool fake_art;

	char *b;

	char *t;

	cptr s;
	cptr u;
	cptr v = NULL;

	char p1 = '(', p2 = ')';
	char b1 = '[', b2 = ']';
	char c1 = '{', c2 = '}';

	char discount_buf[80];
	char insc_buf[80];

	u32b f1, f2, f3;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3);


	/* See if the object is "aware" */
	aware = (object_aware_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "known" */
	known = (object_known_p(o_ptr) ? TRUE : FALSE);

	easy_know = FALSE;

	/* See if the object is "flavored" */
	flavor = (k_ptr->flavor ? TRUE : FALSE);

	/* Allow flavors to be hidden when aware */
	if (aware && !show_flavors) flavor = FALSE;
  
	/* Hack -- mark-to-squelch worthless items XXX */
	if (!k_ptr->everseen && aware && squelch_worthless)
	{
		if (object_value(o_ptr) == 0)
		{
			k_ptr->squelch = TRUE;
			p_ptr->notice |= PN_SQUELCH;
		}
	}
  
	/* We've seen it at least once now we're aware of it */
	if (aware) k_ptr->everseen = TRUE;

	/* Assume no name appending */
	append_name = FALSE;
  
	/* Assume no need to show "weapon" bonuses */
	show_weapon = FALSE;

	/* Assume no need to show "armour" bonuses */
	show_armour = FALSE;

	/* Check to see if it has been inscribed as a fake artifact */
	fake_art = (o_ptr->note && strchr(quark_str(o_ptr->note), '#'));

	/* Extract default "base" string */
	basenm = (k_name + k_ptr->name);

	/* Assume no "modifier" string */
	modstr = "";
  
	/* Mode 4 is small screen only */
	if ((mode == 4) && (!small_screen)) mode = 3;

	/* Analyze the object */
	switch (o_ptr->tval)
	{
		/* Some objects are easy to describe */
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	case TV_SPIKE:
	case TV_FLASK:
	case TV_CHEST:
	{
		break;
	}

	/* Missiles/Bows/Weapons */
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
	case TV_BOW:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
	{
		show_weapon = TRUE;
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
		show_armour = TRUE;
		break;
	}

	/* Lites (including a few "Specials") */
	case TV_LITE:
	{
		break;
	}

	/* Amulets (including a few "Specials") */
	case TV_AMULET:
	{
		/* Hack -- Known artifacts */
		if (artifact_p(o_ptr) && aware) break;
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Amulet~" : "& Amulet~");

		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& \"";
        
		break;
	}
      
	/* Rings (including a few "Specials") */
	case TV_RING:
	{
		/* Hack -- Known artifacts */
		if (artifact_p(o_ptr) && aware) break;
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Ring~" : "& Ring~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& =";
        
		break;
	}

	/* Staffs */
	case TV_STAFF:
	{
		/* Hack -- Known artifacts */
		if (artifact_p(o_ptr) && aware)
		{
			easy_know = TRUE;
			break;
		}
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Staff~" : "& Staff~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& _";
        
		break;
	}
      
	/* Wands */
	case TV_WAND:
	{
		/* Hack -- Known artifacts */
		if (artifact_p(o_ptr) && aware)
		{
			easy_know = TRUE;
			break;
		}
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Wand~" : "& Wand~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& -";
        
		break;
	}
      
	/* Rods */
	case TV_ROD:
	{
		/* Hack -- Known artifacts */
		if (artifact_p(o_ptr) && aware)
		{
			easy_know = TRUE;
			break;
		}
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Rod~" : "& Rod~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& -";
        
		break;
	}
      
	/* Scrolls */
	case TV_SCROLL:
	{
		/* Color the object */
		modstr = scroll_adj[o_ptr->sval];
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& Scroll~ titled \"#\"" : "& Scroll~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = (flavor ? "& ? \"#\"" : "& ?");
        
		break;
	}
      
	/* Potions */
	case TV_POTION:
	{
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Potion~" : "& Potion~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& !";
        
		break;
	}
      
	/* Food */
	case TV_FOOD:
	{
		/* Ordinary food is "boring" */
		if (o_ptr->sval >= SV_FOOD_MIN_FOOD) break;
        
		/* Color the object */
		modstr = flavor_text + flavor_info[k_ptr->flavor].text;
		if (aware) append_name = TRUE;
		basenm = (flavor ? "& # Mushroom~" : "& Mushroom~");
        
		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& ,";
        
		break;
	}
      
	/* Magic Books */
	case TV_MAGIC_BOOK:
	{
		modstr = basenm;
		basenm = "& Book~ of Magic Spells #";

		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& ? #";
        
		break;
	}
      
	/* Prayer Books */
	case TV_PRAYER_BOOK:
	{
		modstr = basenm;
		basenm = "& Holy Book~ of Prayers #";

		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& ? #";
        
		break;
	}
      
	/* Druid Books */
	case TV_DRUID_BOOK:
	{
		modstr = basenm;
		basenm = "& Stone~ of Nature Lore #";

		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& ? #";
        
		break;
	}
      
	/* Necromantic Books */
	case TV_NECRO_BOOK:
	{
		modstr = basenm;
		basenm = "& Tome~ of Necromancy #";

		/* Get the short description */
		if ((mode == 4) && (aware)) basenm = "& ? #";
        
		break;
	}
      
	/* Hack -- Gold/Gems */
	case TV_GOLD:
	{
		strcpy(buf, basenm);
		return;
	}

	/* Hack -- Default -- Used in the "inventory" routine */
	default:
	{
		strcpy(buf, "(nothing)");
		return;
	}
	}


	/* Start dumping the result */
	t = b = buf;

	/* The object "expects" a "number" */
	if (basenm[0] == '&')
	{
		/* Skip the ampersand (and space) */
		s = basenm + 2;

		/* No prefix */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- None left */
		else if (o_ptr->number <= 0)
		{
			object_desc_str_macro(t, "no more ");
		}

		/* Extract the number */
		else if (o_ptr->number > 1)
		{
			object_desc_num_macro(t, o_ptr->number);
			object_desc_chr_macro(t, ' ');
		}
      
		/* Hack -- The only one of its kind (known real artifacts, or any fake 
		 * artifact). */
		else if ((artifact_p(o_ptr) && ((known) || ((easy_know) && (aware)))) || 
			 fake_art)
		{
			object_desc_str_macro(t, "The ");
		}

		/* A single one, and next character will be a vowel */
		else if ((*s == '#') ? is_a_vowel(modstr[0]) : is_a_vowel(*s))
		{
			if (mode != 4) object_desc_str_macro(t, "an ");
		}
      
		/* A single one, and next character will be a non-vowel */
		else
		{
			if (mode != 4) object_desc_str_macro(t, "a ");
		}
	}
  
	/* Hack -- objects that "never" take an article */
	else
	{
		/* No ampersand */
		s = basenm;

		/* No pref */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- all gone */
		else if (o_ptr->number <= 0)
		{
			object_desc_str_macro(t, "no more ");
		}

		/* Prefix a number if required */
		else if (o_ptr->number > 1)
		{
			object_desc_num_macro(t, o_ptr->number);
			object_desc_chr_macro(t, ' ');
		}
      
		/* Hack -- The only one of its kind (known real artifacts, or any fake 
		 * artifact). */
		else if ((artifact_p(o_ptr) && ((known) || ((easy_know) && (aware)))) || 
			 fake_art)
		{
			object_desc_str_macro(t, "The ");
		}

		/* Hack -- A single item, so no prefix needed */
		else
		{
			/* Nothing */
		}
	}

	/* Non-artifact perfectly balanced throwing weapons are indicated. */
	if ((f1 & (TR1_PERFECT_BALANCE)) && (f1 & (TR1_THROWING)) &&
	    (known) && (!o_ptr->name1))
	{
		object_desc_str_macro(t, "Well-balanced ");
	}

	/* Paranoia XXX XXX XXX */
	/* while (*s == '~') s++; */

	/* Copy the string */
	for (; *s; s++)
	{
		/* Pluralizer */
		if (*s == '~')
		{
			/* Add a plural if needed */
			if ((o_ptr->number != 1) && !(known && artifact_p(o_ptr)))
			{
				char k = t[-1];

				/* Hack -- "Cutlass-es" and "Torch-es" */
				if ((k == 's') || (k == 'h')) *t++ = 'e';

				/* Add an 's' */
				*t++ = 's';
			}
		}

		/* Modifier */
		else if (*s == '#')
		{
			/* Append the modifier */
			object_desc_str_macro(t, modstr);
		}

		/* Normal */
		else
		{
			/* Copy */
			*t++ = *s;
		}
	}
  
  
	/* Append the "kind name" to the "base name" */
	if (append_name)
	{
		if (mode != 4) object_desc_str_macro(t, " of ");
		else object_desc_str_macro(t, " ");
		object_desc_str_macro(t, (k_name + k_ptr->name));
	}
  
  
	/* Hack -- Append "Artifact" or "Special" names. */
	if ((known) || ((easy_know) && (aware)))
	{
		/* Grab any artifact name */
		if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			object_desc_chr_macro(t, ' ');
			object_desc_str_macro(t, (a_name + a_ptr->name));
		}

		/* Grab any ego-item name */
		else if (o_ptr->name2)
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
          
			object_desc_chr_macro(t, ' ');
			object_desc_str_macro(t, (e_name + e_ptr->name));
          
			/* Hack - Now we know about the ego-item type */
			e_info[o_ptr->name2].everseen = TRUE;
		}
	}
  
	/* Append a 'fake' artifact name, if any, as if it were a real artifact 
	 * name. */
	if (fake_art)
	{
		/* Find the '#'. Everything after it is the fake artifact name. */
		cptr str = strchr(quark_str(o_ptr->note), '#');
      
		/* Add the false name. */
		object_desc_chr_macro(t, ' ');
		object_desc_str_macro(t, (&str[1]));
	}
  
	/* Additional details, step 1. */
	if (mode > 0)
	{
		/* Hack -- Chests must be described in detail */
		if (o_ptr->tval == TV_CHEST)
		{
			cptr tail = "";

			/* Not searched yet */
			if (!known)
			{
				/* Nothing */
			}

			/* May be "empty" */
			else if (!o_ptr->pval)
			{
				tail = " (empty)";
			}

			/* May be "disarmed" */
			else if (o_ptr->pval < 0)
			{
				if (chest_traps[0 - o_ptr->pval])
				{
					tail = " (disarmed)";
				}
				else
				{
					tail = " (unlocked)";
				}
			}

			/* Describe the traps, if any */
			else
			{
				/* Describe the traps */
				switch (chest_traps[o_ptr->pval])
				{
				case 0:
				{
					tail = " (Locked)";
					break;
				}
				case CHEST_LOSE_STR:
				case CHEST_LOSE_CON:
				{
					tail = " (Poison Needle)";
					break;
				}
				case CHEST_POISON:
				case CHEST_PARALYZE:
				{
					tail = " (Gas Trap)";
					break;
				}
				case CHEST_SCATTER:
				{
					tail = " (A Strange Rune)";
					break;
				}
				case CHEST_EXPLODE:
				{
					tail = " (Explosion Device)";
					break;
				}
				case CHEST_SUMMON:
				case CHEST_E_SUMMON:
				case CHEST_H_SUMMON:
				case CHEST_BIRD_STORM:
				{
					tail = " (Summoning Runes)";
					break;
				}
				case CHEST_RUNES_OF_EVIL:
				{
					tail = " (Gleaming Black Runes)";
					break;
				}
				default:
				{
					tail = " (Multiple Traps)";
					break;
				}
				}
			}

			/* Append the tail */
			object_desc_str_macro(t, tail);
		}


		/* Display the item like a weapon */
		if (f3 & (TR3_SHOW_MODS)) show_weapon = TRUE;

		/* Display the item like a weapon */
		if (o_ptr->to_h && o_ptr->to_d) show_weapon = TRUE;

		/* Display the item like armour */
		if (o_ptr->ac) show_armour = TRUE;


		/* Dump base weapon info */
		switch (o_ptr->tval)
		{
			/* Missiles */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		{
			/* Fall through */
		}

		/* Weapons */
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			/* Append a "damage" string */
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
			object_desc_num_macro(t, o_ptr->dd);
			object_desc_chr_macro(t, 'd');
			object_desc_num_macro(t, o_ptr->ds);
			object_desc_chr_macro(t, p2);

			/* All done */
			break;
		}

		/* Bows */
		case TV_BOW:
		{
			/* Extract the "base power", using the weapon's damage dice.  */
			power = (o_ptr->dd);

			/* Modify the power, if the weapon is known. */
			if (known)
			{
				if (f1 & (TR1_MIGHT1)) power += 1;
				if (f1 & (TR1_MIGHT2)) power += 2;
			}

			/* Append a "power" string */
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
			object_desc_chr_macro(t, 'x');
			object_desc_num_macro(t, power);
			object_desc_chr_macro(t, p2);

			/* All done */
			break;
		}
		}


		/* Add the weapon bonuses */
		if (known)
		{
			/* Show the tohit/todam on request */
			if (show_weapon)
			{
				object_desc_chr_macro(t, ' ');
				object_desc_chr_macro(t, p1);
				object_desc_int_macro(t, o_ptr->to_h);
				object_desc_chr_macro(t, ',');
				object_desc_int_macro(t, o_ptr->to_d);
				object_desc_chr_macro(t, p2);
			}

			/* Show the tohit if needed */
			else if (o_ptr->to_h)
			{
				object_desc_chr_macro(t, ' ');
				object_desc_chr_macro(t, p1);
				object_desc_int_macro(t, o_ptr->to_h);
				object_desc_chr_macro(t, p2);
			}

			/* Show the todam if needed */
			else if (o_ptr->to_d)
			{
				object_desc_chr_macro(t, ' ');
				object_desc_chr_macro(t, p1);
				object_desc_int_macro(t, o_ptr->to_d);
				object_desc_chr_macro(t, p2);
			}
		}


		/* Add the armor bonuses */
		if (known)
		{
			/* Show the armor class info */
			if (show_armour)
			{
				/* Hack - Special case to make it clear what wearing a
				 * shield on the back does to its protective value. -LM-
				 */
				if ((p_ptr->shield_on_back) && (o_ptr == &inventory[INVEN_ARM]))
				{
					object_desc_chr_macro(t, ' ');
					object_desc_chr_macro(t, b1);
					object_desc_num_macro(t, o_ptr->ac / 3);
					object_desc_chr_macro(t, p1);
					object_desc_num_macro(t, o_ptr->ac);
					object_desc_chr_macro(t, p2);
					object_desc_chr_macro(t, ',');
					object_desc_int_macro(t, o_ptr->to_a / 2);
					object_desc_chr_macro(t, p1);
					object_desc_int_macro(t, o_ptr->to_a);
					object_desc_chr_macro(t, p2);
					object_desc_chr_macro(t, b2);
				}
				else
				{
					object_desc_chr_macro(t, ' ');
					object_desc_chr_macro(t, b1);
					object_desc_num_macro(t, o_ptr->ac);
					object_desc_chr_macro(t, ',');
					object_desc_int_macro(t, o_ptr->to_a);
					object_desc_chr_macro(t, b2);
				}
			}

			/* No base armor, but does increase armor */
			else if (o_ptr->to_a)
			{
				object_desc_chr_macro(t, ' ');
				object_desc_chr_macro(t, b1);
				object_desc_int_macro(t, o_ptr->to_a);
				object_desc_chr_macro(t, b2);
			}
		}

		/* Hack -- always show base armor */
		else if (show_armour)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, b1);
			object_desc_num_macro(t, o_ptr->ac);
			object_desc_chr_macro(t, b2);
		}
	}

	/* Additional details, step 2 */
	if (mode > 1)
	{
                /* Hack -- Rods/Wands/Staves may be attuned. */
		if (get_attune(o_ptr)) object_desc_str_macro(t, " [attuned]");

		/* Hack -- Wands and Staffs have charges. */
		if (known && ((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND)))
		{
			/* Dump " (N charges)" */
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
          
			/* Clear explanation for staffs. */
			if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1)) 
			{
				object_desc_num_macro(t, o_ptr->number);
				object_desc_str_macro(t, "x ");
			}
			object_desc_num_macro(t, o_ptr->pval);
			object_desc_str_macro(t, (mode == 4 ? "ch" : " charge"));
			if (o_ptr->pval != 1)
			{
				object_desc_chr_macro(t, 's');
			}

			object_desc_chr_macro(t, p2);
		}
      
		/* Hack -- Rods have a "charging" indicator.   Now that stacks of rods 
		 * may be in any state of charge or discharge, this now includes a 
		 * number.
		 */
		else if (known && (o_ptr->tval == TV_ROD))
		{
			/* Hack -- Dump " (# charging)" if relevant */
			if (o_ptr->timeout)
			{
				/* Stacks of rods display an exact count of charging rods. */
				if (o_ptr->number > 1)
				{
					/* Paranoia. */
					if (k_ptr->pval == 0) k_ptr->pval = 1;

					/* Find out how many rods are charging, by dividing
					 * current timeout by each rod's maximum timeout.
					 * Ensure that any remainder is rounded up.  Display
					 * very discharged stacks as merely fully discharged.
					 */
					power = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
					if (power > o_ptr->number) power = o_ptr->number;

					/* Display prettily. */
					object_desc_str_macro(t, " (");
					object_desc_num_macro(t, power);
					object_desc_str_macro(t, 
							      (mode == 4 ? "chg)" : 
							       " charging)"));
				}
              
				/* "one Rod of Perception (1 charging)" would look tacky. */
				else
				{
					object_desc_str_macro(t, (mode == 4 ? "(chg)" : 
								  " (charging)"));
				}
			}
		}


		/* Hack -- Process Lanterns/Torches */
		else if ((o_ptr->tval == TV_LITE) && (!artifact_p(o_ptr)))
		{
			/* Hack -- Turns of light for normal lites */
			object_desc_str_macro(t, (mode == 4 ? "(" : " (with "));
			object_desc_num_macro(t, o_ptr->pval);
			object_desc_str_macro(t, (mode == 4 ? "trns)" : 
						  " turns of light)"));
		}
      
      
		/* Dump "pval" flags for wearable items */
		if (known && (f1 & (TR1_PVAL_MASK)))
		{
			cptr tail = "";
			cptr tail2 = "";

			/* Start the display */
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);

			/* Dump the "pval" itself */
			object_desc_int_macro(t, o_ptr->pval);

			/* Do not display the "pval" flags */
			if (f3 & (TR3_HIDE_TYPE))
			{
				/* Nothing */
			}
          
			/* Stealth */
			else if ((f1 & (TR1_STEALTH)) && (mode != 4))
			{
				/* Dump " to stealth" */
				tail = " to stealth";
			}
          
			/* Searching */
			else if ((f1 & (TR1_SEARCH)) && (mode != 4))
			{
				/* Dump " to searching" */
				tail = " to searching";
			}
          
			/* Infravision */
			else if ((f1 & (TR1_INFRA)) && (mode != 4))
			{
				/* Dump " to infravision" */
				tail = " to infravision";
			}

#if 0
          
			/* Tunneling */
			else if ((f1 & (TR1_TUNNEL)) && (mode != 4))
			{
				/* Dump " to digging" */
				tail = " to digging";
			}

#endif
          
			/* Speed */
			else if ((f1 & (TR1_SPEED)) && (mode != 4))
			{
				/* Dump " to speed" */
				tail = " to speed";
			}

			/* Add the descriptor */
			object_desc_str_macro(t, tail);
			object_desc_str_macro(t, tail2);

			/* Finish the display */
			object_desc_chr_macro(t, p2);
		}


		/* Indicate charging objects, but not rods. */
		if (known && o_ptr->timeout && o_ptr->tval != TV_ROD)
		{
			/* Hack -- Dump " (charging)" if relevant */
			object_desc_str_macro(t, (mode == 4 ? "(chg)" : " (charging)"));
		}
      
	}

	/* Extra details, step 3 */
	if (mode > 2)
	{

		/* Use standard inscription */
		if (o_ptr->note)
		{
			/* Don't include the fake artifact name in the inscription. */
			if (fake_art)
			{
				char *tmp;

				strcpy(insc_buf, quark_str(o_ptr->note));
				tmp = insc_buf;

				/* Advance until we hit the '#' marking the beginning of the
				 * fake artifact name. */
				for (; *tmp && (*tmp != '#'); tmp++);

				/* Mark what was the '#' as the new end of the string (and the
				 * inscription). */
				*tmp = '\0';

				/* Is there a portion of the inscription that was not dedicated
				 * to the fake artifact name? */
				if (strlen(insc_buf))
				{
					/* The remaining portion becomes the inscription. */
					u = insc_buf;
				}
				else
				{
					/* No remaining portion, so no inscription. */
					u = NULL;
				}
			}
			else /* No fake artifact inscription. */
			{
				u = quark_str(o_ptr->note);
			}
		}

		/* Use nothing */
		else
		{
			u = NULL;
		}


		/* Use special inscription, if any */
		if (o_ptr->feel)
		{
			v = feel_text[o_ptr->feel];
		}

		/* Use "cursed" if the item is known to be cursed */
		else if (cursed_p(o_ptr) && known)
		{
			v = "cursed";
		}

		/* Hack -- Use "empty" for empty wands/staffs */
		else if (!known && (o_ptr->ident & (IDENT_EMPTY)))
		{
			v = "empty";
		}

		/* Use "tried" if the object has been tested unsuccessfully */
		else if (!aware && object_tried_p(o_ptr))
		{
			v = "tried";
		}

		/* Use the discount, if any   No annoying inscription for homemade
		 * branded items.
		 */
		else if ((o_ptr->discount > 0) && (o_ptr->discount != 80))
		{
			char *q = discount_buf;
			object_desc_num_macro(q, o_ptr->discount);
			object_desc_str_macro(q, "% off");
			*q = '\0';
			v = discount_buf;
		}

		/* Nothing */
		else
		{
			v = NULL;
		}


		/* Inscription */
		if (u || v)
		{
			/* Begin the inscription */
			*t++ = ' ';
			*t++ = c1;

			/* Standard inscription */
			if (u)
			{
				/* Append the inscription */
				while ((t < b + (mode == 4 ? 46 : 78)) && *u) *t++ = *u++;
			}
          
			/* Special inscription too */
			if (u && v && (t < b + (mode == 4 ? 46 : 78)))
			{
				/* Separator */
				*t++ = ',';
				*t++ = ' ';
			}

			/* Special inscription */
			if (v)
			{
				/* Append the inscription */
				while ((t < b + (mode == 4 ? 46 : 78)) && *v) *t++ = *v++;
			}
          
			/* Terminate the inscription */
			*t++ = c2;
		}
	}
  
  
	/* Add squelch marker (maybe dodgy -NRM-) */
	if (!hide_squelchable && squelch_item_ok(o_ptr))
		object_desc_str_macro(t, " (squelch)");

	/* Truncate overly long descriptions. */
	if (t - buf > (mode == 4 ? 46 : 78))
	{
		/* Remove all excess characters. */
		do
		{
			*t-- = '\0';
		}
		while (t - buf > (mode == 4 ? 46 : 78));
      
		return;
	}

	/* Otherwise, standard termination.  */
	else
	{
		/* Terminate */
		*t = '\0';

		return;
	}

}


/*
 * Describe an item and pretend the item is fully known and has no flavor.
 */
void object_desc_spoil(char *buf, object_type *o_ptr, int pref, int mode)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;

	/* Make a backup */
	object_copy(i_ptr, o_ptr);

	/* HACK - Pretend the object is in a store inventory */
	i_ptr->ident |= IDENT_KNOWN;

	/* Describe */
	object_desc(buf, i_ptr, pref, mode);
}


/*
 * Hack -- describe an item currently in a store's inventory
 * This allows an item to *look* like the player is "aware" of it
 */
void object_desc_store(char *buf, object_type *o_ptr, int pref, int mode)
{
	/* Save the "flavor" */
	int hack_flavor = k_info[o_ptr->k_idx].flavor;
  
	/* Save the "aware" flag */
	bool hack_aware = k_info[o_ptr->k_idx].aware;

	/* Save the "known" flag */
	bool hack_known = (o_ptr->ident & (IDENT_KNOWN)) ? TRUE : FALSE;


	/* Clear the flavor */
	k_info[o_ptr->k_idx].flavor = FALSE;

	/* Set the "known" flag */
	o_ptr->ident |= (IDENT_KNOWN);

	/* Force "aware" for description */
	k_info[o_ptr->k_idx].aware = TRUE;


	/* Describe the object */
	object_desc(buf, o_ptr, pref, mode);


	/* Restore "flavor" value */
	k_info[o_ptr->k_idx].flavor = hack_flavor;

	/* Restore "aware" flag */
	k_info[o_ptr->k_idx].aware = hack_aware;

	/* Clear the known flag */
	if (!hack_known) o_ptr->ident &= ~(IDENT_KNOWN);
}


/*
 * Strip an "object name" into a buffer.  Lifted from wizard2.c.
 */
void strip_name(char *buf, int k_idx, bool easy_know)
{
	char *t;
  
	object_kind *k_ptr = &k_info[k_idx];

	cptr str = (k_name + k_ptr->name);
  
  
	/* If not aware, use flavor */
	if (!easy_know && !k_ptr->aware && k_ptr->flavor)
		str = flavor_text + flavor_info[k_ptr->flavor].text;

	/* Skip past leading characters */
	while ((*str == ' ') || (*str == '&')) str++;
  
	/* Copy useful chars */
	for (t = buf; *str; str++)
	{
		if (*str != '~') *t++ = *str;
	}
  
	/* Terminate the new name */
	*t = '\0';
}


/*
 * Convert an inventory index into a one character label
 * Note that the label does NOT distinguish inven/equip.
 */
char index_to_label(int i)
{
	/* Indexes for "inven" are easy */
	if (i < INVEN_WIELD) return (I2A(i));

	/* Indexes for quiver slots are numeric */
	if ((i >= INVEN_Q0) && (i <= INVEN_Q9)) return (I2A(A2I('n') + i - INVEN_Q0));

	/* Indexes for other "equip" are offset */
	return (I2A(i - INVEN_WIELD));
}


/*
 * Convert a label into the index of an item in the "inven"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_inven(int c)
{
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return (-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}


/*
 * Convert a label into the index of a item in the "equip"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_equip(int c)
{
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return (-1);


	/* Empty slots can never be chosen */
	if (!inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}



/*
 * Determine which equipment slot (if any) an item likes
 */
s16b wield_slot(object_type *o_ptr)
{
	/* Slot for equipment */
	switch (o_ptr->tval)
	{
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	{
		return (INVEN_WIELD);
	}

	case TV_BOW:
	{
		return (INVEN_BOW);
	}

	case TV_RING:
	{
		/* Use the right hand first */
		if (!inventory[INVEN_RIGHT].k_idx) return (INVEN_RIGHT);

		/* Use the left hand for swapping (by default) */
		return (INVEN_LEFT);
	}

	case TV_AMULET:
	{
		return (INVEN_NECK);
	}

	case TV_LITE:
	{
		return (INVEN_LITE);
	}

	case TV_DRAG_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	{
		return (INVEN_BODY);
	}

	case TV_CLOAK:
	{
		return (INVEN_OUTER);
	}

	case TV_SHIELD:
	{
		return (INVEN_ARM);
	}

	case TV_CROWN:
	case TV_HELM:
	{
		return (INVEN_HEAD);
	}

	case TV_GLOVES:
	{
		return (INVEN_HANDS);
	}

	case TV_BOOTS:
	{
		return (INVEN_FEET);
	}

	/* Ammo asks for first quiver slot. */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
	{
		return (INVEN_Q0);
	}
	}

	/* No slot available */
	return (-1);
}


/*
 * Return a string mentioning how a given item is carried
 */
cptr mention_use(int i)
{
	cptr p;

	/* Examine the location */
	switch (i)
	{
	case INVEN_WIELD: p = "Wielding"; break;
	case INVEN_BOW:	  p = "Shooting"; break;
	case INVEN_LEFT:  p = "On left hand"; break;
	case INVEN_RIGHT: p = "On right hand"; break;
	case INVEN_NECK:  p = "Around neck"; break;
	case INVEN_LITE:  p = "Light source"; break;
	case INVEN_BODY:  p = "On body"; break;
	case INVEN_OUTER: p = "About body"; break;
	case INVEN_ARM:
	{
		if (p_ptr->shield_on_back) p = "On back";
		else p = "On arm";
		break;
	}
	case INVEN_HEAD:  p = "On head"; break;
	case INVEN_HANDS: p = "On hands"; break;
	case INVEN_FEET:  p = "On feet"; break;
	case INVEN_Q0:	case INVEN_Q1:	case INVEN_Q2:	case INVEN_Q3:
	case INVEN_Q4:      case INVEN_Q5:  case INVEN_Q6:
	case INVEN_Q7:      case INVEN_Q8:  case INVEN_Q9:
	{
		p = "At ready"; break;
	}
	default:      p = "In pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "Just lifting";
		}
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "Just holding";
		}
	}

	/* Return the result */
	return (p);
}


/*
 * Return a string describing how a given item is being worn.
 * Currently, only used for items in the equipment, not inventory.
 */
cptr describe_use(int i)
{
	cptr p;

	switch (i)
	{
	case INVEN_WIELD: p = "attacking monsters with"; break;
	case INVEN_BOW:	  p = "shooting missiles with"; break;
	case INVEN_LEFT:  p = "wearing on your left hand"; break;
	case INVEN_RIGHT: p = "wearing on your right hand"; break;
	case INVEN_NECK:  p = "wearing around your neck"; break;
	case INVEN_LITE:  p = "using to light the way"; break;
	case INVEN_BODY:  p = "wearing on your body"; break;
	case INVEN_OUTER: p = "wearing on your back"; break;
	case INVEN_ARM:
	{
		if (p_ptr->shield_on_back) p = "carrying on your back";
		else p = "wearing on your arm";
		break;
	}
	case INVEN_HEAD:  p = "wearing on your head"; break;
	case INVEN_HANDS: p = "wearing on your hands"; break;
	case INVEN_FEET:  p = "wearing on your feet"; break;
	case INVEN_Q0:	case INVEN_Q1:	case INVEN_Q2:	case INVEN_Q3:
	case INVEN_Q4:      case INVEN_Q5:  case INVEN_Q6:
	case INVEN_Q7:      case INVEN_Q8:  case INVEN_Q9:
	{
		p = "have at the ready"; break;
	}
	default:      p = "carrying in your pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "just lifting";
		}
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "just holding";
		}
	}

	/* Return the result */
	return p;
}





/*
 * Check an item against the item tester info
 */
bool item_tester_okay(object_type *o_ptr)
{
	/* Hack -- show empty slots if requested. */
	if (item_tester_full) return (TRUE);

	/* Require an item */
	if (!o_ptr->k_idx) return (FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD) return (FALSE);

	/* Check the tval */
	if (item_tester_tval)
	{
		if (!(item_tester_tval == o_ptr->tval)) return (FALSE);
	}

	/* Check the hook */
	if (item_tester_hook)
	{
		if (!(*item_tester_hook)(o_ptr)) return (FALSE);
	}

	/* Assume okay */
	return (TRUE);
}




/*
 * Choice window "shadow" of the "show_inven()" function
 */
void display_inven(void)
{
	register int i, n, z = 0;

	object_type *o_ptr;

	byte attr = TERM_WHITE;

	char tmp_val[80];

	char o_name[120];


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Track */
		z = i + 1;
	}

	/* Display the pack */
	for (i = 0; i < z; i++)
	{
		/* Examine the item */
		o_ptr = &inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item "acceptable"? */
		if (item_tester_okay(o_ptr))
		{
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
      
		/* Obtain an item description */
		object_desc(o_name, o_ptr, TRUE, 4);
      
		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Acquire inventory color.  Apply spellbook hack. */
		attr = proc_list_color_hack(o_ptr);

		/* Display the entry itself */
		Term_putstr(3, i, n, attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, i, 255);

		/* Display the weight if needed */
		if (show_weights && o_ptr->weight)
		{
			int wgt = o_ptr->weight * o_ptr->number;
			if (use_metric) 
				sprintf(tmp_val, "%3d.%1d kg", 
					make_metric(wgt) / 10, make_metric(wgt) % 10);
			else sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			Term_putstr(71, i, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = z; i < Term->hgt; i++)
	{
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}



/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(void)
{
	register int i, n;
	object_type *o_ptr;
	byte attr = TERM_WHITE;

	char tmp_val[80];

	char o_name[120];


	/* Display the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		/* Examine the item */
		o_ptr = &inventory[i];

		/* Hack -- never show empty quiver slots. */
		if ((!o_ptr->k_idx) && (i >= INVEN_Q0) && (i <= INVEN_Q9))
		{
			/* Clear the line, skip to next slot */
			Term_erase(0, i - INVEN_WIELD, 255);
			continue;
		}

		/* Hack -- never show the "blank" slot. */
		if (i == INVEN_BLANK)
		{
			/* Clear the line, skip to next slot */
			Term_erase(0, i - INVEN_WIELD, 255);
			continue;
		}

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item "acceptable"? */
		if (item_tester_okay(o_ptr))
		{
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i - INVEN_WIELD, 3, TERM_WHITE, tmp_val);
      
		/* Obtain an item description */
		object_desc(o_name, o_ptr, TRUE, 4);
      
		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Acquire inventory color.  Apply spellbook hack. */
		attr = proc_list_color_hack(o_ptr);

		/* Display the entry itself */
		Term_putstr(3, i - INVEN_WIELD, n, attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, i - INVEN_WIELD, 255);

		/* Display the slot description (if needed) */
		if (show_labels)
		{
			Term_putstr(61, i - INVEN_WIELD, -1, TERM_WHITE, "<--");
			Term_putstr(65, i - INVEN_WIELD, -1, TERM_WHITE, mention_use(i));
		}

		/* Display the weight (if needed) */
		if (show_weights && o_ptr->weight)
		{
			int wgt = o_ptr->weight * o_ptr->number;
			int col = (show_labels ? 52 : 71);
          
			if (use_metric) 
				sprintf(tmp_val, "%3d.%1d kg", 
					make_metric(wgt) / 10, make_metric(wgt) % 10);
			else sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
          
			Term_putstr(col, i - INVEN_WIELD, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++)
	{
		/* Clear that line */
		Term_erase(0, i, 255);
	}
}



/*
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void)
{
	int i, j, k, l, z = 0;
	int col, len, lim, wgt_col;
	int tile_hgt;
  
	object_type *o_ptr;
  
	char o_name[120];

	char tmp_val[80];

	int out_index[24];
	byte out_color[24];
	char out_desc[24][80];

	/* Count number of missiles in the quiver slots. */
	int ammo_num = quiver_count();

	/* Default length */
	len = 79 - 50;
  
	/* Maximum space allowed for descriptions */
	lim = Term->wid - 3;
  
	/* Require space for weight (if needed) */
	if (show_weights) lim -= 6;
  
	/* Hack -- Description must fit on screen */
	if (lim > Term->wid - 1) lim = Term->wid - 1;
  
	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Track */
		z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++)
	{
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;
      
		/* Describe the object */
		object_desc(o_name, o_ptr, TRUE, 4);
      
		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the index */
		out_index[k] = i;

		/* Acquire inventory color.  Apply spellbook hack. */
		out_color[k] = proc_list_color_hack(o_ptr);

		/* Save the object description */
		strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}
  
	/* Find the column to start in */
	if (len > Term->wid - 4 ) col = 0;
	else if ((Term->wid - len) / 2 < 12) col = (Term->wid -len) / 2;
	else col = 12;
  
	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		if (o_ptr->ident & (IDENT_MENTAL))
		{
			c_put_str(TERM_L_BLUE, tmp_val, j+1, col);
		}
		else if ((object_known_p(o_ptr)) || (object_aware_p(o_ptr)))
		{
			put_str(tmp_val, j+1, col);
		}
		else
		{
			c_put_str(TERM_L_WHITE, tmp_val, j+1, col);
		}

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;      
          
			if (use_metric) 
				sprintf(tmp_val, "%3d.%1d kg", 
					make_metric(wgt) / 10, make_metric(wgt) % 10);
			else sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
          
			wgt_col = col + len + 1;
			if (wgt_col + 5 > Term->wid) wgt_col = Term->wid - 5;
			put_str(tmp_val, j + 1, wgt_col);
		}
	}
  
	/*
	 * Add notes about slots used by the quiver, if we have space, want
	 * to show all slots, and have items in the quiver.
	 */
	if ((p_ptr->pack_size_reduce) && (item_tester_full) &&
	    (j <= (INVEN_PACK - p_ptr->pack_size_reduce)))
	{
		/* Insert a blank dividing line, if we have the space. */
		if (j <= ((INVEN_PACK - 1) - p_ptr->pack_size_reduce))
		{
			j++;
			prt("", j, col ? col - 2 : col);
		}

		for (i = 1; i <= p_ptr->pack_size_reduce; i++)
		{
			/* Go to next line. */
			j++;
			prt("", j, col ? col - 2 : col);

			/* Determine index, print it out. */
			sprintf(tmp_val, "%c)", index_to_label(INVEN_PACK - i));
			put_str(tmp_val, j, col);

			/* Note amount of ammo */
			k = (ammo_num > 99) ? 99 : ammo_num;
          
			/* Hack -- use "(Ready Ammunition)" as a description. */
			c_put_str(TERM_BLUE, format("(Ready Ammunition) [%2d]", k), 
				  j, col + 3);

			/* Reduce ammo count */
			ammo_num -= k;
		}
	}
  
	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", ++j, col ? col - 2 : col);

	/* Hack - delete exact graphics rows */
	if (use_trptile || use_dbltile)
	{ 
		tile_hgt = (use_trptile ? 3 : 2);
		while ((j % tile_hgt) && (j <= SCREEN_ROWS)) 
			prt("", ++j, col ? col - 2 : col);
	}
}



/*
 * Display the equipment.
 */
void show_equip(void)
{
	int i, j, k, l;
	int col, len, lim, wgt_col;
	int tile_hgt;
  
	object_type *o_ptr;
  
	char tmp_val[80];

	char o_name[120];

	int out_index[24];
	byte out_color[24];
	char out_desc[24][80];


	/* Default length */
	len = 79 - 50;
  
	/* Maximum space allowed for descriptions */
	lim = Term->wid - 3;
  
	/* Require space for labels (if needed) */
	if (show_labels) lim -= (14 + 2);
  
	/* Require space for weight (if needed) */
	if (show_weights) lim -= 6;
  
	/* Hack -- Description must fit */
	if (lim > Term->wid) lim = Term->wid;
  
	/* Scan the equipment list */
	for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Hack -- never show empty quiver slots. */
		if ((!o_ptr->k_idx) && (i >= INVEN_Q0) && (i <= INVEN_Q9))
		{
			/* Skip to next slot */
			continue;
		}

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;
      
		/* Description */
		object_desc(o_name, o_ptr, TRUE, 4);
      
		/* Truncate the description */
		o_name[lim] = 0;

		/* Save the index */
		out_index[k] = i;

		/* Acquire inventory color */
		out_color[k] = proc_list_color_hack(o_ptr);

		/* Save the description */
		strcpy(out_desc[k], o_name);

		/* Extract the maximal length (see below) */
		l = strlen(out_desc[k]) + (2 + 3);

		/* Increase length for labels (if needed) */
		if (show_labels) l += (14 + 2);
      
		/* Increase length for weight (if needed) */
		if (show_weights) l += 6;
      
		/* Maintain the max-length */
		if (l > len) len = l;

		/* Advance the entry */
		k++;
	}
  
	/* Hack -- Find a column to start in */
	if (len > Term->wid - 4 ) col = 0;
	else if ((Term->wid - len) / 2 < 12) col = (Term->wid -len) / 2;
	else col = 12;
  
	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);


		/* Leave an empty line for the blank slot. */
		if (i == INVEN_BLANK) continue;


		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		if (o_ptr->ident & (IDENT_MENTAL))
		{
			c_put_str(TERM_L_BLUE, tmp_val, j+1, col);
		}
		else if ((!o_ptr) || (object_known_p(o_ptr)) || (object_aware_p(o_ptr)))
		{
			put_str(tmp_val, j+1, col);
		}
		else
		{
			c_put_str(TERM_L_WHITE, tmp_val, j+1, col);
		}

		/* Use labels */
		if (show_labels)
		{
			/* Mention the use */
			sprintf(tmp_val, "%-14s: ", mention_use(i));
			put_str(tmp_val, j+1, col + 3);

			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1, col + 3 + 14 + 2);
		}

		/* No labels */
		else
		{
			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1, col + 3);
		}

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
          
			if (use_metric) 
				sprintf(tmp_val, "%3d.%1d kg", 
					make_metric(wgt) / 10, make_metric(wgt) % 10);
			else sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
          
			wgt_col = col + len + 1;
			if (wgt_col + 5 > Term->wid) wgt_col = Term->wid - 5;
			put_str(tmp_val, j + 1, wgt_col);
		}
	}
  
	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", ++j, col ? col - 2 : col);

	/* Hack - delete exact graphics rows */
	if (use_trptile || use_dbltile)
	{ 
		tile_hgt = (use_trptile ? 3 : 2);
		while ((j % tile_hgt) && (j <= SCREEN_ROWS)) 
			prt("", ++j, col ? col - 2 : col);
	}
}




/*
 * Flip "inven" and "equip" in any sub-windows
 */
void toggle_inven_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < TERM_WIN_MAX; j++)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Flip inven to equip */
		if (op_ptr->window_flag[j] & (PW_INVEN))
		{
			/* Flip flags */
			op_ptr->window_flag[j] &= ~(PW_INVEN);
			op_ptr->window_flag[j] |= (PW_EQUIP);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);
		}

		/* Flip inven to equip */
		else if (op_ptr->window_flag[j] & (PW_EQUIP))
		{
			/* Flip flags */
			op_ptr->window_flag[j] &= ~(PW_EQUIP);
			op_ptr->window_flag[j] |= (PW_INVEN);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN);
		}
	}
}



/*
 * Verify the choice of an item.
 *
 * The item can be negative to mean "item on floor".
 */
bool verify_item(cptr prompt, int item)
{
	char o_name[120];
  
	char out_val[160];

	object_type *o_ptr;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Describe */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Prompt */
	sprintf(out_val, "%s %s? ", prompt, o_name);

	/* Query */
	return (get_check(out_val));
}


/*
 * Hack -- allow user to "prevent" certain choices
 *
 * The item can be negative to mean "item on floor".
 */
extern bool get_item_allow(int item)
{
	cptr s;
  
	object_type *o_ptr;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* No inscription */
	if (!o_ptr->note) return (TRUE);

	/* Find a '!' */
	s = strchr(quark_str(o_ptr->note), '!');

	/* Process preventions */
	while (s)
	{
		/* Check the "restriction" */
		if ((s[1] == p_ptr->command_cmd) || (s[1] == '*'))
		{
			/* Verify the choice */
			if (!verify_item("Really try", item)) return (FALSE);
		}

		/* Find another '!' */
		s = strchr(s + 1, '!');
	}

	/* Allow it */
	return (TRUE);
}



/*
 * Verify the "okayness" of a given item.
 *
 * The item can be negative to mean "item on floor".
 */
static bool get_item_okay(int item)
{
	object_type *o_ptr;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Verify the item */
	return (item_tester_okay(o_ptr));
}

/*
 * Find the "first" inventory object with the given "tag".
 *
 * A "tag" is a char "n" appearing as "@n" anywhere in the
 * inscription of an object.
 *
 * Also, the tag "@xn" will work as well, where "n" is a tag-char,
 * and "x" is the "current" p_ptr->command_cmd code.
 */
static int get_tag(int *cp, char tag)
{
	int i, start;
	cptr s;


	if (p_ptr->command_wrk == USE_EQUIP) start = INVEN_WIELD;
	else start = 0;


	/* Check every object */
	for (i = start; i < INVEN_TOTAL; ++i)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Skip items not of the required tval. */
		if ((item_tester_tval) && (o_ptr->tval != item_tester_tval)) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->note) continue;

		/* Find a '@' */
		s = strchr(quark_str(o_ptr->note), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Check the special tags */
			if ((s[1] == p_ptr->command_cmd) && (s[2] == tag))
			{
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}


/*
 * Get the indexes of objects at a given floor location. -TNB-
 *
 * Return the number of object indexes acquired.
 *
 * Never acquire more than "size" object indexes, and never return a
 * number bigger than "size", even if more floor objects exist.
 *
 *
 * Valid flags are any combination of the bits:
 *
 *   0x01 -- Verify item tester
 *   0x02 -- Marked items only
 */
bool scan_floor(int *items, int *item_num, int y, int x, int mode)
{
	int this_o_idx, next_o_idx;

	int num = 0;

	(*item_num) = 0;

	/* Sanity */
	if (!in_bounds(y, x)) return (FALSE);

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Squelched */
		if (squelch_hide_item(o_ptr)) continue;
      
		/* Item tester */
		if ((mode & 0x01) && !item_tester_okay(o_ptr)) continue;
      
		/* Marked */
		if ((mode & 0x02) && (!o_ptr->marked)) continue;
      
		/* Accept this item */
		items[num++] = this_o_idx;

		/* Only one */
		if (mode & 0x04) break;
      
		/* XXX Hack -- Enforce limit */
		if (num == MAX_FLOOR_STACK) break;
	}
  
	/* Number of items */
	(*item_num) = num;

	/* Result */
	return (num != 0);
}


/*
 * Display a list of the items on the floor at the given location. -TNB-
 */
void show_floor(const int *floor_list, int floor_num, bool gold)
{
	int i, j, k, l;
	int col, len, lim;
	int tile_hgt;
  
	object_type *o_ptr;
  
	char o_name[120];
  
	char tmp_val[80];
  
	int out_index[24];
	byte out_color[24];
	char out_desc[24][80];
  
	/* Default length */
	len = 79 - 50;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	if (show_weights) lim -= 9;
  
	/* Display the inventory */
	for (k = 0, i = 0; i < floor_num; i++)
	{
		o_ptr = &o_list[floor_list[i]];
      
		/* Optionally, show gold */
		if ((o_ptr->tval != TV_GOLD) || (!gold))
		{
			/* Is this item acceptable? */
			if (!item_tester_okay(o_ptr)) continue;
		}
      
		/* Describe the object */
		object_desc(o_name, o_ptr, TRUE, 4);
      
		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the index */
		out_index[k] = i;
      
		/* Get inventory color */
		out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];
      
		/* Save the object description */
		strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}
  
	/* Find the column to start in */
	if (len > Term->wid - 4 ) col = 0;
	else if ((Term->wid - len) / 2 < 12) col = (Term->wid -len) / 2;
	else col = 12;
  
	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		/* Get the index */
		i = floor_list[out_index[j]];

		/* Get the item */
		o_ptr = &o_list[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(j));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
			if (use_metric) 
				sprintf(tmp_val, "%3d.%1d kg", 
					make_metric(wgt) / 10, make_metric(wgt) % 10);
			else sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			put_str(tmp_val, j + 1, 71);
		}
	}
    
  
	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", ++j, col ? col - 2 : col);

	/* Hack - delete exact graphics rows */
	if (use_trptile || use_dbltile)
	{ 
		tile_hgt = (use_trptile ? 3 : 2);
		while ((j % tile_hgt) && (j <= SCREEN_ROWS)) 
			prt("", ++j, col ? col - 2 : col);
	}
}

/* Arrays for inventory, equipment and floor */
int i1, i2, e1, e2, f1, f2;
int inven_count = 0, equip_count = 0, floor_count = 0;
int equ[24], inv[24], flo[24];


/* 
 * Make the correct prompt for items, handle mouse buttons
 */
void item_prompt(int mode, cptr pmt)
{
	bool can_squelch = ((mode & (CAN_SQUELCH)) ? TRUE : FALSE);
	bool use_inven   = ((mode & (USE_INVEN))   ? TRUE : FALSE);
	bool use_equip   = ((mode & (USE_EQUIP))   ? TRUE : FALSE);
  
	char tmp_val[160];
	char out_val[160];

	/* Clear the line */
	prt("", 0, 0);
 
	/* Viewing inventory */
	if (p_ptr->command_wrk == (USE_INVEN))
	{
		/* Begin the prompt */
		sprintf(out_val, "Inven:");
      
		/* Indicate lack of inventory choices. */
		if (i1 > i2) 
		{
			sprintf(tmp_val, " (none),");
		}
      
		/* List choices. */
		else sprintf(tmp_val, " %c-%c,", index_to_label(i1), 
			     index_to_label(i2));
      
		/* Append choices. */
		strcat(out_val, tmp_val);
      
		/* Indicate ability to "view" */
		if (!p_ptr->command_see) 
		{
			strcat(out_val, " * to see,");
			add_button("*", '*');
		}
      
		/* Indicate that equipment items are available */
		if (use_equip) 
		{
			strcat(out_val, " / equip,");
			add_button("/", '/');
		}
      
		/* Indicate that floor items are available */
		if (f1 <= f2) 
		{
			strcat(out_val, " - floor, . floor top,");
			add_button("-", '-');
			add_button(".", '.');
		}
      
		/* Indicate that selecting all SQUELCHED items is an option */
		if (can_squelch)
		{ 
			strcat(out_val, " ! for squelched,");
			add_button("!", '!');
		}
	}
  
	/* Viewing equipment */
	else if (p_ptr->command_wrk == (USE_EQUIP))
	{
		/* Begin the prompt */
		sprintf(out_val, "Equip:");
      
		/* Indicate lack of equipment choices. */
		if (e1 > e2) 
		{
			sprintf(tmp_val, " (none),");
		}
      
		/* List choices. */
		else sprintf(tmp_val, " %c-%c,", index_to_label(e1), 
			     index_to_label(e2));
      
		/* Append choices. */
		strcat(out_val, tmp_val);
      
		/* Indicate ability to "view" */
		if (!p_ptr->command_see) 
		{
			strcat(out_val, " * to see,");
			add_button("*", '*');
		}
      
		/* Append */
		if (use_inven) 
		{
			strcat(out_val, " / inven,");
			add_button("/", '/');
		}
      
		/* Append */
		if (f1 <= f2) 
		{
			strcat(out_val, " - floor, . floor top,");
			add_button("-", '-');
			add_button(".", '.');
		}
      
		/* Indicate that selecting all SQUELCHED items is an option */
		if (can_squelch) 
		{
			strcat(out_val, " ! for squelched,");
			add_button("!", '!');
		}
	}
      
	/* Viewing floor */
	else if (p_ptr->command_wrk == (USE_FLOOR))
	{
		/* Begin the prompt */
		sprintf(out_val, "Floor:");
      
		/* Indicate lack of floor choices. */
		if (f1 > f2) 
			sprintf(tmp_val, " (none),");
      
		/* List choices. */
		else sprintf(tmp_val, " %c-%c,", I2A(f1-f1), I2A(f2-f1));
      
		/* Append */
		strcat(out_val, tmp_val);
      
		/* Indicate ability to "view" */
		if (!p_ptr->command_see) 
		{
			strcat(out_val, " * to see,");
			add_button("*", '*');
		}
      
		/* Append */
		if (use_inven)
		{
			strcat(out_val, " / inven,");
			add_button("/", '/');
		}
		else if (use_equip)
		{
			strcat(out_val, " / equip,");
			add_button("/", '/');
		}
      
		/* Indicate that selecting all SQUELCHED items is an option */
		if (can_squelch) 
		{
			strcat(out_val, " ! for squelched,");
			add_button("!", '!');
		}
	}

	/* Do the buttons */
	update_statusline();
  
	/* Finish the prompt */
	strcat(out_val, " ESC");
  
	/* Build the prompt */
	sprintf(tmp_val, "(%s) %s", out_val, pmt);
      
	/* Show the prompt */
	prt(tmp_val, 0, 0);
}
      



/* Item selection menus */

static char get_item_tag(menu_type *menu, int oid)
{
	const int *choice = menu->menu_data;
	int idx = choice[oid];

	if (p_ptr->command_wrk == USE_FLOOR)
		return I2A(oid);
	else
		return index_to_label(idx);
}

/*
 * Display an entry on the item menu
 */
void get_item_display(menu_type *menu, int oid, bool cursor, int row, 
                      int col, int width)
{
	const int *choice = menu->menu_data;
	int idx = choice[oid];
	char o_name[120];
  
	byte attr;

	object_type *o_ptr;

	/* Do we even have a menu? */
	if (!p_ptr->command_see) return;

	/* Get the object - hack to abort if invalid */
	if (p_ptr->command_wrk == (USE_INVEN))
	{
		o_ptr = &inventory[idx];
		if (!get_item_okay(idx)) return;
	}
	else if (p_ptr->command_wrk == (USE_EQUIP))
	{
		o_ptr = &inventory[idx];
		if (!get_item_okay(idx)) return;
	}
	else 
	{
		o_ptr = &o_list[idx];
		if (!get_item_okay(0 - idx)) return;
	}
  
	/* Set the colour */
	attr = tval_to_attr[o_ptr->tval & 0x7F];

	/* Get the object description */
	object_desc(o_name, o_ptr, TRUE, 4);
      
	/* Hack -- enforce max length */
	o_name[width - 3] = '\0';
      

	/* Print it */
	if (cursor)
		c_put_str(TERM_L_BLUE, format(">>",o_name), row, col);
  
	c_put_str(attr, format("%s",o_name), row, col + 2);
}

/*
 * Deal with events on the get_item menu
 */
bool get_item_action(char cmd, void *db, int oid)
{
	return TRUE;
}


/*
 * Display list items to choose from
 */
bool item_menu(int *cp, cptr pmt, int mode, bool *oops)
{
	menu_type menu;
	menu_iter menu_f = { 0, get_item_tag, 0, get_item_display, 
			     get_item_action };
	region area = { 0, 1, -1, -1 };
	event_type evt = { EVT_NONE, 0, 0, 0, 0 };
	int num_entries;
	bool done;
  
	int py = p_ptr->py;
	int px = p_ptr->px;
  
	int j, k = 0;
	bool item;
  
	bool refresh = FALSE;
  
	bool fall_through = FALSE;
  
	bool can_squelch = ((mode & (CAN_SQUELCH)) ? TRUE : FALSE);
  
	bool use_inven  = ((mode & (USE_INVEN)) ? TRUE : FALSE);
	bool use_equip  = ((mode & (USE_EQUIP)) ? TRUE : FALSE);
	bool use_floor  = ((mode & (USE_FLOOR)) ? TRUE : FALSE);
	bool use_quiver = FALSE;
  
	bool allow_equip = FALSE;
	bool allow_inven = FALSE;
	bool allow_floor = FALSE;
  
	bool toggle = FALSE;
  
	int floor_list[24];
	int floor_num;

	/* FIX LATER */
	int len = 70;
  
	/* Paranoia XXX XXX XXX */
	msg_print(NULL);
  
	/* Not done */
	done = FALSE;
  
	/* No item selected */
	item = FALSE;
  
	/* Mega-hack -- show lists */
	if (show_lists || small_screen) p_ptr->command_see = TRUE;      
  
	/* Full inventory */
	i1 = 0;
	i2 = INVEN_PACK - 1;
  
	/* Forbid inventory */
	if (!use_inven) i2 = -1;
  
	/* Restrict inventory indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;
  
	/* Accept inventory */
	if (i1 <= i2) 
	{
		allow_inven = TRUE;

		/* Record them */
		for (inven_count = 0, j = i1; j <= i2; j++)
			if (get_item_okay(j))
				inv[inven_count++] = j;
	}
  
	/* Full equipment */
	e1 = INVEN_WIELD;
	e2 = INVEN_TOTAL - 1;
  
	/* Forbid equipment */
	if (!use_equip) e2 = -1;
  
	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;
  
	/* Accept equipment */
	if (e1 <= e2) 
	{
		allow_equip = TRUE;
      
		/* Record them */
		for (equip_count = 0, j = e1; j <= e2; j++)
			if (get_item_okay(j))
				equ[equip_count++] = j;
	}

	/* Check for anything in the quiver */
	use_quiver = (allow_equip && (e2 > INVEN_SUBTOTAL));
  
	/* Count "okay" floor items */
	floor_num = 0;
  
	/* Restrict floor usage */
	if (mode & (USE_FLOOR))
	{
		/* Scan all objects in the grid */
		(void) scan_floor(floor_list, &floor_num, py, px, 0x01);
	}
  
	/* Full floor */
	f1 = 0;
	f2 = floor_num - 1;

	/* Forbid floor */
	if (!use_floor) f2 = -1;

	/* Restrict floor indexes */
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f1]))) f1++;
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f2]))) f2--;
  
	/* Accept floor */
	if (f1 <= f2) 
	{
		allow_floor = TRUE;
      
		/* Record them */
		for (floor_count = 0, j = f1; j <= f2; j++)
			if (get_item_okay(0 - floor_list[j]))
				flo[floor_count++] = floor_list[j];
	}
  
	/* Require at least one legal choice */
	if (!allow_inven && !allow_equip && !allow_floor)
	{
		/* Cancel p_ptr->command_see */
		p_ptr->command_see = FALSE;

		/* Report failure */
		*oops = TRUE;

		/* Done here */
		return FALSE;
	}

	/* Assume we'll be looking at something*/
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
      
	/* Hack -- Start on equipment if requested */
	if ((mode == (USE_EQUIP)) && use_equip)
	{
		p_ptr->command_wrk = (USE_EQUIP);
	}
  
	/* Hack -- Start on equipment if shooting or throwing */
	else if (((p_ptr->command_cmd == 'f') || (p_ptr->command_cmd == 'v'))
		 && use_quiver)
	{
		p_ptr->command_wrk = (USE_EQUIP);
	}
  
	/* Use inventory if allowed. */
	else if (allow_inven)
	{
		p_ptr->command_wrk = (USE_INVEN);
	}
  
	/* Use equipment if allowed */
	else if (allow_equip)
	{
		p_ptr->command_wrk = (USE_EQUIP);
	}

	/* Use floor if allowed */
	else if (allow_floor)
	{
		p_ptr->command_wrk = (USE_FLOOR);
	}
	/* Hack -- Use (empty) inventory if no other choices available. */
	else
	{
		p_ptr->command_wrk = (USE_INVEN);
	}


	/* Find the column to start in */
	if (len > Term->wid - 4 ) area.col = 0;
	else if ((Term->wid - len) / 2 < 12) area.col = (Term->wid - len) / 2;
	else area.col = 12;
  
	/* Save the screen */
	screen_save();
  
	/* Set up the menu */
	WIPE(&menu, menu);
	menu.cmd_keys = "\n\r";
  
	/* Set the prompt */
	item_prompt(mode, pmt);

	/* Choose an appropriate set of items to display */
	if (p_ptr->command_wrk == (USE_INVEN))
	{
		menu.menu_data = inv;
		num_entries = inven_count;
	}
	else if (p_ptr->command_wrk == (USE_EQUIP))
	{
		menu.menu_data = equ;
		num_entries = equip_count;
	}
	else if (p_ptr->command_wrk == (USE_FLOOR))
	{
		menu.menu_data = flo;
		num_entries = floor_count;
	}
	else return FALSE;
	if (!p_ptr->command_see) 
	{
		num_entries = 0;
	}
  
	/* Clear space */
	area.page_rows = num_entries + 1;
	area.width = len;
	menu.count = num_entries;
	menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);

	/* Play until item selected or refused */
	while (!done)
	{
		event_type ke0;
		int ni = 0;
		int ne = 0;
              
		/* Scan windows */
		for (j = 0; j < TERM_WIN_MAX; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (op_ptr->window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (op_ptr->window_flag[j] & (PW_EQUIP)) ne++;
		}
      
		/* Toggle if needed */
		if ((((p_ptr->command_wrk == (USE_EQUIP)) && ni && !ne) ||
		     ((p_ptr->command_wrk == (USE_INVEN)) && !ni && ne)) &&
		    (p_ptr->command_see))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		/* Update */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
      
		/* Redraw windows */
		window_stuff();
      
		/* Change the display if needed */
		if (refresh)
		{
			/* Hack - load the screen and re-save */
			screen_load();
			screen_save();
          
          
			/* Set the prompt */
			item_prompt(mode, pmt);
          
			/* Pick the right menu */
			if (p_ptr->command_wrk == (USE_INVEN))
			{
				menu.menu_data = inv;
				num_entries = inven_count;
			}
			else if (p_ptr->command_wrk == (USE_EQUIP))
			{
				menu.menu_data = equ;
				num_entries = equip_count;
			}
			else if (p_ptr->command_wrk == (USE_FLOOR))
			{
				menu.menu_data = flo;
				num_entries = floor_count;
			}
			else return FALSE;
          
			if (!p_ptr->command_see) 
			{
				num_entries = 0;
			}
          
			/* Clear space */
			area.page_rows = num_entries + 1;
			menu.count = num_entries;
			menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
          
			refresh = FALSE;
		}
      
		menu_refresh(&menu);
      
		evt = inkey_ex();

		/* Hack - check for arrow-like inscriptions */
		if (get_tag(&k, evt.key) && ((evt.key == '2') || (evt.key == '4') || 
					     (evt.key == '6') || (evt.key == '8')))
			;
		else
		{                             
			ke0 = run_event_loop(&(menu.target), 0, &evt);
			if (ke0.type != EVT_AGAIN) evt = ke0;
		}
		switch(evt.type) 
		{
		case EVT_KBRD:
		{
			break;
		}
          
		case ESCAPE:
		{
			done = TRUE;
			continue;
		}
          
		case EVT_SELECT:
		{
			int *tmp = (int *) menu.menu_data;
			if (p_ptr->command_wrk == (USE_FLOOR))
				k = 0 - tmp[evt.index];
			else
				k = tmp[evt.index];

			/* Paranoia */
			if (!get_item_okay(k))
				continue;
            
			(*cp) = k;
			done = TRUE;
			continue;
		}
          
		case EVT_MOVE:
		{
			continue;
		}
          
		case EVT_BACK:
		{
			done = TRUE;
		}
          
		default:
		{
			continue;
		}
		}
		switch (evt.key)
		{
		case '*':case ' ':
		{
			/* Hide the list */
			if (p_ptr->command_see)
			{
				/* Flip flag */
				p_ptr->command_see = FALSE;
			}
            
			/* Show the list */
			else
			{
				/* Flip flag */
				p_ptr->command_see = TRUE;
			}
			refresh = TRUE;
			break;
		}
		case '-':
		{
			if (!allow_floor)
			{
				bell("Cannot select floor!");
				break;
			}
            
			/*
			 * If we aren't examining the floor and there is only
			 * one item, we will select it if floor_query_flag
			 * is FALSE.
			 */
			/* Hack -- Auto-Select */
			if ((!floor_query_flag) && (floor_num == 1))
			{
				/* Fall through */
			}
			else
			{
				p_ptr->command_wrk = (USE_FLOOR);
				refresh = TRUE;
				break;
			}
		}
		case '.':
		{
			/*
			 * If we are allow to use the floor, select
			 * the top item. -BR-
			 */
			if (allow_floor)
			{
				int k;

				/* Special index */
				k = 0 - floor_list[0];
                
				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
			}
            
			break;
		}
		case '/':
		{
			/* Toggle to inventory */
			if (allow_inven && (p_ptr->command_wrk != (USE_INVEN)))
			{
				p_ptr->command_wrk = (USE_INVEN);
				refresh = TRUE;
			}
            
			/* Toggle to equipment */
			else if (allow_equip && (p_ptr->command_wrk != (USE_EQUIP)))
			{
				p_ptr->command_wrk = (USE_EQUIP);
				refresh = TRUE;
			}
            
			/* No toggle allowed */
			else
			{
				bell("Cannot switch item selector!");
			}
            
			break;
		}
		case '!':
		{
			/* Can we select all squelched items? */
			if (can_squelch)
			{
				(*cp) = ALL_SQUELCHED;
				done = TRUE;
				break;
			}
			else bell("No squelched items!");
            
			break;
		}
		case ESCAPE:
		{
			evt.type = EVT_ESCAPE;
			done = TRUE;
			break;
		}
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
		{
			/* Look up the tag */
			if (!get_tag(&k, evt.key))
			{
				/* We are asking for something from the quiver */
				if ((item_tester_tval == 0) || 
				    (item_tester_tval == TV_SHOT) || 
				    (item_tester_tval == TV_ARROW) || 
				    (item_tester_tval == TV_BOLT))
				{
					/* If allowed, look at equipment and fall through. */
					if (use_equip)
					{
						p_ptr->command_wrk = USE_EQUIP;

						/* Fall through. */
						fall_through = TRUE;
					}
				}

				/*
				 * If not asking for ammo, or not allowed to look at
				 * equipment, display error message.
				 */
				if (!fall_through)
				{
					bell("Illegal object choice (tag)!");
					break;
				}
			}
            
			/* Hack -- Validate the item */
			if ((!fall_through) && 
			    ((k < INVEN_WIELD) ? !allow_inven : !allow_equip))
			{
				bell("Illegal object choice (tag1)!");
				break;
			}

			/* Validate the item */
			if ((!fall_through) && (!get_item_okay(k)))
			{
				bell("Illegal object choice (tag2)!");
				break;
			}

			/* Accept that choice */
			if (!fall_through)
			{
				(*cp) = k;
				done = TRUE;
				break;
			}

			/* Done with fall_through */
			fall_through = FALSE;

		}

		default:
		{
			bool verify;
            
			/* Note verify */
			verify = (isupper(evt.key) ? TRUE : FALSE);
            
			/* Lowercase */
			evt.key = tolower(evt.key);
            
			/* Convert letter to inventory index */
			if (p_ptr->command_wrk == (USE_INVEN))
			{
				k = label_to_inven(evt.key);
				if (k < 0)
				{
					bell("Illegal object choice (inven)!");
					break;
				}
			}

			/* Convert letter to equipment index */
			else if (p_ptr->command_wrk == (USE_EQUIP))
			{
				k = label_to_equip(evt.key);
                    
				if (k < 0)
				{
					bell("Illegal object choice (equip)!");
					break;
				}
			}

			/* Convert letter to floor index */
			else
			{
				k = (islower(evt.key) ? A2I(evt.key) : -1);
                
				if (k < 0 || k >= floor_num)
				{

					bell("Illegal object choice (floor)!");
					break;
				}

				/* Special index */
				k = 0 - floor_list[k];
			}

			/* Validate the item */
			if (!get_item_okay(k))
			{
				bell("Illegal object choice (normal)!");
				break;
			}

			/* Verify the item */
			if (verify && !verify_item("Try", k))
			{
				done = TRUE;
				break;
			}
            
			/* Accept that choice */
			(*cp) = k;
			done = TRUE;
			break;
		}
		}
	}
  
	/* Kill buttons */
	kill_button('*');
	kill_button('/');
	kill_button('-');
	kill_button('.');
	kill_button('!');
	update_statusline();
  
	/* Load screen */
	screen_load();

	/* Clean up */
	if (show_choices)
	{
		/* Toggle again if needed */
		if (toggle) toggle_inven_equip();

		/* Update */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Window stuff */
		window_stuff();
	}
  
  
	return ((evt.type != EVT_ESCAPE) && (evt.type != EVT_BACK));
}

/*
 * Let the user select an item, save its "index"
 *
 * Return TRUE only if an acceptable item was chosen by the user.
 *
 * The selected item must satisfy the "item_tester_hook()" function,
 * if that hook is set, and the "item_tester_tval", if that value is set.
 *
 * All "item_tester" restrictions are cleared before this function returns.
 *
 * The user is allowed to choose acceptable items from the equipment,
 * inventory, or floor, respectively, if the proper flag was given,
 * and there are any acceptable items in that location.
 *
 * Any of these are displayed (even if no acceptable items are in that 
 * location) if the proper flag was given.
 *
 * If there are no acceptable items available anywhere, and "str" is
 * not NULL, then it will be used as the text of a warning message
 * before the function returns.
 *
 * Note that the user must press "-" to specify the item on the floor.  The
 * use of "capital" letters will "examine" an inventory, equipment, or floor
 * item, and prompt for its use.
 *
 * If a legal item is selected from the inventory, we save it in "cp"
 * directly (0 to 35), and return TRUE.
 *
 * If a legal item is selected from the floor, we save it in "cp" as
 * a negative (-1 to -511), and return TRUE.
 *
 * If no item is available, we do nothing to "cp", and we display a
 * warning message, using "str" if available, and return FALSE.
 *
 * If no item is selected, we do nothing to "cp", and return FALSE.
 *
 * If 'all squelched items' are selected we set cp to ALL_SQUELCHED and return
 * TRUE.
 *
 * Global "p_ptr->command_new" is used when viewing the inventory or equipment
 * to allow the user to enter a command while viewing those screens, and
 * also to induce "auto-enter" of stores, and other such stuff.
 *
 * Global "p_ptr->command_see" may be set before calling this function to start
 * out in "browse" mode.  It is cleared before this function returns.
 *
 * Global "p_ptr->command_wrk" is used to choose between equip/inven/floor
 * listings.  It is equal to USE_INVEN or USE_EQUIP or USE_FLOOR, except
 * when this function is first called, when it is equal to zero, which will
 * cause it to be set to USE_INVEN.
 *
 * We always erase the prompt when we are done, leaving a blank line,
 * or a warning message, if appropriate, if no items are available.
 *
 * Note that only "acceptable" floor objects get indexes, so between two
 * commands, the indexes of floor objects may change.  XXX XXX XXX
 *
 * This function has been revised using code from Tim Baker's Easy Patch 1.2
 *
 * This function has been largely rewritten for FAangband 0.3.2 using 
 * Pete Mack's menu code. 
 */
bool get_item(int *cp, cptr pmt, cptr str, int mode)
{
	bool done, item;
  
	bool oops = FALSE;
  
	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* Verify the item */
		if (get_item_okay(*cp))
		{
			/* Forget the item_tester_tval restriction */
			item_tester_tval = 0;
          
			/* Forget the item_tester_hook restriction */
			item_tester_hook = NULL;
          
			/* Success */
			return (TRUE);
		}
      
		/* Invalid repeat - reset it */
		else repeat_clear();
	}
  
	/* Paranoia XXX XXX XXX */
	msg_print(NULL);
  
  
	/* Not done */
	done = FALSE;
  
	/* No item selected */
	item = FALSE;
	*cp = 0;
 
	/* Go to menu */  
	item = item_menu(cp, pmt, mode, &oops);

	/* Check validity */
	if (item)
		if (!get_item_allow(*cp)) 
		{
			item = FALSE;
			msg_print(NULL);
		}
  
	/* Hack -- Cancel "display" */
	p_ptr->command_see = FALSE;
  
	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;
  
	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;
  
	/* Clear the prompt line */
	prt("", 0, 0);

	/* Paranoia */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Window stuff */
	window_stuff();
  
	/* Warning if needed */
	if (oops && str) msg_print(str);
  
	/* Save item if available */
	if (item) repeat_push(*cp);
  
	/* Result */
	return (item);
}


/*
 * Link to various object coloring functions from info.c. -LM-
 */
cptr object_adj(int tval, int sval)
{
	object_kind *k_ptr = &k_info[lookup_kind(tval, sval)];

	switch (tval)
	{
	case TV_AMULET: case TV_RING: case TV_STAFF: case TV_WAND: 
	case TV_ROD: case TV_POTION: 
		return flavor_text + flavor_info[k_ptr->flavor].text;
	case TV_SCROLL: 
		return scroll_adj[sval];
	default: return (NULL);
	}
}



/* Set Item Code */

/*
 * Determin if a given Set of artifacts is being used by the player.
 */
extern bool check_set(byte s_idx)
{
	byte count = 0;
	byte i;
	set_type *s_ptr = &s_info[s_idx];;

	for (i=INVEN_WIELD;i<=INVEN_FEET;i++)
	{
		object_type *o_ptr=&inventory[i];
		if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];
			if (a_ptr->set_no==s_idx)
			{
				count++;
			}
		}
	}

	return (count>=s_ptr->no_of_items);
}

/*
 * Apply bonuses for complete artifact sets.
 */
extern void apply_set(int s_idx)
{
	set_type *s_ptr = &s_info[s_idx];
  
	bool bonus_applied = FALSE;
  
        byte i, j;

	for (i = INVEN_WIELD;i <= INVEN_FEET;i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Is it an artifact? */
		if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			/* Is it in the correct set? */
			if (a_ptr->set_no==s_idx)
			{

				/* Loop through set elements */
				for (j=0;j<(s_ptr->no_of_items);j++)
				{

					set_element *se_ptr = &s_ptr->set_items[j];

					/* Correct Element? */
					if (se_ptr->a_idx == o_ptr->name1)
					{

						/* Bonus already applied? */
						if (!(a_ptr->set_bonus))
						{
							a_ptr->flags1 |= se_ptr->flags1;
							a_ptr->flags2 |= se_ptr->flags2;
							a_ptr->flags3 |= se_ptr->flags3;
							o_ptr->pval += se_ptr->pval;
							a_ptr->set_bonus = TRUE;
							bonus_applied = TRUE;
						}
					}
				}
			}
		}
	}

	/* Notify */
	if (bonus_applied) msg_print("Item set completed!");
}

/*
 * Remove bonuses for no-longer-complete artifact sets.
 */
extern void remove_set(int s_idx)
{
	set_type *s_ptr = &s_info[s_idx];
  
	bool bonus_removed = FALSE;
  
        byte i, j;
  
	for (i=INVEN_WIELD;i<=INVEN_FEET;i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Is it an artifact? */
		if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			/* Is it in the correct set? */
			if (a_ptr->set_no==s_idx)
			{

				/* Loop through set elements */
				for (j=0;j<(s_ptr->no_of_items);j++)
				{

					set_element *se_ptr = &s_ptr->set_items[j];

					/* Correct Element? */
					if (se_ptr->a_idx == o_ptr->name1)
					{

						/* Is the bonus really there? */
						if (a_ptr->set_bonus)
						{
							a_ptr->flags1 &= ~se_ptr->flags1;
							a_ptr->flags2 &= ~se_ptr->flags2;
							a_ptr->flags3 &= ~se_ptr->flags3;
							o_ptr->pval = a_ptr->pval;
							a_ptr->set_bonus = FALSE;
							bonus_removed = TRUE;
						}
					}
				}
			}
		}
	}

	/* Notify */
	if (bonus_removed) msg_print("Item set no longer completed.");
}
