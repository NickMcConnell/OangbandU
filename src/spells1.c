/* File: spells1.c */

/* Polymorph, teleport monster, teleport player, colors and ascii 
 * graphics for spells, hurt and kill player, melting/burning/freezing/
 * electrocuting items, effects of these attacks on the player and his 
 * stuff, spells that increase, decrease, and restore stats, disenchant, 
 * nexus, destroy and create terrain features, light+dark, destroy ob-
 * jects.  handlers for beam/bolt/ball spells that do damage to a monster, 
 * other player combat spells, special effects of monster and player 
 * breaths and spells to the player, monsters, and dungeon terrain.  The 
 * projection code.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"



/*
 * Attempts a saving throw with a difficulty of "roll".
 *
 * Returns true on successful save.
 */
bool check_save(int roll)
{
	return (rand_int(roll) < p_ptr->skill_sav);
}

/*
 * Return another race for a monster to polymorph into.
 *
 * Perform a modified version of "get_mon_num()", with exact minimum and
 * maximum depths and preferred monster types.
 *
 * Note that this function modifies final monster probabilities, and so we
 * must be careful not to call "get_mon_num_quick()" until "get_mon_num()"
 * has been called at least once.
 */
s16b poly_r_idx(int base_idx)
{
	monster_race *r_ptr = &r_info[base_idx];
  
	alloc_entry *table = alloc_race_table;
  
	int i, min_lev, max_lev, r_idx;
	long value;
  
	/* Source monster's level and symbol */
	int r_lev = r_ptr->level;
	char d_char = r_ptr->d_char;
  
	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (base_idx);
  
	/* Allowable level of new monster */
	min_lev = (MAX(	       1, r_lev - 1 - r_lev / 5));
	max_lev = (MIN(MAX_DEPTH, r_lev + 1 + r_lev / 5));

  
  
	/* Reset sum of final monster probabilities. */
	alloc_race_total = 0L;
  
	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Assume no probability */
		table[i].prob3 = 0;
      
		/* Ignore illegal monsters - only those that don't get generated. */
		if (!table[i].prob1) continue;
      
		/* Not below the minimum base depth */
		if (table[i].level < min_lev) continue;
      
		/* Not above the maximum base depth */
		if (table[i].level > max_lev) continue;
      
		/* Get the monster index */
		r_idx = table[i].index;
      
		/* We're polymorphing -- we don't want the same monster */
		if (r_idx == base_idx) continue;
      
		/* Get the actual race */
		r_ptr = &r_info[r_idx];
      
		/* Hack -- No uniques */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;
      
		/* Forced-depth monsters only appear at their level. */
		if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && 
		    (r_ptr->level != p_ptr->depth)) continue;
      
		/* Accept */
		table[i].prob3 = table[i].prob2;
      
		/* Bias against monsters far from inital monster's depth */
		if (table[i].level < (min_lev + r_lev) / 2) table[i].prob3 /= 4;
		if (table[i].level > (max_lev + r_lev) / 2) table[i].prob3 /= 4;
      
		/* Bias against monsters not of the same symbol */
		if (r_ptr->d_char != d_char) table[i].prob3 /= 4;
      
		/* Sum up probabilities */
		alloc_race_total += table[i].prob3;
	}
  
	/* No legal monsters */
	if (alloc_race_total == 0)
	{
		return (base_idx);
	}
  
  
	/* Pick a monster */
	value = rand_int(alloc_race_total);
  
	/* Find the monster */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;
      
		/* Decrement */
		value = value - table[i].prob3;
	}
  
	/* Result */
	return (table[i].index);
}



/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
void teleport_away(int m_idx, int dis)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
	int ny, nx, oy, ox, d, i, min;
  
	bool look = TRUE;
  
	/* Paranoia */
	if (!m_ptr->r_idx) return;
  
	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;
  
	/* Minimum distance */
	min = dis / 2;
  
	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;
      
		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}
	  
			/* Ignore illegal locations */
			if (!in_bounds_fully(ny, nx)) continue;
	  
			/* Require a grid that the monster can (safely) exist in. */
			if (!cave_exist_mon(r_ptr, ny, nx, FALSE)) continue;
	  
			/* This grid looks good */
			look = FALSE;
	  
			/* Stop looking */
			break;
		}
      
		/* Increase the maximum distance */
		dis = dis * 2;
      
		/* Decrease the minimum distance */
		min = min / 2;
	}
  
	/* Sound */
	sound(SOUND_TPOTHER);
  
	/* Swap the monsters */
	monster_swap(oy, ox, ny, nx);
  
	/* Clear the cave_temp flag (the "project()" code may have set it). */
	cave_info[ny][nx] &= ~(CAVE_TEMP);
}


/*
 * Thrust the player or a monster away from the source of a projection.   
 * Used for GF_FORCE only (GF_STORM and GF_GRAVITY blink the player in 
 * a random direction).  -LM-
 *
 * Monsters and players can be pushed past monsters or players weaker than 
 * they are.
 */
static void thrust_away(int who, int t_y, int t_x, int grids_away)
{
	int y, x, yy, xx;
	int i, d, first_d;
	int angle;
  
	int c_y, c_x;
  
	/* Assume a default death */
	cptr note_dies = " dies.";
  
	/*** Find a suitable endpoint for testing. ***/
  
	/* Get location of caster (assumes index of caster is not zero) */
	if (who > 0)
	{
		c_y = m_list[who].fy;
		c_x = m_list[who].fx;
	}
	else
	{
		c_y = p_ptr->py;
		c_x = p_ptr->px;
	}
  
	/* Determine where target is in relation to caster. */
	y = t_y - c_y + 20;
	x = t_x - c_x + 20;

	/* Find the angle (/2) of the line from caster to target. */
	angle = get_angle_to_grid[y][x];
  
	/* Start at the target grid. */
	y = t_y;
	x = t_x;
  
	/* Up to the number of grids requested, force the target away from the 
	 * source of the projection, until it hits something it can't travel around.
	 */
	for (i = 0; i < grids_away; i++)
	{
		/* Randomize initial direction. */
		first_d = rand_int(8);
      
		/* Look around. */
		for (d = first_d; d < 8 + first_d; d++)
		{
			/* Reject angles more than 44 degrees from line. */
			if (d % 8 == 0)	/* 135 */
			{
				if ((angle > 157) || (angle < 114)) continue;
			}
			if (d % 8 == 1)	/* 45 */
			{
				if ((angle > 66) || (angle < 23)) continue;
			}
			if (d % 8 == 2)	/* 0 */
			{
				if ((angle > 21) && (angle < 159)) continue;
			}
			if (d % 8 == 3)	/* 90 */
			{
				if ((angle > 112) || (angle < 68)) continue;
			}
			if (d % 8 == 4)	/* 158 */
			{
				if ((angle > 179) || (angle < 136)) continue;
			}
			if (d % 8 == 5)	/* 113 */
			{
				if ((angle > 134) || (angle < 91)) continue;
			}
			if (d % 8 == 6)	/* 22 */
			{
				if ((angle > 44) || (angle < 1)) continue;
			}
			if (d % 8 == 7)	/* 67 */
			{
				if ((angle > 89) || (angle < 46)) continue;
			}
	  
			/* Extract adjacent location */
			yy = y + ddy_ddd[d % 8];
			xx = x + ddx_ddd[d % 8];
	  
			/* Cannot switch places with stronger monsters. */
			if (cave_m_idx[yy][xx] != 0)
			{
				/* A monster is trying to pass. */
				if (cave_m_idx[y][x] > 0)
				{
		  
					monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
		  
					if (cave_m_idx[yy][xx] > 0)
					{
                                                monster_type *n_ptr = &m_list[cave_m_idx[yy][xx]];

                                                /* Monsters cannot pass by stronger monsters. */
                                                if (r_info[n_ptr->r_idx].mexp > r_info[m_ptr->r_idx].mexp)
                                                        continue;
                                        }
                                        else
					{
						/* Monsters cannot pass by stronger characters. */
						if (p_ptr->lev * 2 > r_info[m_ptr->r_idx].level)
							continue;
					}
				}
	      
				/* The player is trying to pass. */
				if (cave_m_idx[y][x] < 0)
				{
					if (cave_m_idx[yy][xx] > 0)
					{
						monster_type *n_ptr = &m_list[cave_m_idx[yy][xx]];
		      
						/* Players cannot pass by stronger monsters. */
						if (r_info[n_ptr->r_idx].level > p_ptr->lev * 2)
							continue;
					}
				}
			}
	  
			/* Check for obstruction. */
			if (!cave_floor_bold(yy, xx))
			{
				/* Some features allow entrance, but not exit. */
				if (cave_passable_bold(yy, xx))
				{
					/* Travel down the path. */
					monster_swap(y, x, yy, xx);
		  
					/* Jump to new location. */
					y = yy;
					x = xx;
		  
					/* We can't travel any more. */
					i = grids_away;
		  
					/* Stop looking. */
					break;
				}
	      
				/* If there are walls everywhere, stop here. */
                                else if (d == (8 + first_d - 1))
                                {
                                        /* Message for player. */
                                        if (cave_m_idx[y][x] < 0) msg_print("You come to rest next to a wall.");
                                        i = grids_away;
                                }
                        }
			else
			{
				/* Travel down the path. */
				monster_swap(y, x, yy, xx);
	      
				/* Jump to new location. */
				y = yy;
				x = xx;
	      
				/* Stop looking at previous location. */
				break;
			}
		}
	}
  
        /* Some special messages or effects for player. */
        if (cave_m_idx[y][x] < 0)
        {
                if (cave_feat[y][x] == FEAT_TREE)
                        msg_print("You come to rest in some trees.");
                if (cave_feat[y][x] == FEAT_RUBBLE)
                        msg_print("You come to rest in some rubble.");
		if (cave_feat[y][x] == FEAT_WATER)
			msg_print("You come to rest in a pool of water.");
		if (cave_feat[y][x] == FEAT_LAVA)
		{
                        msg_print("You are thrown into molten lava!");
                        fire_dam(damroll(4, 100), "burnt up in molten lava");
                }
        }

        /* Some monsters don't like lava or water. */
        if (cave_m_idx[y][x] > 0)
        {
                monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
		if (cave_feat[y][x] == FEAT_WATER)
		{
			if ((strchr("uU", r_ptr->d_char)) || 
			    ((r_ptr->flags4 & (RF4_BRTH_FIRE)) && 
			     (!r_ptr->flags2 & (RF2_FLYING))))
			{
				note_dies = " is drowned.";

                                /* Hurt the monster.  No fear. */
                                mon_take_hit(cave_m_idx[y][x],
					     damroll(2, 18 + m_ptr->maxhp / 12), FALSE, note_dies);

                                /* XXX - If still alive, monster escapes. */
                                teleport_away(cave_m_idx[y][x], 3);
			}
		}
		if (cave_feat[y][x] == FEAT_LAVA)
		{
			if ((!r_ptr->flags3 & (RF3_IM_FIRE)) && 
			    (!r_ptr->flags2 & (RF2_FLYING)))
			{
				note_dies = " is burnt up.";

                                /* Hurt the monster.  No fear. */
                                mon_take_hit(cave_m_idx[y][x],
					     damroll(2, 18 + m_ptr->maxhp / 12), FALSE, note_dies);

                                /* XXX - If still alive, monster escapes. */
                                teleport_away(cave_m_idx[y][x], 3);
                        }
                }
        }

        /* Clear the cave_temp flag (the "project()" code may have set it). */
	cave_info[y][x] &= ~(CAVE_TEMP);
}

/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 *
 * If "unsafe" is TRUE, then feel free to slam the player into trees, 
 * dunk him in water, or burn him in lava.
 */
void teleport_player(int dis, bool safe)
{
	int py = p_ptr->py;
	int px = p_ptr->px;
  
	int d, i, min, y, x;
  
	bool look = TRUE;
  
  
	/* Initialize */
	y = py;
	x = px;
  
	/* Minimum distance */
	min = dis / 2;
	if (min < 1) min = 1;
  
	/* Check for specialty resistance on unsafe teleports */
	if ((safe == FALSE) && (check_ability(SP_PHASEWALK)) && (check_save(100)))
	{
		msg_print("Teleport Resistance!");
		return;
	}
  
	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;
      
		/* Try several locations  */
		for (i = 0; i < 20 + dis * 3; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(py, dis);
				x = rand_spread(px, dis);
				d = distance(py, px, y, x);
				if ((d >= min) && (d <= dis)) break;
			}
	  
			/* Ignore illegal locations */
			if (!in_bounds_fully(y, x)) continue;
	  
			if (safe)
			{
				/* Require unoccupied floor space */
				if (!cave_empty_bold(y, x)) continue;
	      
				/* No teleporting into vaults and such */
				if (cave_info[y][x] & (CAVE_ICKY)) continue;
			}
			else
			{
				/* Require any terrain capable of holding the player. */
				if (!cave_passable_bold(y, x)) continue;
	      
				/* Must be unoccupied. */
				if (cave_m_idx[y][x] != 0) continue;
	      
				/* Can teleport into vaults and such. */
			}
	  
			/* This grid looks good */
			look = FALSE;
	  
			/* Stop looking */
			break;
		}
      
		/* Increase the maximum distance */
		dis = dis * 2;
      
		/* Decrease the minimum distance */
		min = min / 2;
	}
  
	/* Sound */
	sound(SOUND_TELEPORT);
  
	/* Move player */
	monster_swap(py, px, y, x);
  
	/* Check for specialty speed boost on safe teleports */
	if ((safe == TRUE) && (check_ability(SP_PHASEWALK)))
	{
		/* Calculate Distance */
		d = distance(py, px, y, x);
      
		add_speed_boost(20 + d);
	}
  
        if (!safe)
        {
                /* The player may hit a tree, slam into rubble, or even land in lava. */
                if ((cave_feat[y][x] == FEAT_TREE) && (rand_int(2) == 0))
                {
                        msg_print("You hit a tree!");
                        take_hit(damroll(2, 8), "hurtled into a tree by gravity");
                        if (rand_int(3) != 0) set_stun(p_ptr->stun + damroll(2, 8));
                }
                else if ((cave_feat[y][x] == FEAT_RUBBLE) && (rand_int(2) == 0))
                {
                        msg_print("You slam into jagged rock!");
                        take_hit(damroll(2, 14), "slammed into rubble by gravity");
                        if (rand_int(3) == 0) set_stun(p_ptr->stun + damroll(2, 14));
                        if (rand_int(3) != 0) set_cut(p_ptr->cut + damroll(2, 14) * 2);
                }
                else if (cave_feat[y][x] == FEAT_LAVA)
                {
                        msg_print("You land in molten lava!");
                        fire_dam(damroll(4, 100), "thrown into molten lava");
                }
        }

	/* Clear the cave_temp flag (the "project()" code may have set it). */
	cave_info[y][x] &= ~(CAVE_TEMP);
  
	/* Handle stuff XXX XXX XXX */
	if (safe) handle_stuff();
}

/*
 * Teleport monster to a grid near the given location.  This function is
 * used in the monster spell "TELE_SELF_TO", to allow monsters both to
 * suddenly jump near the character, and to make them "dance" around the
 * character.
 *
 * Usually, monster will teleport to a grid that is not more than 4
 * squares away from the given location, and not adjacent to the given
 * location.  These restrictions are relaxed if necessary.
 *
 * This function allows teleporting into vaults.
 */
void teleport_towards(int oy, int ox, int ny, int nx)
{
	int y, x;
  
	int dist;
	int ctr = 0;
	int min = 2, max = 4;
  
	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, max);
			x = rand_spread(nx, max);
                        if (in_bounds_fully(y, x)) break;
                }

                /* Consider all unoccupied floor grids */
                if ((cave_feat[y][x] == FEAT_FLOOR) && (cave_m_idx[y][x] == 0))
                {
                        /* Calculate distance between target and current grid */
                        dist = distance(ny, nx, y, x);
	  
			/* Accept grids that are the right distance away. */
			if ((dist >= min) && (dist <= max)) break;
		}
      
		/* Occasionally relax the constraints */
		if (++ctr > 15)
		{
			ctr = 0;
	  
			max++;
			if (max > 5) min = 0;
		}
	}
  
	/* Sound (assumes monster is moving) */
	sound(SOUND_TPOTHER);
  
	/* Move monster */
	monster_swap(oy, ox, y, x);
  
	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}




/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(int ny, int nx, bool friendly)
{
	int py = p_ptr->py;
	int px = p_ptr->px;
  
	int y, x;
  
	int dis = 0, ctr = 0;
  
	/* Initialize */
	y = py;
        x = px;

        /* Check for specialty resistance on hostile teleports */
        if ((friendly == FALSE) && (check_ability(SP_PHASEWALK)) && (check_save(100)))
        {
                msg_print("Teleport Resistance!");
                return;
	}
  
	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds_fully(y, x)) break;
		}
      
		/* Accept "naked" floor grids */
		if (cave_naked_bold(y, x)) break;
      
		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}
  
	/* Check for specialty speed boost on friendly teleports */
	if ((friendly == TRUE) && (check_ability(SP_PHASEWALK)))
	{
		/* Calculate Distance */
		int d = distance(py, px, y, x);
      
		add_speed_boost(20 + d);
	}
  
	/* Sound */
	sound(SOUND_TELEPORT);
  
	/* Move player */
	monster_swap(py, px, y, x);
  
	/* Clear the cave_temp flag (the "project()" code may have set it). */
	cave_info[y][x] &= ~(CAVE_TEMP);
  
	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}

/*
 * Teleport the player one level up or down (random when legal)
 */
void teleport_player_level(bool friendly)
{
	/* Check for specialty resistance on hostile teleports */
	if ((friendly == FALSE) && (check_ability(SP_PHASEWALK)))
	{
		msg_print("Teleport Resistance!");
                return;
        }

        if (!p_ptr->depth)
        {
                message(MSG_TPLEVEL, 0, "You sink through the floor.");

                /* New depth */
                p_ptr->depth++;

                /* Leaving */
                p_ptr->leaving = TRUE;
        }

        else if (is_quest(p_ptr->depth) || (p_ptr->depth >= MAX_DEPTH-1))
        {
                message(MSG_TPLEVEL, 0, "You rise up through the ceiling.");

                /* New depth */
                p_ptr->depth--;

                /* Leaving */
                p_ptr->leaving = TRUE;
        }

        else if (rand_int(100) < 50)
        {
                message(MSG_TPLEVEL, 0, "You rise up through the ceiling.");

                /* New depth */
                p_ptr->depth--;

                /* Leaving */
                p_ptr->leaving = TRUE;
        }

        else
        {
                message(MSG_TPLEVEL, 0, "You sink through the floor.");

                /* New depth */
                p_ptr->depth++;

                /* Leaving */
                p_ptr->leaving = TRUE;
        }

        /* Sound */
        sound(SOUND_TPLEVEL);
}
	





/*
 * Return a color to use for the bolt/ball spells
 */
static byte spell_color(int type)
{
	/* Analyze */
	switch (type)
	{
	case GF_ROCK:	 return (TERM_SLATE);
	case GF_SHOT:	 return (TERM_SLATE);
	case GF_ARROW:	 return (TERM_L_UMBER);
	case GF_MISSILE:     return (TERM_UMBER);
	case GF_PMISSILE:    return (TERM_GREEN);
	case GF_WHIP:	 return (TERM_UMBER);
      
	case GF_ACID:        return (TERM_SLATE);
	case GF_ELEC:	 return (TERM_BLUE);
	case GF_FIRE:	 return (TERM_RED);
	case GF_COLD:	 return (TERM_WHITE);
	case GF_POIS:	 return (TERM_GREEN);
      
	case GF_PLASMA:      return (TERM_RED);
	case GF_HELLFIRE:    return (TERM_RED);
	case GF_ICE:         return (TERM_WHITE);
      
      
	case GF_LITE_WEAK:   return (TERM_ORANGE);
	case GF_LITE:	 return (TERM_ORANGE);
	case GF_DARK_WEAK:   return (TERM_L_DARK);
	case GF_DARK:	 return (TERM_L_DARK);
	case GF_MORGUL_DARK: return (TERM_L_DARK);
      
	case GF_CONFUSION:   return (TERM_L_UMBER);
	case GF_SOUND:	 return (TERM_YELLOW);
	case GF_SHARD:	 return (TERM_UMBER);
	case GF_INERTIA:     return (TERM_L_WHITE);
	case GF_GRAVITY:     return (TERM_L_WHITE);
	case GF_FORCE:	 return (TERM_UMBER);
	case GF_WATER:	 return (TERM_SLATE);
	case GF_STORM:	 return (TERM_SLATE);
      
	case GF_NEXUS:	 return (TERM_L_RED);
	case GF_NETHER:	 return (TERM_L_GREEN);
	case GF_CHAOS:	 return (TERM_VIOLET);
	case GF_DISENCHANT:  return (TERM_VIOLET);
	case GF_TIME:	 return (TERM_L_BLUE);
      
	case GF_MANA:	 return (TERM_VIOLET);
	case GF_HOLY_ORB:    return (TERM_L_DARK);
	case GF_METEOR:	 return (TERM_RED);
	case GF_SPIRIT:	 return (TERM_L_DARK);
	}
  
	/* Standard "color" */
	return (TERM_WHITE);
}



/*
 * Find the attr/char pair to use for a spell effect
 *
 * It is moving (or has moved) from (x,y) to (nx,ny).
 *
 * If the distance is not "one", we (may) return "*".
 */
static u16b bolt_pict(int y, int x, int ny, int nx, int typ)
{
	int base;
  
	byte k;
  
	byte a;
	char c;
  
	/* No motion (*) */
	if ((ny == y) && (nx == x)) base = 0x30;
  
	/* Vertical (|) */
	else if (nx == x) base = 0x40;
  
	/* Horizontal (-) */
	else if (ny == y) base = 0x50;
  
	/* Diagonal (/) */
	else if ((ny-y) == (x-nx)) base = 0x60;
  
	/* Diagonal (\) */
	else if ((ny-y) == (nx-x)) base = 0x70;
  
	/* Weird  */
	else base = 0x30;
  
	/* Basic spell color */
	k = spell_color(typ);
  
	/* Obtain attr/char */
	a = misc_to_attr[base+k];
	c = misc_to_char[base+k];
  
	/* Create pict */
	return (PICT(a,c));
}


/*
 * Increase the short term "heighten power".  Initially used for special
 * ability "Heighten Power".
 */
void add_heighten_power(int value)
{
	int max_heighten_power = 60 + ((5 * p_ptr->lev) / 2);
  
        /* Not already max-ed */
        if (p_ptr->heighten_power < max_heighten_power)
        {
                /* Increase Hieghten Power */
                p_ptr->heighten_power += value;

                /* Apply Cap */
                p_ptr->heighten_power = (p_ptr->heighten_power > max_heighten_power ? max_heighten_power : p_ptr->heighten_power);
        }

        /* Redraw mana display */
        p_ptr->redraw |= (PR_MANA);
}

/*
 * Increase the short term "speed boost".  Initially used for special
 * ability "Fury".
 */
void add_speed_boost(int value)
{
	int max_speed_boost = 25 + ((3 * p_ptr->lev) / 2);
  
	/* Not already max-ed */
	if (p_ptr->speed_boost < max_speed_boost)
	{
		/* Increase Speed Boost */
                p_ptr->speed_boost += value;

                /* Apply Cap */
                p_ptr->speed_boost = (p_ptr->speed_boost > max_speed_boost ? max_speed_boost : p_ptr->speed_boost);

                /* Recalculate bonuses */
                p_ptr->update |= (PU_BONUS);
	}
}

/*
 * Reduce dam by an amount appropriate to the players resistance to
 * the damage type.
 *
 * Return damage reduction calculated.
 *
 * Rand factor should be 0 for low elements and informational queries,
 * 1 for damage from most high elements, 2 damage from chaos.
 */
int resist_damage(int dam, byte resist, byte rand_factor)
{
        /* Base Value */
        int resist_percentage = extract_resistance[p_ptr->res_list[resist]];

        /* Randomize if requested */
        /* No randomization of vulnerability */
	if ((rand_factor != 0) & (resist_percentage > 0) & (resist_percentage < 100))
	{
                int max_range, range;

                /* How far is the closer of zero and 100 */
                max_range = ((resist_percentage > 50) ? (100-resist_percentage) : resist_percentage);

                /* Random range is rand_factor times 10% of that range */
                range = (rand_factor * max_range) / 10;

                /* Randomize */
                resist_percentage = resist_percentage - range + rand_int((2 * range) + 1);
        }

        /* Return damage reduction */
	return (((dam * resist_percentage) + 99) / 100);
}

/*
 * Decreases players hit points and sets death flag if necessary
 *
 * Hack -- this function allows the user to save (or quit) the game
 * when he dies, since the "You die." message is shown before setting
 * the player to "dead".
 */
void take_hit(int dam, cptr kb_str)
{
	int old_chp = p_ptr->chp;

        int warning = (p_ptr->mhp * op_ptr->hitpoint_warn / 10);

#ifdef _WIN32_WCE
	unsigned long fake_time(unsigned long* fake_time_t);
	time_t ct = fake_time(0);
#else
	time_t ct = time((time_t*)0);
#endif
	char long_day[25];
	char buf[120];
  
        /* Paranoia */
        if (p_ptr->is_dead) return;

        /* Disturb */
        if (dam > 0) disturb(2, 0);
        else disturb(1, 0);

        /* Hurt the player */
        p_ptr->chp -= dam;
  
	/* Specialty Ability Fury */
	if (check_ability(SP_FURY)) add_speed_boost(1 + ((dam * 70) / p_ptr->mhp));
  
	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);
  
	/* Window stuff */
	p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		/* Sound */
		sound(SOUND_DEATH);
      
		/* Hack -- Note death */
		message(MSG_DEATH, 0, "You die.");
		msg_print(NULL);
      
		/* Ask for a screen dump */
		if (get_check("Would you like to dump the final screen? "))
		{
			/* Flush the messages */
			msg_print(NULL);

			/* Redraw */
			redraw_stuff();
  
			/* Dump the screen */
			do_cmd_save_screen();
		}
      
		/* Note cause of death */
		strcpy(p_ptr->died_from, kb_str);
      
		/* No longer a winner */
		p_ptr->total_winner = FALSE;
      
		/* Note death */
		p_ptr->is_dead = TRUE;
      
		/* Leaving */
		p_ptr->leaving = TRUE;
      
		/* Get time */
#ifdef _WIN32_WCE
		{
			char* fake_ctime(const unsigned long* fake_time_t);
			sprintf(long_day, "%-.6s %-.2s",
				fake_ctime(&ct) + 4, fake_ctime(&ct) + 22);
		}
#else
		(void)strftime(long_day, 25, "%m/%d/%Y at %I:%M %p", localtime(&ct));
#endif
      
		/* Add note */
      
		/*killed by */
		sprintf(buf, "Killed by %s.", p_ptr->died_from);
      
		/* Write message */
		make_note(buf,  p_ptr->depth, NOTE_DEATH, p_ptr->lev);
      
		/* date and time*/
		sprintf(buf, "Killed on %s.", long_day);
      
		/* Write message */
		make_note(buf,  p_ptr->depth, NOTE_DEATH, p_ptr->lev);
          
		/* Dead */
		return;
	}
  
	/* Hitpoint warning */
	if (p_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (alert_hitpoint && (old_chp > warning))
		{
			bell("Low hitpoint warning!");
		}
      
		/* Message */
		msg_print("*** LOW HITPOINT WARNING! ***");
		msg_print(NULL);
	}
}





/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable items */
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	{
		return (TRUE);
	}
      
	/* Staffs/Scrolls are wood/paper */
	case TV_STAFF:
	case TV_SCROLL:
	{
		return (TRUE);
	}
      
	/* Ouch */
	case TV_CHEST:
	{
		return (TRUE);
	}
      
	/* Junk is useless */
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	{
		return (TRUE);
	}
	}
  
	return (FALSE);
}


/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_RING:
	case TV_WAND:
	case TV_ROD:
	{
		return (TRUE);
	}
	}
  
	return (FALSE);
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
static bool hates_fire(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable */
	case TV_LITE:
	case TV_ARROW:
	case TV_BOW:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	{
		return (TRUE);
	}
      
	/* Books */
	case TV_MAGIC_BOOK:
	case TV_PRAYER_BOOK:
	case TV_DRUID_BOOK:
	case TV_NECRO_BOOK:
	{
		return (TRUE);
	}
      
	/* Chests */
	case TV_CHEST:
	{
		return (TRUE);
	}
      
	/* Staffs/Scrolls burn */
	case TV_STAFF:
	case TV_SCROLL:
	{
		return (TRUE);
	}
	}
  
	return (FALSE);
}


/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_POTION:
	case TV_FLASK:
	case TV_BOTTLE:
	{
		return (TRUE);
	}
	}
  
	return (FALSE);
}









/*
 * Melt something
 */
static int set_acid_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
        if (!hates_acid(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
        if (f3 & (TR3_IGNORE_ACID)) return (FALSE);
        return (TRUE);
}


/*
 * Electrical damage
 */
static int set_elec_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
        if (!hates_elec(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
        if (f3 & (TR3_IGNORE_ELEC)) return (FALSE);
        return (TRUE);
}


/*
 * Burn something
 */
static int set_fire_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
        if (!hates_fire(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
        if (f3 & (TR3_IGNORE_FIRE)) return (FALSE);
        return (TRUE);
}


/*
 * Freeze things
 */
static int set_cold_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
        if (!hates_cold(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
        if (f3 & (TR3_IGNORE_COLD)) return (FALSE);
        return (TRUE);
}




/*
 * This seems like a pretty standard "typedef"
 */
typedef int (*inven_func)(object_type *);

/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * New-style wands and rods handled correctly. -LM-
 * Returns number of items destroyed.
 */
static int inven_damage(inven_func typ, int perc)
{
	int i, j, k, amt;
  
	object_type *o_ptr;
  
	char o_name[120];
  
  
	/* Count the casualties */
	k = 0;
  
	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];
      
		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;
      
		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr)) continue;
      
		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Scrolls and potions are more vulnerable. */
			if ((o_ptr->tval == TV_SCROLL) || (o_ptr->tval == TV_POTION))
			{
				perc = 3 * perc / 2;
			}
			/* Rods are tough. */
			if (o_ptr->tval == TV_ROD) perc = (perc / 4);
	  
			/* Count the casualties. */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (rand_int(1000) < perc) amt++;
			}
	  
			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(o_name, o_ptr, FALSE, 3);
	      
				/* Message */
				msg_format("%sour %s (%c) %s destroyed!",
					   ((o_ptr->number > 1) ?
					    ((amt == o_ptr->number) ? "All of y" :
					     (amt > 1 ? "Some of y" : "One of y")) : "Y"),
					   o_name, index_to_label(i),
					   ((amt > 1) ? "were" : "was"));
	      
				/* Hack -- If rods or wand are destroyed, the total maximum 
				 * timeout or charges of the stack needs to be reduced, 
				 * unless all the items are being destroyed. -LM-
				 */
				if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD)) 
				    && (amt < o_ptr->number))
				{
					o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
				}
	      
	      
				/* Destroy "amt" items */
				inven_item_increase(i, -amt);
				inven_item_optimize(i);
	      
				/* Count the casualties */
				k += amt;
			}
		}
	}
  
	/* Return the casualty count */
	return (k);
}




/*
 * Acid has hit the player, attempt to affect some armor, if the armor
 * is not melded with the player's body because of a shapechange.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
static int minus_ac(int dam)
{
	object_type *o_ptr = NULL;
  
	u32b f1, f2, f3;
  
	char o_name[120];
  
	/* If the player has shapechanged, their armor is part of their
	 * body, and cannot be damaged. -JL-
	 */
	if (SCHANGE) return (FALSE);
  
	/* Not all attacks hurt armour */
	if (dam < 15)
	{
		if (randint(3) <= 2) return (FALSE);
	}
	else if (dam < 25)
	{
		if (randint(2) == 1) return (FALSE);
	}
	else if (dam < 50)
	{
		if (randint(3) == 1) return (FALSE);
	}
  
	/* Pick a (possibly empty) equipment slot */
	switch (randint(6))
	{
	case 1: o_ptr = &inventory[INVEN_BODY]; break;
	case 2: o_ptr = &inventory[INVEN_ARM]; break;
	case 3: o_ptr = &inventory[INVEN_OUTER]; break;
	case 4: o_ptr = &inventory[INVEN_HANDS]; break;
	case 5: o_ptr = &inventory[INVEN_HEAD]; break;
	case 6: o_ptr = &inventory[INVEN_FEET]; break;
	}
  
	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);
  
	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);
  
  
	/* Describe */
	object_desc(o_name, o_ptr, FALSE, 0);
  
	/* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3);

        /* Object resists */
        if (f3 & (TR3_IGNORE_ACID))
        {
                msg_format("Your %s is unaffected!", o_name);

		return (TRUE);
	}
  
	/* Message */
	msg_format("Your %s is damaged!", o_name);
  
	/* Damage the item */
	o_ptr->to_a--;
  
	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);
  
	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid.  Resistances now reduce inventory
 * destruction.   Acid can reduce CHR, as in Zangband. -LM-
 */
void acid_dam(int dam, cptr kb_str)
{
	int inv = 0;
  
	/* No damage. */
	if (dam <= 0) return;
  
  
	/* If any armor gets hit, defend the player and his backpack */
	if (minus_ac(dam)) dam = 2 * dam / 3;
  
	/* Determine the chance in 1000 of an inventory item being lost (note 
	 * that scrolls and potions have a +50% extra chance to be lost).
	 */
	inv = 3 + dam / 15;
	if (inv > 30) inv = 30;
  
	/* Partial resistance to inventory damage */
	inv -= resist_damage(((4 * inv) / 5), P_RES_ACID, 0);
  
	/* Resist the damage */
	dam -= resist_damage(dam, P_RES_ACID, 0);
  
	/* Players can lose unsustained CHR to strong, unresisted acid */
	if ((!p_resist_pos(P_RES_ACID)) && (randint(HURT_CHANCE) == 1) && (dam > 9))
		(void) do_dec_stat(A_CHR);
  
	/* Take damage */
	take_hit(dam, kb_str);
  
	/* Inventory damage */
	inven_damage(set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity.  Resistances now reduce inventory
 * destruction.   Electricity can reduce DEX, as in Zangband.  Electricity can 
 * stun the player. -LM-
 */
void elec_dam(int dam, cptr kb_str)
{
	int inv = 0;
  
	/* No damage. */
	if (dam <= 0) return;
  
  
	/* Determine the chance in 1000 of an inventory item being lost (note 
	 * that scrolls and potions have a +50% extra chance to be lost).
	 */
	inv = 3 + dam / 15;
	if (inv > 30) inv = 30;
  
	/* Partial resistance to inventory damage */
	inv -= resist_damage(((4 * inv) / 5), P_RES_ELEC, 0);
  
	/* Resist the damage */
	dam -= resist_damage(dam, P_RES_ELEC, 0);
  
	/* Players can lose unsustained DEX to strong, unresisted electricity */
	if ((!p_resist_pos(P_RES_ELEC)) && (randint(HURT_CHANCE) == 1) && (dam > 9))
		(void) do_dec_stat(A_DEX);
  
	/* Can stun, if enough damage is done. */
	if (dam > 30)
	{
		if (randint(dam - 15) > dam / 2)
			set_stun(p_ptr->stun += (int)rand_int(dam > 900 ? 50 : 5 + dam / 20));
	}
  
	/* Take damage */
	take_hit(dam, kb_str);
  
	/* Inventory damage */
	inven_damage(set_elec_destroy, inv);
}


/*
 * Hurt the player with Fire.  Resistances now reduce inventory
 * destruction.   Fire can reduce STR. -LM-
 */
void fire_dam(int dam, cptr kb_str)
{
	int inv = 0;
  
	/* No damage. */
	if (dam <= 0) return;
  
  
	/* Determine the chance in 1000 of an inventory item being lost (note 
	 * that scrolls and potions have a 50% chance to be lost).
	 */
	inv = 3 + dam / 15;
	if (inv > 30) inv = 30;
  
	/* Partial resistance to inventory damage */
	inv -= resist_damage(((4 * inv) / 5), P_RES_FIRE, 0);
  
	/* Resist the damage */
	dam -= resist_damage(dam, P_RES_FIRE, 0);
  
	/* Players can lose unsustained STR to strong, unresisted fire */
	if ((!p_resist_pos(P_RES_FIRE)) && (randint(HURT_CHANCE) == 1) && (dam > 9))
		(void) do_dec_stat(A_STR);
  
	/* Take damage */
	take_hit(dam, kb_str);
  
	/* Inventory damage */
	inven_damage(set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold.  Resistances now reduce inventory
 * destruction.   Cold can reduce CON. -LM-
 */
void cold_dam(int dam, cptr kb_str)
{
	int inv = 0;
  
	/* No damage. */
	if (dam <= 0) return;
  
  
	/* Determine the chance in 1000 of an inventory item being lost (note 
	 * that scrolls and potions have a +50% extra chance to be lost).
	 */
	inv = 3 + dam / 15;
	if (inv > 30) inv = 30;
  
	/* Partial resistance to inventory damage */
	inv -= resist_damage(((4 * inv) / 5), P_RES_COLD, 0);
  
	/* Resist the damage */
	dam -= resist_damage(dam, P_RES_COLD, 0);
  
	/* Players can lose unsustained CON to strong, unresisted cold */
	if ((!p_resist_pos(P_RES_COLD)) && (randint(HURT_CHANCE) == 1) && (dam > 9))
		(void)do_dec_stat(A_CON);
  
	/* Take damage */
	take_hit(dam, kb_str);
  
	/* Inventory damage */
	inven_damage(set_cold_destroy, inv);
}


/*
 * Hurt the player with Poison.
 */
void pois_dam(int dam, cptr kb_str)
{
	/* No damage. */
	if (dam <= 0) return;
  
	/* Poison Effect */
	pois_hit(dam);
  
	/* Resist the damage */
	dam -= resist_damage(dam, P_RES_POIS, 0);
  
	/* Take damage */
	take_hit(dam, kb_str);
  
	return;
}


/*
 * Add to the player's poison count.
 *
 * Returns true is poison added.
 */
bool pois_hit(int pois_inc)
{
	/* Has the attack done anything? */
	bool did_harm = FALSE;
  
	/* No damage. */
	if (pois_inc <= 0) return(did_harm);
  
	/* Take "poison" effect if not strongly resistant */
	if (!p_resist_strong(P_RES_POIS))
	{
		/* Resist the toxin */
		pois_inc -= resist_damage(pois_inc, P_RES_POIS, 0);
      
                /* Poison is not fully cumulative. */
                if (p_ptr->poisoned)
                {
                        /* 1/3 to 2/3 pois_inc + 2. */
                        if (set_poisoned(p_ptr->poisoned + 2 + randint((pois_inc + 2) / 3) +
					 (pois_inc / 3)))
                                did_harm = TRUE;
                }
                else
                {
                        /* 1/2 to whole pois_inc, plus 8. */
                        if (set_poisoned(p_ptr->poisoned + 8 + randint((pois_inc + 1) / 2) +
					 (pois_inc / 2)))
                                did_harm = TRUE;
                }
	}
  
	return(did_harm);
}



/*
 * Increase a stat by one randomized level
 *
 * Most code will "restore" a stat before calling this function,
 * in particular, stat potions will always restore the stat and
 * then increase the fully restored value.
 */
bool inc_stat(int stat)
{
        int value, gain;

	/* Then augment the current/max stat */
        value = p_ptr->stat_cur[stat];

        /* Cannot go above 18/100 */
        if (value < 18+100)
        {
                /* Gain one (sometimes two) points */
                if (value < 18)
                {
                        gain = ((rand_int(100) < 75) ? 1 : 2);
                        value += gain;
                }

                /* Gain 1/6 to 1/3 of distance to 18/100 */
                else if (value < 18+98)
                {
                        /* Approximate gain value */
                        gain = (((18+100) - value) / 2 + 3) / 2;
	  
			/* Paranoia */
			if (gain < 1) gain = 1;
	  
			/* Apply the bonus */
			value += randint(gain) + gain / 2;
	  
			/* Maximal value */
			if (value > 18+99) value = 18 + 99;
                }

                /* Gain one point at a time */
                else
                {
                        value++;
                }
      
		/* Save the new value */
		p_ptr->stat_cur[stat] = value;
      
		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
		{
			p_ptr->stat_max[stat] = value;
		}
      
		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
      
		/* Success */
		return (TRUE);
	}
  
	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(int stat, int amount, int permanent)
{
	int cur, max, loss, same, res = FALSE;
  
  
	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];
  
	/* Note when the values are identical */
	same = (cur == max);
  
	/* Damage "current" value */
	if (cur > 3)
	{
		/* Handle "low" values */
		if (cur <= 18)
		{
			if (amount > 90) cur--;
			if (amount > 50) cur--;
			if (amount > 20) cur--;
			cur--;
		}
      
		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);
	  
			/* Paranoia */
			if (loss < 1) loss = 1;
	  
			/* Randomize the loss */
			loss = ((randint(loss) + loss) * amount) / 100;
	  
			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;
	  
			/* Lose some points */
			cur = cur - loss;
	  
			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}
      
		/* Prevent illegal values */
		if (cur < 3) cur = 3;
      
		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}
  
	/* Damage "max" value */
	if (permanent && (max > 3))
	{
		/* Handle "low" values */
		if (max <= 18)
		{
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}
      
		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			if (loss < 1) loss = 1;
			loss = ((randint(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;
	  
			/* Lose some points */
			max = max - loss;
	  
			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}
      
		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;
      
		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}
  
	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;
      
		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}
  
	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(int stat)
{
	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];
      
		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
      
		/* Success */
		return (TRUE);
	}
  
	/* Nothing to restore */
	return (FALSE);
}


/*
 * Hostile removal of player mana.
 *
 * "power" is the amount of mana to try to remove.
 *
 * Return the number amount removed.
 */
int remove_player_mana(int power)
{
	/* Full drain */
	if (power >= p_ptr->csp)
	{
		power = p_ptr->csp;
		p_ptr->csp = 0;
		p_ptr->csp_frac = 0;
	}
  
	/* Partial drain */
	else
	{
		p_ptr->csp -= power;
	}
  
	return(power);
}


/*
 * "Dispel Magic" - remove temporary enhancements from the player.
 *
 * "power" is the size of the dieroll for the saving throw vs each effect.
 *
 * Return the number of effects removed.
 */
int apply_dispel(int power)
{
	int num_effects = 0;
  
	if (p_ptr->fast && (!check_save(power)))
	{
		set_fast(0);
		num_effects += 1;
	}
	if (p_ptr->protevil && (!check_save(power)))
	{
		set_protevil(0);
		num_effects += 1;
	}
	if (p_ptr->magicdef && (!check_save(power)))
	{
		set_extra_defences(0);
		num_effects += 1;
	}
	if (p_ptr->hero && (!check_save(power)))
	{
		set_hero(0);
		num_effects += 1;
	}
	if (p_ptr->shero && (!check_save(power)))
	{
		set_shero(0);
		num_effects += 1;
	}
	if (p_ptr->shield && (!check_save(power)))
	{
		set_shield(0);
		num_effects += 1;
	}
	if (p_ptr->blessed && (!check_save(power)))
	{
		set_blessed(0);
		num_effects += 1;
	}
	if (p_ptr->tim_invis && (!check_save(power)))
	{
		set_tim_invis(0);
		num_effects += 1;
	}
	if (p_ptr->tim_infra && (!check_save(power)))
	{
		set_tim_infra(0);
		num_effects += 1;
	}
	if (p_ptr->tim_esp && (!check_save(power)))
	{
		set_tim_esp(0);
		num_effects += 1;
	}
	if (p_ptr->superstealth && (!check_save(power)))
	{
		set_superstealth(0);
		num_effects += 1;
	}
	if (p_ptr->ele_attack && (!check_save(power)))
	{
		set_ele_attack(0, 0);
		num_effects += 1;
	}
	if (p_ptr->oppose_acid && (!check_save(power)))
	{
		set_oppose_acid(0);
		num_effects += 1;
	}
	if (p_ptr->oppose_elec && (!check_save(power)))
	{
		set_oppose_elec(0);
		num_effects += 1;
	}
	if (p_ptr->oppose_fire && (!check_save(power)))
	{
		set_oppose_fire(0);
		num_effects += 1;
	}
	if (p_ptr->oppose_cold && (!check_save(power)))
	{
		set_oppose_cold(0);
		num_effects += 1;
	}
	if (p_ptr->oppose_pois && (!check_save(power)))
	{
		set_oppose_pois(0);
		num_effects += 1;
	}
	if (p_ptr->word_recall && (!check_save(power)))
	{
		set_recall(0);
		num_effects += 1;
	}
	if ((p_ptr->special_attack & (ATTACK_CONFUSE)) && (!check_save(power)))
	{
		p_ptr->special_attack &= ~(ATTACK_CONFUSE);
		msg_print("Your hands stop glowing.");
		num_effects += 1;
	}
	if ((p_ptr->special_attack & (ATTACK_BLKBRTH)) && (!check_save(power)))
	{
		p_ptr->special_attack &= ~(ATTACK_BLKBRTH);
		msg_print("Your hands stop radiating Night.");
		num_effects += 1;
	}
	if ((p_ptr->special_attack & (ATTACK_FLEE)) && (!check_save(power)))
	{
		p_ptr->special_attack &= ~(ATTACK_FLEE);
		msg_print("You forget your escape plan.");
		num_effects += 1;
	}
	if ((p_ptr->special_attack & (ATTACK_SUPERSHOT)) && (!check_save(power)))
	{
		p_ptr->special_attack &= ~(ATTACK_SUPERSHOT);
		msg_print("Your ready crossbow bolt seems less dangerous.");
		num_effects += 1;
	}
	if ((p_ptr->special_attack & (ATTACK_HOLY)) && (!check_save(power)))
	{
		p_ptr->special_attack &= ~(ATTACK_HOLY);
		msg_print("Your Holy attack dissipates.");
		num_effects += 1;
	}
	if (SCHANGE && (!check_save(power)))
	{
		shapechange(SHAPE_NORMAL);
		num_effects += 1;
	}
  
	return(num_effects);
}


/*
 * Apply disenchantment to the player's stuff, unless it is melded with
 * his body because of a shapechange.
 *
 * This function is also called from the "melee" code.
 *
 * Some effects require a high enough damage.
 *
 * Return "TRUE" if the player notices anything.
 */
bool apply_disenchant(int dam)
{
	int t = 0;
  
	object_type *o_ptr;
  
	char o_name[120];
  
  
	/* Disenchantment can force the player back into his normal form. */
	if ((SCHANGE) && (randint(dam) > (20 + (dam/3))) && (!check_save(150)))
	{
		/* Change back to normal form. */
		shapechange(SHAPE_NORMAL);
	}
  
	/* If the player retains his shapeshift, his equipment cannot be harmed 
	 * by disenchantment.
	 */
	if (SCHANGE) 
	{
		return (FALSE);
	}
  
	/* Pick a random slot */
	switch (randint(8))
	{
	case 1: t = INVEN_WIELD; break;
	case 2: t = INVEN_BOW; break;
	case 3: t = INVEN_BODY; break;
	case 4: t = INVEN_OUTER; break;
	case 5: t = INVEN_ARM; break;
	case 6: t = INVEN_HEAD; break;
	case 7: t = INVEN_HANDS; break;
	case 8: t = INVEN_FEET; break;
	}
  
	/* Get the item */
	o_ptr = &inventory[t];
  
	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);
  
  
	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0))
	{
		/* Nothing to notice */
		return (FALSE);
	}
  
  
	/* Describe the object */
	object_desc(o_name, o_ptr, FALSE, 0);
  
  
	/* Artifacts have a 70% chance to resist */
	if (artifact_p(o_ptr) && (rand_int(100) < 70))
	{
		/* Message */
		msg_format("Your %s (%c) resist%s disenchantment!",
			   o_name, index_to_label(t),
			   ((o_ptr->number != 1) ? "" : "s"));
      
		/* Notice */
		return (TRUE);
	}
  
  
	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && (rand_int(100) < 20)) o_ptr->to_h--;
  
	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && (rand_int(100) < 20)) o_ptr->to_d--;
  
	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && (rand_int(100) < 20)) o_ptr->to_a--;
  
	/* Message */
	msg_format("Your %s (%c) %s disenchanted!",
		   o_name, index_to_label(t),
		   ((o_ptr->number != 1) ? "were" : "was"));
  
	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
  
	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
	/* Notice */
	return (TRUE);
}


/*
 * Apply Nexus
 */
static void apply_nexus(monster_type *m_ptr)
{
	int max1, cur1, max2, cur2, ii, jj;

        switch (randint(7))
        {
	case 1: case 2: case 3:
	{

		teleport_player(200, FALSE);
	
		break;
	}

	case 4: case 5:
	{

		teleport_player_to(m_ptr->fy, m_ptr->fx, FALSE);
	
		break;
	}
      
	case 6:
	{
		if (check_save(100))
		{
			msg_print("You resist the effects!");
			break;
		}
	
		/* Teleport Level */
		teleport_player_level(FALSE);
		break;
	}
      
	case 7:
	{
		if (check_save(100))
		{
			msg_print("You resist the effects!");
			break;
		}
	
		msg_print("Your body starts to scramble...");
	
		/* Pick a pair of stats */
		ii = rand_int(A_MAX);
		for (jj = ii; jj == ii; jj = rand_int(A_MAX)) /* loop */;
	
		max1 = p_ptr->stat_max[ii];
		cur1 = p_ptr->stat_cur[ii];
		max2 = p_ptr->stat_max[jj];
		cur2 = p_ptr->stat_cur[jj];
	
		p_ptr->stat_max[ii] = max2;
		p_ptr->stat_cur[ii] = cur2;
		p_ptr->stat_max[jj] = max1;
		p_ptr->stat_cur[jj] = cur1;
	
		p_ptr->update |= (PU_BONUS);
	
		break;
	}
	}
}





/*
 * Mega-Hack -- track "affected" monsters (see "project()" comments)
 */
static int project_m_n;
static int project_m_x;
static int project_m_y;



/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * Hack -- We also "see" grids which are "memorized".
 *
 * Perhaps we should affect doors and/or walls.
 *
 * Handle Oangband terrain.  Some terrain types that change the damage 
 * done by various projections are marked (using the CAVE_TEMP flag) for 
 * later processing.  This prevents a fire breath, for example, changing 
 * floor to lava and then getting the damage bonuses that accrue to fire 
 * spells on lava.  We use "dist" to keep terrain alteration under control.
 */
static bool project_f(int who, int y, int x, int dist, int dam, int typ)
{
	bool obvious = FALSE;
  
	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
	case GF_ACID:
	case GF_ELEC:
	case GF_METEOR:
	case GF_SHARD:
	case GF_FORCE:
	case GF_SOUND:
	case GF_MANA:
	case GF_HOLY_ORB:
	{
		break;
	}
      
	/* Can solidify lava.  See "project_t()". */
	case GF_COLD:
	case GF_ICE:
	{
		/* Mark the lava grid for (possible) later alteration. */
		if ((cave_feat[y][x] == FEAT_LAVA) && (dist <= 1))
			cave_info[y][x] |= (CAVE_TEMP);
		break;
	}
      
	/* Can burn, evaporate, and even make lava.  See "project_t()". */
	case GF_FIRE:
	case GF_HELLFIRE:
	case GF_PLASMA:
	{
		if (dist <= 1)
		{
			/* Mark the grid for (possible) later alteration. */
			cave_info[y][x] |= (CAVE_TEMP);
		}
		break;
	}
      
	/* Can make pools.  See "project_t()". */
	case GF_WATER:
	case GF_STORM:
	{
		if (dist <= 1)
		{
			/* Mark the floor grid for (possible) later alteration. */
			if (cave_feat[y][x] == FEAT_FLOOR) 
				cave_info[y][x] |= (CAVE_TEMP);
		}
		break;
	}
      
	/* Destroy Traps (and Locks) */
	case GF_KILL_TRAP:
	{
		/* Destroy traps */
		if ((cave_feat[y][x] == FEAT_INVIS) ||
		    ((cave_feat[y][x] >= FEAT_TRAP_HEAD) &&
		     (cave_feat[y][x] <= FEAT_TRAP_TAIL)))
		{
			/* 95% chance of success. */
			if (randint(20) != 20)
			{
				/* Check line of sight */
				if (player_has_los_bold(y, x))
				{
					msg_print("There is a bright flash of light.");
					obvious = TRUE;
				}
		
				/* Forget the trap */
				cave_info[y][x] &= ~(CAVE_MARK);
		
				/* Destroy the trap */
				cave_set_feat(y, x, FEAT_FLOOR);
			}
			/* 5% chance of setting off the trap. */
			else
			{
				msg_print("Your magic was too weak!");
				(void)hit_trap(y, x);
			} 
		}
	
		/* Secret / Locked doors are (always) found and unlocked */
		else if ((cave_feat[y][x] == FEAT_SECRET) ||
			 ((cave_feat[y][x] >= FEAT_DOOR_HEAD + 0x01) &&
			  (cave_feat[y][x] <= FEAT_DOOR_HEAD + 0x07)))
		{
			/* Unlock the door */
			place_unlocked_door(y, x);
	    
			/* Check line of sound */
			if (player_has_los_bold(y, x))
			{
				msg_print("Click!");
				obvious = TRUE;
			}
		}
	
		break;
	}
      
	/* Destroy Doors */
	case GF_KILL_DOOR:
	{
		/* Destroy all doors.  Traps are not affected in Oangband. */
		if ((cave_feat[y][x] == FEAT_OPEN) ||
		    (cave_feat[y][x] == FEAT_BROKEN) ||
		    (cave_feat[y][x] == FEAT_SECRET) ||
		    ((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
		     (cave_feat[y][x] <= FEAT_DOOR_TAIL)))
		{
			/* Check line of sight */
			if (player_has_los_bold(y, x))
			{
				/* Message */
				msg_print("There is a bright flash of light!");
				obvious = TRUE;
		
				/* Visibility change */
				if ((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
				    (cave_feat[y][x] <= FEAT_DOOR_TAIL))
				{
					/* Update the visuals */
					p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
				}
			}
	    
			/* Forget the door */
			cave_info[y][x] &= ~(CAVE_MARK);
	    
			/* Destroy the feature */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	
		break;
	}
      
	/* Destroy walls, rubble, and doors */
	case GF_KILL_WALL:
	{
		/* Non-walls (etc) */
		if (cave_floor_bold(y, x)) break;

		/* Trees are unaffected. */
		if (cave_feat[y][x] == FEAT_TREE) break;

		/* Permanent walls and stores. */
		if (cave_feat[y][x] >= FEAT_PERM_EXTRA) break;

		/* Granite */
		if (cave_feat[y][x] >= FEAT_WALL_EXTRA)
		{
			/* Message */
			if (cave_info[y][x] & (CAVE_MARK))
			{
				msg_print("The wall turns into mud.");
				obvious = TRUE;
			}

			/* Forget the wall */
			cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);

			/* Destroy the wall */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	    
		/* Quartz / Magma with treasure */
		else if (cave_feat[y][x] >= FEAT_MAGMA_H)
		{
			/* Message */
			if (cave_info[y][x] & (CAVE_MARK))
			{
				msg_print("The vein turns into mud.");
				msg_print("You have found something!");
				obvious = TRUE;
			}

			/* Forget the wall */
			cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);

			/* Destroy the wall */
			cave_set_feat(y, x, FEAT_FLOOR);
		
			/* Place some gold */
			place_gold(y, x);
		}
	    
		/* Quartz / Magma */
		else if (cave_feat[y][x] >= FEAT_MAGMA)
		{
			/* Message */
			if (cave_info[y][x] & (CAVE_MARK))
			{
				msg_print("The vein turns into mud.");
				obvious = TRUE;
			}

			/* Forget the wall */
			cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);

			/* Destroy the wall */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	    
		/* Rubble */
		else if (cave_feat[y][x] == FEAT_RUBBLE)
		{
			/* Message */
			if (cave_info[y][x] & (CAVE_MARK))
			{
				msg_print("The rubble turns into mud.");
				obvious = TRUE;
			}

			/* Forget the wall */
			cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);

			/* Destroy the rubble */
			cave_set_feat(y, x, FEAT_FLOOR);
		
			/* Hack -- place an object.  Chance much less in Oangband. */
			if (rand_int(100) < 1)
			{
				/* Found something */
				if (player_can_see_bold(y, x))
				{
					msg_print("There was something buried in the rubble!");
					obvious = TRUE;
				}
		    
				/* Place object */
				place_object(y, x, FALSE, FALSE, FALSE);
			}
		}

		/* Destroy doors (and secret doors) */
		else if (cave_feat[y][x] >= FEAT_DOOR_HEAD)
		{
			/* Hack -- special message */
			if (cave_info[y][x] & (CAVE_MARK))
			{
				msg_print("The door turns into mud!");
				obvious = TRUE;
			}

			/* Forget the wall */
			cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);

			/* Destroy the feature */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	
		break;
	}
      
	/* Make doors */
	case GF_MAKE_DOOR:
	{
		/* Require a "naked" floor grid */
		if (!cave_naked_bold(y, x)) break;
	
		/* Create closed door */
		cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
	
		/* Observe */
		if (cave_info[y][x] & (CAVE_MARK)) obvious = TRUE;
	
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	
		break;
	}
      
	/* Make traps */
	case GF_MAKE_TRAP:
	{
		/* Require a "naked" floor grid */
		if (!cave_naked_bold(y, x)) break;

		/* Place a trap */
		place_trap(y, x);
	
		break;
	}
      
	/* Hold door or monster. */
	case GF_HOLD:
	{
		/* Require any door. */
		if ((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
		    (cave_feat[y][x] <= FEAT_DOOR_TAIL))
		{
			/* Message. */
			msg_print("You cast a binding spell on the door.");
	    
			/* Hack - maximum jamming. */
			cave_feat[y][x] = 0x2F;
		}
	}
      
	/* Light up the grid */
	case GF_LITE_WEAK:
	case GF_LITE:
	{
		/* Turn on the light */
		cave_info[y][x] |= (CAVE_GLOW);
	
		/* Grid is in line of sight */
		if (player_has_los_bold(y, x))
		{
			/* Observe */
			obvious = TRUE;
	    
			/* Fully update the visuals */
			p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
		}
	
		break;
	}
      
	/* Darken the grid */
	case GF_DARK_WEAK:
	case GF_DARK:
	{
		/* Turn off the light */
		cave_info[y][x] &= ~(CAVE_GLOW);
	
		/* Hack -- Forget "boring" grids */
		if (cave_feat[y][x] <= FEAT_INVIS)
		{
			/* Forget */
			cave_info[y][x] &= ~(CAVE_MARK);
		}
	
		/* Grid is in line of sight */
		if (player_has_los_bold(y, x))
		{
			/* Observe */
			obvious = TRUE;
	    
			/* Fully update the visuals */
			p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
		}
	
		/* All done */
		break;
	}
	}
  
	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * We are called from "project()" to "damage" objects
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * Hack -- We also "see" objects which are "memorized".
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_o(int who, int y, int x, int dam, int typ)
{
	s16b this_o_idx, next_o_idx = 0;
  
	bool obvious = FALSE;
  
	u32b f1, f2, f3;
  
	char o_name[120];
  
  
	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
      
		bool is_art = FALSE;
		bool ignore = FALSE;
		bool plural = FALSE;
		bool do_kill = FALSE;
      
		cptr note_kill = NULL;
      
		/* Acquire object */
		o_ptr = &o_list[this_o_idx];
      
		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;
      
		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3);
      
		/* Get the "plural"-ness */
		if (o_ptr->number > 1) plural = TRUE;
      
		/* Check for artifact */
		if (artifact_p(o_ptr)) is_art = TRUE;
      
		/* Analyze the type */
		switch (typ)
		{
			/* Acid -- Lots of things */
		case GF_ACID:
		{
			if (hates_acid(o_ptr) && dam > rand_int(50))
			{
				do_kill = TRUE;
				note_kill = (plural ? " melt!" : " melts!");
				if (f3 & (TR3_IGNORE_ACID)) ignore = TRUE;
			}
			break;
		}
	  
		/* Elec -- Rings and Wands */
		case GF_ELEC:
		{
			if (hates_elec(o_ptr) && dam > rand_int(40))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & (TR3_IGNORE_ELEC)) ignore = TRUE;
			}
			break;
		}
	  
		/* Fire -- Flammable objects */
		case GF_FIRE:
		{
			if (hates_fire(o_ptr) && dam > rand_int(40))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
			}
			break;
		}
	  
		/* Cold -- potions and flasks */
		case GF_COLD:
		{
			if (hates_cold(o_ptr) && dam > rand_int(40))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
				if (f3 & (TR3_IGNORE_COLD)) ignore = TRUE;
			}
			break;
		}
	  
		/* Fire + Elec */
		case GF_PLASMA:
		{
			if (hates_fire(o_ptr) && (dam > rand_int(40)))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
			}
			if (hates_elec(o_ptr) && (dam > rand_int(40)))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & (TR3_IGNORE_ELEC)) ignore = TRUE;
			}
			break;
		}
	  
		/* Fire + Cold */
		case GF_METEOR:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
			}
			if (hates_cold(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " shatter!" : " shatters!");
				if (f3 & (TR3_IGNORE_COLD)) ignore = TRUE;
			}
			break;
		}
	  
		/* Hack -- break potions and such */
		case GF_ICE:
		case GF_SHARD:
		case GF_FORCE:
		case GF_SOUND:
		{
			if (hates_cold(o_ptr) && (dam > rand_int(40)))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
			}
			break;
		}
	  
		/* Mana -- destroys everything */
		case GF_MANA:
		{
			do_kill = TRUE;
			note_kill = (plural ? " are destroyed!" : " is destroyed!");
		}
	  
		/* Holy Orb -- destroys cursed non-artifacts */
		case GF_HOLY_ORB:
		{
			if (cursed_p(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
			}
			break;
		}
	  
		/* Unlock chests */
		case GF_KILL_TRAP:
		{
			/* Chests are noticed only if trapped or locked */
			if (o_ptr->tval == TV_CHEST)
			{
				/* Disarm/Unlock traps */
				if (o_ptr->pval > 0)
				{
					/* Disarm or Unlock */
					o_ptr->pval = (0 - o_ptr->pval);
		    
					/* Identify */
					object_known(o_ptr);
		    
					/* Notice */
					if (o_ptr->marked)
					{
						msg_print("Click!");
						obvious = TRUE;
					}
				}
			}
	    
			break;
		}
		}
      
      
		/* Attempt to destroy the object */
		if (do_kill)
		{
			/* Effect "observed" */
			if (o_ptr->marked)
			{
				obvious = TRUE;
				object_desc(o_name, o_ptr, FALSE, 0);
			}
	  
			/* Artifacts, and other objects, get to resist */
			if (is_art || ignore)
			{
				/* Observe the resist */
				if (o_ptr->marked)
				{
					msg_format("The %s %s unaffected!",
						   o_name, (plural ? "are" : "is"));
				}
			}
	  
			/* Kill it */
			else
			{
				/* Describe if needed */
				if (o_ptr->marked && note_kill)
				{
					msg_format("The %s%s", o_name, note_kill);
				}
	      
				/* Delete the object */
				delete_object_idx(this_o_idx);
	      
				/* Redraw */
				lite_spot(y, x);
			}
		}
	}
  
	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Certain terrain types affect spells. -LM-
 *
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.   If can breath nether, then it resists it as well.
 *
 * XXX XXX - For monsters, hellfire is the same as fire, and morgul-dark the
 * same as darkness.
 *
 * Variable damage reductions for monster resistances use the following formulas:
 *   dam *= 3; dam /= 5 + rand_int(3):  averages (just over) 1/2rd damage.
 *   dam *= 3; dam /= 11 + rand_int(3): averages (just over) 1/4th damage.
 *   dam *= 3; dam /= 14 + rand_int(3): averages (just over) 1/5th damage.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 */
static bool project_m(int who, int y, int x, int dam, int typ, int flg)
{
	int tmp;
	int charm_boost, turn_all_boost, turn_evil_boost, turn_undead_boost;
  
	bool beguile;
  
	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_lore *l_ptr;
  
	monster_type *m2_ptr;
  
	cptr name;
  
	/* Adjustment to damage caused by terrain, if applicable. */
	int terrain_adjustment = 0;
  
	/* Is the monster "seen"? */
	bool seen = FALSE;
  
	/* Were the effects "obvious" (if seen)? */
	bool obvious = FALSE;
  
	/* Were the effects "irrelevant"? */
	bool skipped = FALSE;
  
	/* Polymorph setting (true or false) */
	int do_poly = 0;
  
	/* Confusion setting (amount to confuse) */
	int do_conf = 0;
  
	/* Stunning setting (amount to stun) */
	int do_stun = 0;
  
	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;
  
	/* Fear amount (amount to fear) */
	int do_fear = 0;
  
  
	/* Hold the monster name */
	char m_name[80];
  
	/* Assume no note */
	cptr note = NULL;
  
	/* Assume a default death */
	cptr note_dies = " dies.";
  
	/* No monster here */
	if (!(cave_m_idx[y][x] > 0)) return (FALSE);
  
	/* Walls and doors entirely protect monsters, but rubble and trees do not. */
	if (!cave_passable_bold(y,x)) return (FALSE);
  
	/* Never affect projector */
	if (cave_m_idx[y][x] == who) return (FALSE);
  
	/* Obtain monster info */
	m_ptr = &m_list[cave_m_idx[y][x]];
	r_ptr = &r_info[m_ptr->r_idx];
	l_ptr = &l_list[m_ptr->r_idx];
	name = (r_name + r_ptr->name);
	if (m_ptr->ml) seen = TRUE;
  
	/* Breathers don't blast members of the same race. */
	if ((who > 0) && (flg & (PROJECT_SAFE)))
	{
		/* Point to monster information of caster */
		m2_ptr = &m_list[who];
      
		/* Skip monsters with the same racial index */
		if (m2_ptr->r_idx == m_ptr->r_idx) return (FALSE);
	}
  
	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(m_name, m_ptr, 0);
  
	/* Monsters in stasis are invulnerable. -LM- */
	if (m_ptr->stasis)
	{
		msg_format("%^s is in stasis, and cannot be harmed.", m_name);
		return (FALSE);
	}
  
	/*
	 * Various bonuses to beguiling spells
	 */
  
	/* Specialty beguiling */
	beguile = check_ability(SP_BEGUILE);
  
	/* Charmers are good with some types of slow/sleep/confusion spells */
	charm_boost = 0;
	if (check_ability(SP_CHARM)) charm_boost = 1;

        /* Strong holy casters are good at turning */
        turn_all_boost = 0;
        if ((check_ability(SP_HOLY)) && (check_ability(SP_STRONG_MAGIC))) turn_all_boost = 1;

        /* Strong evil casters are great at turning undead; Strong holy casters are good at it */
        turn_undead_boost = 0;
        if ((check_ability(SP_EVIL)) && (check_ability(SP_STRONG_MAGIC))) turn_undead_boost = 2;
        else if ((check_ability(SP_HOLY)) && (check_ability(SP_STRONG_MAGIC))) turn_undead_boost = 1;

        /* Strong holy casters are great at turning evil; Weak holy, and strong evil casters are good at it */
        turn_evil_boost = 0;
        if ((check_ability(SP_HOLY)) && (check_ability(SP_STRONG_MAGIC))) turn_evil_boost = 2;
        else if (check_ability(SP_HOLY)) turn_evil_boost = 1;
        else if ((check_ability(SP_EVIL)) && (check_ability(SP_STRONG_MAGIC))) turn_evil_boost = 1;

        /* Determine if terrain is capable of adjusting physical damage. */
        switch (cave_feat[y][x])
	{
                /* Monsters can duck behind rubble, or take only partial damage. */
	case FEAT_RUBBLE:
	{
		if ((rand_int(4) == 0) && (!((r_ptr->flags1) & RF1_NEVER_MOVE)))
		{
			msg_format("%^s ducks behind a boulder!", m_name);
			return (FALSE);
		}
		else terrain_adjustment -= dam / 4;
		break;
	}
      
	/* Fire-based spells suffer, but water spells come into their own. */
	case FEAT_WATER:
	{
		if ((typ == GF_FIRE) || (typ == GF_HELLFIRE) ||
		    (typ == GF_PLASMA)) terrain_adjustment -= dam / 2;
		else if ((typ == GF_WATER) || (typ == GF_STORM))
			terrain_adjustment = dam / 3;
		break;
	}

	/* Cold and water-based spells suffer, and fire-based spells benefit. */
	case FEAT_LAVA:
	{
		if ((typ == GF_COLD) || (typ == GF_ICE) ||
		    (typ == GF_WATER) || (typ == GF_STORM))
			terrain_adjustment -= dam / 3;
		else if ((typ == GF_FIRE) || (typ == GF_HELLFIRE) ||
			 (typ == GF_PLASMA)) terrain_adjustment = dam / 5;
		break;
	}

	/* Monsters can duck, or take only partial damage. */
	case FEAT_TREE:
	{
		if ((rand_int(4) == 0) && (!((r_ptr->flags1) & RF1_NEVER_MOVE)))
		{
			msg_format("%^s hides behind a tree!", m_name);
			return(FALSE);
		}
		else terrain_adjustment -= dam / 4;
		break;
	}
	}
  
	/* Some monsters get "destroyed" */
	if ((r_ptr->flags3 & (RF3_DEMON)) ||
	    (r_ptr->flags3 & (RF3_UNDEAD)) ||
	    (r_ptr->flags2 & (RF2_STUPID)) ||
	    (strchr("Evg", r_ptr->d_char)))
	{
		/* Special note at death */
		note_dies = " is destroyed.";
	}
  

	/* Analyze the damage type */
	switch (typ)
	{
		/* Boulders -- damage, possibly stunning.  Can miss. */
	case GF_ROCK:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		/* XXX - Crude formula to determine hit. */
		if (rand_int(200) < r_ptr->ac) 
		{
			msg_print("The boulder misses.");
			dam = 0;
		}
	
		/* Can stun monsters. */
		if ((dam > 15) && rand_int(2) == 0) 
			do_stun = randint(dam > 240 ? 32 : dam / 8);
	
		if (seen) obvious = TRUE;
		break;
	}
      
	/* Arrows and Missiles -- XXX: damage only.  Can miss. */
	case GF_SHOT:
	case GF_ARROW:
	case GF_MISSILE:
	case GF_PMISSILE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		/* XXX - Crude formula to determine hit. */
		if (rand_int(200) < r_ptr->ac) 
		{
			msg_print("The missile misses.");
			dam = 0;
		}
	
		if (seen) obvious = TRUE;
		break;
	}
      
      
	/* Acid */
	case GF_ACID:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_ACID))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_ACID);
		}
		break;
	}
      
	/* Electricity */
	case GF_ELEC:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_ELEC))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_ELEC);
		}
		/* Can stun, if enough damage is done. */
		else if ((dam > 10) && (rand_int(2) == 0)) 
			do_stun = randint(dam > 240 ? 32 : dam / 8);
	
		break;
	}
      
	/* Fire damage */
	case GF_FIRE:
	case GF_HELLFIRE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_FIRE))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_FIRE);
		}
		break;
	}
      
	/* Cold */
	case GF_COLD:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_COLD))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_COLD);
		}
		break;
	}
      
	/* Poison */
	case GF_POIS:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_POIS))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_POIS);
		}
		break;
	}
      
	/* Ice -- Cold + Stun */
	case GF_ICE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_IM_COLD))
		{
			note = " resists a lot.";
			dam /= 9;
			if (seen) l_ptr->flags3 |= (RF3_IM_COLD);
		}
		else
		{
			do_stun = randint(dam > 240 ? 20 : dam / 12);
		}
		break;
	}
      
	/* Plasma  */
	case GF_PLASMA:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (prefix(name, "Plasma") ||
		    (r_ptr->flags4 & (RF4_BRTH_PLAS)) || 
		    (r_ptr->flags3 & (RF3_RES_PLAS)))
		{
			note = " resists a lot.";
			dam *= 3; dam /= 14 + rand_int(3);
		}

		else if ((r_ptr->flags3 & (RF3_IM_ELEC)) || (r_ptr->flags3 & (RF3_IM_FIRE)))
		{
			if ((r_ptr->flags3 & (RF3_IM_ELEC)) && (r_ptr->flags3 & (RF3_IM_FIRE)))
			{
				note = " resists a lot.";
				dam *= 3; dam /= 11 + rand_int(3);
		
				if (seen) l_ptr->flags3 |= (RF3_IM_FIRE);
				if (seen) l_ptr->flags3 |= (RF3_IM_ELEC);
			}
			else
			{
				note = " resists somewhat.";
				dam *= 3; dam /= 5 + rand_int(3);
		
				if (seen)
				{
					if ((r_ptr->flags3 & (RF3_IM_FIRE)) && 
					    (rand_int(2) == 0)) 
						l_ptr->flags3 |= (RF3_IM_FIRE);
					if ((r_ptr->flags3 & (RF3_IM_ELEC)) && 
					    (rand_int(2) == 0)) 
						l_ptr->flags3 |= (RF3_IM_ELEC);
				}
			}
		}
		break;
	}
      
	/* Light, but only hurts susceptible creatures */
	case GF_LITE_WEAK:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		/* Hurt by light */
		if (r_ptr->flags3 & (RF3_HURT_LITE))
		{
			/* Obvious effect */
			if (seen) obvious = TRUE;
	    
			/* Memorize the effects */
			if (seen) l_ptr->flags3 |= (RF3_HURT_LITE);
	    
			/* Special effect */
			note = " cringes from the light!";
			note_dies = " shrivels away in the light!";
		}
	
		/* Normally no damage */
		else
		{
			/* No damage */
			dam = 0;
		}
	
		/* Holy Light gives a chance to scare targets */
		if ((who < 0) &&
		    ((r_ptr->flags3 & (RF3_UNDEAD)) || 
		     (r_ptr->flags3 & (RF3_HURT_LITE)) || 
		     (r_ptr->flags3 & (RF3_EVIL))) && 
		    (check_ability(SP_HOLY_LIGHT)) & 
		    (randint(5) == 1))
		{
			if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
			else tmp = r_ptr->level + 2;
	    
			/* Afraid */
			if (tmp <= randint(p_ptr->lev + 10))
			{
				/* Apply some fear */
				do_fear = damroll(3, (p_ptr->lev / 2)) + 1;
				obvious = FALSE;
		
				/* Give Feedback */
				note = " is dismayed by the Light!";
			}
		}
	
		break;
	}
      
	/* Light -- opposite of Dark */
	case GF_LITE:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_LITE))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		else if (r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (seen) l_ptr->flags3 |= (RF3_HURT_LITE);
			note = " cringes from the light!";
			note_dies = " shrivels away in the light!";
			dam = 3 * dam / 2;
		}
	
		/* Holy Light gives a chance to scare targets */
		if ((who < 0) &&
		    ((r_ptr->flags3 & (RF3_UNDEAD)) || 
		     (r_ptr->flags3 & (RF3_HURT_LITE)) || 
		     (r_ptr->flags3 & (RF3_EVIL))) && 
		    (check_ability(SP_HOLY_LIGHT)) & 
		    (randint(3) == 1))
		{
			if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
			else tmp = r_ptr->level + 2;
	    
			/* Afraid */
			if (tmp <= randint((3 * p_ptr->lev / 2) + 15))
			{
				/* Apply some fear */
				do_fear = damroll(3, (3 * p_ptr->lev / 4)) + 1;
				obvious = FALSE;
		
				/* Give Feedback */
				note = " is dismayed by the Light!";
			}
		}
	
		break;
	}
      
      
	/* Dark -- opposite of Lite */
	case GF_DARK:
	case GF_MORGUL_DARK:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_DARK))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
	
		/* Creatures that use Morgul-magic are resistant to darkness. */
		else if (r_ptr->flags2 & (RF2_MORGUL_MAGIC))
		{
			note = " resists somewhat.";
			dam *= 3; dam /= 5 + rand_int(3);
		}
	
		/* Orcs partially resist darkness. */
		else if (r_ptr->flags3 & (RF3_ORC))
		{
			note = " resists somewhat.";
			dam *= 3; dam /= 5 + rand_int(3);
		}
		break;
	}
      
	/* Confusion */
	case GF_CONFUSION:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		do_conf = randint(dam > 240 ? 15 : dam / 16);
		if (r_ptr->flags4 & (RF4_BRTH_CONFU))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		else if (r_ptr->flags3 & (RF3_NO_CONF))
		{
			note = " resists somewhat.";
			dam *= 3; dam /= 5 + rand_int(3);
		}
		break;
	}

	/* Sound -- Sound breathers resist, others may be stunned. */
	case GF_SOUND:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
	
		do_stun = randint(dam > 240 ? 30 : dam / 8);
	
		if (r_ptr->flags4 & (RF4_BRTH_SOUND))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		break;
	}
      
	/* Shards -- Shard breathers resist */
	case GF_SHARD:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_SHARD))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		break;
	}
      
	/* Inertia -- breathers resist */
	case GF_INERTIA:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_INER))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		break;
	}
      
	/* Gravity -- breathers resist */
	case GF_GRAVITY:
	{
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_GRAV))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Force.  Can stun. */
	case GF_FORCE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		do_stun = randint(dam > 240 ? 20 : dam / 12);
		if (r_ptr->flags4 & (RF4_BRTH_FORCE))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Water (acid) damage -- Water spirits/elementals are immune */
	case GF_WATER:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if ((r_ptr->flags3 & (RF3_RES_WATE)) || (prefix(name, "Water")))
		{
			note = " is immune.";
			dam = 0;
		}
		break;
	}
      
	/* Storm damage -- Various immunities, resistances, & effects */
	case GF_STORM:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		if ((r_ptr->flags3 & (RF3_RES_WATE)) || (prefix(name, "Water")))
		{
			note = " is immune.";
			dam = 0;
		}
	
		/* Electricity resistance. */
		if (r_ptr->flags3 & (RF3_IM_ELEC))
		{
			note = " resists.";
			if (seen) l_ptr->flags3 |= (RF3_IM_ELEC);
			dam /= 2;
		}
		else if ((dam) && rand_int(6) == 0)
		{
			/* Lightning strike. */
			note = " is struck by lightning!";
	    
			dam += dam / 2;
		}
	
	
		/* Can stun, if enough damage is done. */
		if ((dam > 50) && (rand_int(2) == 0)) 
			do_stun = randint(dam > 240 ? 20 : dam / 12);
	
		/* Can confuse, if monster can be confused. */
		if (r_ptr->flags3 & (RF3_NO_CONF))
		{
			/* Memorize a flag. */
			if (seen) l_ptr->flags3 |= (RF3_NO_CONF);
		}
		else if ((dam > 20) && rand_int(3) == 0)
		{
			/* Get confused later */
			do_conf = randint(dam / 10) + 1;
		}
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
      
	/* Nexus -- Breathers and Nexus beings resist */
	case GF_NEXUS:
	{
		if (seen) obvious = TRUE;
		if ((r_ptr->flags3 & (RF3_RES_NEXU)) || 
		    (r_ptr->flags4 & (RF4_BRTH_NEXUS)) || prefix(name, "Nexus"))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Nether -- see above */
	case GF_NETHER:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_UNDEAD))
		{
			note = " is immune.";
			dam = 0;
			if (seen) l_ptr->flags3 |= (RF3_UNDEAD);
		}
		else if ((r_ptr->flags3 & (RF3_RES_NETH)) || 
			 (r_ptr->flags4 & (RF4_BRTH_NETHR)))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		else if (r_ptr->flags3 & (RF3_EVIL))
		{
			dam = 2 * dam / 3;
			note = " resists somewhat.";
			if (seen) l_ptr->flags3 |= (RF3_EVIL);
		}
		break;
	}
      
	/* Chaos -- Chaos breathers resist */
	case GF_CHAOS:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		do_poly = TRUE;
		do_conf = randint(dam > 240 ? 20 : dam / 12);
		if (r_ptr->flags4 & (RF4_BRTH_CHAOS))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
			do_poly = FALSE;
		}
		break;
	}
      
	/* Disenchantment -- Breathers and Disenchanters resist */
	case GF_DISENCHANT:
	{
		if (seen) obvious = TRUE;
		if ((r_ptr->flags3 & (RF3_RES_DISE)) || 
		    (r_ptr->flags4 & (RF4_BRTH_DISEN)) || prefix(name, "Disen"))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		/* If not resistant, drain mana */
		else if (m_ptr->mana)
		{
			m_ptr->mana -= ((m_ptr->mana > (dam / 5)) ? (dam / 5) : m_ptr->mana);
		}
		break;
	}
      
	/* Time -- breathers resist */
	case GF_TIME:
	{
		if (seen) obvious = TRUE;
		if (r_ptr->flags4 & (RF4_BRTH_TIME))
		{
			note = " resists.";
			dam *= 3; dam /= 14 + rand_int(3);
		}
		break;
	}
      
	/* Pure damage */
	case GF_MANA:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		break;
	}
      
	/* Holy Orb -- hurts Evil */
	case GF_HOLY_ORB:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if (r_ptr->flags3 & (RF3_EVIL))
		{
			dam = 3 * dam / 2;
			note = " is hit hard.";
			if (seen) l_ptr->flags3 |= (RF3_EVIL);
		}
		break;
	}
      
	/* Meteor -- powerful magic missile */
	case GF_METEOR:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (seen) obvious = TRUE;
		break;
	}
      
	/* Spirit (hurts all but undead).  From Sangband. */
	case GF_SPIRIT:
	{
		if (r_ptr->flags3 & (RF3_UNDEAD))
		{
			dam = 0;
			note = " is immune.";
	    
			if (seen)
			{
				/* Learn about type */
				l_ptr->flags3 |= (RF3_UNDEAD);
		
				/* Obvious */
				obvious = TRUE;
			}
		}
		break;
	}
      
      
      
	/* Drain Life */
	case GF_OLD_DRAIN:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (seen) obvious = TRUE;
		if ((r_ptr->flags3 & (RF3_UNDEAD)) ||
		    (r_ptr->flags3 & (RF3_DEMON)) ||
		    (strchr("Egv", r_ptr->d_char)))
		{
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (seen) l_ptr->flags3 |= (RF3_UNDEAD);
			}
			if (r_ptr->flags3 & (RF3_DEMON))
			{
				if (seen) l_ptr->flags3 |= (RF3_DEMON);
			}
	    
			note = " is unaffected!";
			obvious = FALSE;
			dam = 0;
		}
	
		break;
	}
      
	/* Polymorph monster (Use "dam" as "power") */
	case GF_OLD_POLY:
	{
		if (seen) obvious = TRUE;
	
		/* Attempt to polymorph (see below) */
		do_poly = TRUE;
	
		/* Powerful monsters can resist */
		if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
		    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
		{
			note = " is unaffected!";
			do_poly = FALSE;
			obvious = FALSE;
		}
	
		/* No "real" damage */
		dam = 0;
	
		break;
	}
      
      
	/* Clone monsters (Ignore "dam") */
	case GF_OLD_CLONE:
	{
		if (seen) obvious = TRUE;
	
		/* Heal fully */
		m_ptr->hp = m_ptr->maxhp;
	
		/* Speed up.  Bonus to speed reduced in Oangband. */
		if (m_ptr->mspeed < 150) m_ptr->mspeed += 5;
	
		/* Attempt to clone. */
		if (multiply_monster(cave_m_idx[y][x]))
		{
			note = " spawns!";
		}
	
		/* No "real" damage */
		dam = 0;
	
		break;
	}
      
      
	/* Heal Monster (use "dam" as amount of healing) */
	case GF_OLD_HEAL:
	{
		if (seen) obvious = TRUE;
	
		/* Wake up */
		m_ptr->csleep = 0;
	
		/* Heal */
		m_ptr->hp += dam;
	
		/* No overflow */
		if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

		/* Redraw (later) if needed */
		if (p_ptr->health_who == cave_m_idx[y][x]) p_ptr->redraw |= (PR_HEALTH);

		/* Message */
		note = " looks healthier.";
	
		/* No "real" damage */
		dam = 0;
		break;
	}
      
      
	/* Speed Monster (Ignore "dam") */
	case GF_OLD_SPEED:
	{
		if (seen) obvious = TRUE;
	
		/* Speed up */
		if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;
		note = " starts moving faster.";
	
		/* No "real" damage */
		dam = 0;
		break;
	}
      
      
	/* Slow Monster (Use "dam" as "power").   Reworked in Oangband. */
	case GF_OLD_SLOW:
	{
		if (seen) obvious = TRUE;
	
	
		/* Sometimes super-charge the spell. */
		if ((charm_boost > 0) && (randint(3) == 1))
		{
			dam += dam / 2;
		}
		else if (randint(6) == 1) dam += dam / 3;
	
		/* Beguiling specialty ability */
		if (beguile) dam += dam / 2;
	
		/* Determine monster's power to resist. */
		if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
		else tmp = r_ptr->level + 2;
	
		/* Attempt a saving throw. */
		if (tmp > randint(dam))
		{
			note = " is unaffected!";
			obvious = FALSE;
		}
	
		/* If it fails, slow down if not already slowed. */
		else
		{
			if (m_ptr->mspeed > 60)
			{
				if (r_ptr->speed - m_ptr->mspeed <= 10) 
				{
					m_ptr->mspeed -= 10;
					note = " starts moving slower.";
				}
			}
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
      
	/* Sleep (Use "dam" as "power").  Reworked in Oangband. */
	case GF_OLD_SLEEP:
	{
		if (seen) obvious = TRUE;
	
	
		/* Sometimes super-charge the spell. */
		if ((charm_boost > 0) && (randint(3) == 1))
		{
			dam += dam / 2;
		}
		else if (randint(6) == 1) dam += dam / 3;
	
		/* Beguiling specialty ability */
		if (beguile) dam += dam / 2;
	
		/* Determine monster's power to resist. */
		if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
		else tmp = r_ptr->level + 2;
	
		/* Attempt a saving throw. */
		if ((tmp > randint(dam)) || (r_ptr->flags3 & (RF3_NO_SLEEP)))
		{
			/* Memorize a flag */
			if (r_ptr->flags3 & (RF3_NO_SLEEP))
			{
				if (seen) l_ptr->flags3 |= (RF3_NO_SLEEP);
			}
	    
			/* No obvious effect */
			note = " is unaffected!";
			obvious = FALSE;
		}
	
		/* If it fails, hit the hay.  Sleeping reduced in Oangband. */
		else
		{
			/* Go to sleep (much) later */
			note = " falls asleep!";
			do_sleep = 350;
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
      
	/* Confusion (Use "dam" as "power").  Reworked in Oangband. */
	case GF_OLD_CONF:
	{
		if (seen) obvious = TRUE;
	
	
		/* Sometimes super-charge the spell. */
		if ((charm_boost > 0) && (randint(3) == 1))
		{
			dam += dam / 2;
		}
		else if (randint(6) == 1) dam += dam / 3;
	
		/* Beguiling specialty ability */
		if (beguile) dam += dam / 2;
	
		/* Determine monster's power to resist.   */
		if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
		else if (r_ptr->flags3 & (RF3_UNDEAD)) tmp = r_ptr->level + 15;
		else tmp = r_ptr->level + 2;
	
		/* Attempt a saving throw.  No rescue from previous confusion. */
		if ((tmp > randint(dam)) || (r_ptr->flags3 & (RF3_NO_CONF)))
		{
			/* Memorize a flag */
			if (r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (seen) l_ptr->flags3 |= (RF3_NO_CONF);
			}
	    
			/* No obvious effect */
			note = " is unaffected!";
			obvious = FALSE;
		}
	
		/* If it fails, become confused.  Reduced in Oangband. */
		else
		{
			/* Get confused later */
			do_conf = damroll(3, (dam / 5)) + 1;
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
	/* Hold door or monster. */
	case GF_HOLD:
	{
		/* Attempt a saving throw. */
		if ((randint(3 * r_ptr->level / 2) > p_ptr->lev * 2) || 
		    rand_int(4) == 0) note = " fights off your spell.";
		else 
		{
			m_ptr->stasis = (byte)(5 + rand_int(6));
			note = " is Held within your magics!";
		}
		/* Obvious effect */
		if (seen) obvious = TRUE;
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
	/* Powerful Holding magics against undead only. */
	case GF_HOLD_UNDEAD:
	{
		/* Only affect undead */
		if (r_ptr->flags3 & (RF3_UNDEAD))
		{
			/* Attempt a saving throw. */
			if ((randint(r_ptr->level) > p_ptr->lev * 2) || 
			    rand_int(5) == 0) note = " fights off your spell.";
			else 
			{
				m_ptr->stasis = (byte)(6 + rand_int(7));
				note = " is Held within your magics!";
			}
			/* Obvious effect */
			if (seen) obvious = TRUE;
		}
	
		/* Others ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
		}
	
		break;
	}
      
	/* Stone to Mud */
	case GF_KILL_WALL:
	{
		/* Hurt by rock remover */
		if (r_ptr->flags3 & (RF3_HURT_ROCK))
		{
			/* Notice effect */
			if (seen) obvious = TRUE;
	    
			/* Memorize the effects */
			if (seen) l_ptr->flags3 |= (RF3_HURT_ROCK);
	    
			/* Cute little message */
			note = " loses some skin!";
			note_dies = " dissolves!";
		}
	
		/* Usually, ignore the effects */
		else
		{
			/* No damage */
			dam = 0;
		}
	
		break;
	}
      
	/* Teleport undead (Use "dam" as "power") */
	case GF_AWAY_UNDEAD:
	{
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		/* No damage */
		dam = 0;
	
		break;
	}
      
	/* Teleport evil (Use "dam" as "power") */
	case GF_AWAY_EVIL:
	{
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		/* No damage */
		dam = 0;
	
		break;
	}
      
      
	/* Teleport monsters and player (Use "dam" as "power") */
	case GF_AWAY_ALL:
	{
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		/* No damage */
		dam = 0;
	
		break;
	}
      
	/* Turn undead (Use "dam" as "power").  Reworked in Oangband. */
	case GF_TURN_UNDEAD:
	{
		/* Only affect undead */
		if (r_ptr->flags3 & (RF3_UNDEAD))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_UNDEAD);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
	    
			/* Sometimes super-charge the spell. */
			if ((turn_undead_boost > 1) && (randint(2) == 1))
			{
				dam += dam / 2;
			}
			else if ((turn_undead_boost == 1) && (randint(4) == 1))
			{
				dam += dam / 2;
			}
			else if (randint(6) == 1) dam += dam / 3;
	    
			/* Beguiling specialty ability */
			if (beguile) dam += dam / 2;
	    
			/* Determine monster's power to resist. */
			if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 10;
			else tmp = r_ptr->level + 2;
	    
			/* Attempt a saving throw.  No rescue from previous fear.  */
			if (tmp > randint(dam))
			{
		
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
	    
			/* If it fails, panic. */
			else
			{
				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;
			}
		}
	
		/* All but undead ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
      
	/* Turn evil (Use "dam" as "power").  Reworked in Oangband. */
	case GF_TURN_EVIL:
	{
		/* Only affect undead */
		if (r_ptr->flags3 & (RF3_EVIL))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_EVIL);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
	    
			/* Sometimes super-charge the spell. */
			if ((turn_evil_boost > 1) && (randint(3) == 1))
			{
				dam += dam / 2;
			}
			else if ((turn_evil_boost == 1) && (randint(4) == 1))
			{
				dam += dam / 3;
			}
			else if (randint(6) == 1) dam += dam / 3;
	    
			/* Beguiling specialty ability */
			if (beguile) dam += dam / 2;
	    
			/* Determine monster's power to resist.   */
			if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
			else if (r_ptr->flags3 & (RF3_UNDEAD)) tmp = r_ptr->level + 10;
			else tmp = r_ptr->level + 2;
	    
			/* Attempt a saving throw.  No rescue from previous fear. */
			if ((tmp > randint(dam)) || (r_ptr->flags3 & (RF3_NO_FEAR)))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_FEAR))
				{
					if (seen) l_ptr->flags3 |= (RF3_NO_FEAR);
				}
		
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
	    
			/* If it fails, panic. */
			else
			{
				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;
			}
		}
	
		/* All but evil ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
	/* Frighten monsters (Use "dam" as "power").  Reworked in Oangband. */
	case GF_TURN_ALL:
	{
		/* Obvious */
		if (seen) obvious = TRUE;
	

		/* Sometimes super-charge the spell. */

		if ((turn_all_boost > 0) &&
		    (randint(3) == 1))
		{
			dam += dam / 3;
		}
		else if (randint(6) == 1) dam += dam / 3;
	
		/* Beguiling specialty ability */
		if (beguile) dam += dam / 2;
	
		/* Determine monster's power to resist.   */
		if (r_ptr->flags1 & (RF1_UNIQUE)) tmp = r_ptr->level + 20;
		else if (r_ptr->flags3 & (RF3_UNDEAD)) tmp = r_ptr->level + 20;
		else tmp = r_ptr->level + 2;
	
		/* Attempt a saving throw.  No rescue from previous fear.  */
		if ((tmp > randint(dam)) || (r_ptr->flags3 & (RF3_NO_FEAR)))
		{
			/* Memorize a flag */
			if (r_ptr->flags3 & (RF3_NO_FEAR))
			{
				if (seen) l_ptr->flags3 |= (RF3_NO_FEAR);
			}
	    
			/* No obvious effect */
			note = " is unaffected!";
			obvious = FALSE;
		}
	
		/* If it fails, panic. */
		else
		{
			/* Apply some fear */
			do_fear = damroll(4, (dam / 2)) + 1;
		}
	
		/* No physical damage. */
		dam = 0;
		break;
	}
      
      
	/* Dispel undead */
	case GF_DISP_UNDEAD:
	{
		/* Only affect undead */
		if (r_ptr->flags3 & (RF3_UNDEAD))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_UNDEAD);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";
		}
	
		/* Others ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
	    
			/* No damage */
			dam = 0;
		}
	
		break;
	}
      
      
	/* Dispel evil */
	case GF_DISP_EVIL:
	{
		/* Only affect evil */
		if (r_ptr->flags3 & (RF3_EVIL))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_EVIL);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";
		}
	
		/* Others ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
	    
			/* No damage */
			dam = 0;
		}
	
		break;
	}
      
	/* Dispel all but evil. */
	case GF_DISP_NOT_EVIL:
	{
		/* Evil is immune. */
		if (r_ptr->flags3 & (RF3_EVIL))
		{
			/* Irrelevant */
			skipped = TRUE;
	    
			/* No damage */
			dam = 0;
		}
	
		/* Others are hit. */
		else
		{
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";
		}
	
		break;
	}
      
	/* Dispel demons. Be stingy with this capacity. */
	case GF_DISP_DEMON:
	{
		/* Only affect evil */
		if (r_ptr->flags3 & (RF3_DEMON))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_DEMON);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";
		}
	
		/* Others ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
	    
			/* No damage */
			dam = 0;
		}
	
		break;
	}
      
	/* Dispel dragons. Be stingy with this capacity. */
	case GF_DISP_DRAGON:
	{
		/* Only affect evil */
		if (r_ptr->flags3 & (RF3_DRAGON))
		{
			/* Learn about type */
			if (seen) l_ptr->flags3 |= (RF3_DRAGON);
	    
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";
		}
	
		/* Others ignore */
		else
		{
			/* Irrelevant */
			skipped = TRUE;
	    
			/* No damage */
			dam = 0;
		}
	
		break;
	}
      
	/* Dispel monster */
	case GF_DISP_ALL:
	{
		/* Obvious */
		if (seen) obvious = TRUE;
	
		/* Message */
		note = " shudders.";
		note_dies = " dissolves!";
	
		break;
	}
      
	/* Dispel only weak monsters. */
	case GF_DISP_SMALL_ALL:
	{
		/* Obvious */
		if (seen) obvious = TRUE;
	
		/* Only effect the weakest creatures. */
		if (m_ptr->hp >= dam) 
		{
			dam = 0;
			note = " is unaffected.";
		}
		else
		{
			tmp = rand_int(18);
			/* Colorful messages */
			if (tmp <= 11) note_dies = " collapses.";
			if (tmp == 12) note_dies = " falls suddenly silent!";
			if (tmp == 13) note_dies = " lies stiff and still!";
			if (tmp == 14) note_dies = " dies in a fit of agony!";
			if (tmp == 15) note_dies = " squeals and topples over!";
			if (tmp == 16) note_dies = " is snuffed out!";
			if (tmp == 17) note_dies = " shrieks in mortal pain!";
		}
	
		break;
	}

	/* Default */
	default:
	{
		/* Irrelevant */
		skipped = TRUE;
	
		/* No damage */
		dam = 0;
	
		break;
	}
	}
  
  
	/* Absolutely no effect */
	if (skipped) return (FALSE);
  
  
	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & (RF1_UNIQUE)) do_poly = FALSE;
  
  
  
  
	/* "Unique" monsters can only be "killed" by the player */
	if (r_ptr->flags1 & (RF1_UNIQUE))
	{
		/* Uniques may only be killed by the player */
		if ((who > 0) && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}
  
	/* Check for death */
	if (dam > m_ptr->hp)
	{
		/* Extract method of death */
		note = note_dies;
	}
  
	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	else if (do_poly && (randint(90) > r_ptr->level))
	{
		/* Default -- assume no polymorph */
		note = " is unaffected!";
      
		/* Pick a "new" monster race */
		tmp = poly_r_idx(m_ptr->r_idx);
      
		/* Handle polymorh */
		if (tmp != m_ptr->r_idx)
		{
			/* Obvious */
			if (seen) obvious = TRUE;
	  
			/* Monster polymorphs */
			note = " changes!";
	  
			/* Turn off the damage */
			dam = 0;
	  
			/* "Kill" the "old" monster */
			delete_monster_idx(cave_m_idx[y][x]);
	  
			/* Create a new monster (no groups) */
			(void)place_monster_aux(y, x, tmp, FALSE, FALSE);
	  
			/* Hack -- Assume success XXX XXX XXX */
	  
			/* Hack -- Get new monster */
			m_ptr = &m_list[cave_m_idx[y][x]];
	  
			/* Hack -- Get new race */
			r_ptr = &r_info[m_ptr->r_idx];
		}
	}
  
	/* Sound and Impact breathers never stun */
	else if (do_stun &&
		 !(r_ptr->flags4 & (RF4_BRTH_SOUND)) &&
		 !(r_ptr->flags4 & (RF4_BRTH_FORCE)))
	{
		/* Obvious */
		if (seen) obvious = TRUE;
      
		/* Get confused */
		if (m_ptr->stunned)
		{
			note = " is more dazed.";
			tmp = m_ptr->stunned + (do_stun / 2);
		}
		else
		{
			note = " is dazed.";
			tmp = do_stun;
		}
      
		/* Apply stun */
		m_ptr->stunned = (tmp < 200) ? tmp : 200;
	}
  
	/* Confusion and Chaos breathers (and sleepers) never confuse */
	else if (do_conf &&
		 !(r_ptr->flags3 & (RF3_NO_CONF)) &&
		 !(r_ptr->flags4 & (RF4_BRTH_CONFU)) &&
		 !(r_ptr->flags4 & (RF4_BRTH_CHAOS)))
	{
		/* Obvious */
		if (seen) obvious = TRUE;
      
		/* Already partially confused */
		if (m_ptr->confused)
		{
			note = " looks more confused.";
			tmp = m_ptr->confused + (do_conf / 2);
		}
      
		/* Was not confused */
		else
		{
			note = " looks confused.";
			tmp = do_conf;
		}
      
		/* Apply confusion */
		m_ptr->confused = (tmp < 200) ? tmp : 200;
	}
  
  
	/* Fear */
	if (do_fear)
	{
		/* Increase fear */
		tmp = m_ptr->monfear + do_fear;
      
		/* Set fear */
		m_ptr->monfear = (tmp < 200) ? tmp : 200;
      
		/* Flag minimum range for recalculation */
		m_ptr->min_range = 0;
	}
  
  
	/* If another monster did the damage, hurt the monster by hand */
	if (who > 0)
	{
		/* Redraw (later) if needed */
		if (p_ptr->health_who == cave_m_idx[y][x]) p_ptr->redraw |= (PR_HEALTH);
      
		/* Wake the monster up */
		m_ptr->csleep = 0;
      
		/* Go active */
		m_ptr->mflag |= (MFLAG_ACTV);
      
		/* Hurt the monster */
		m_ptr->hp -= dam;
      
		/* Dead monster */
		if (m_ptr->hp < 0)
		{
			/* Generate treasure, etc */
			monster_death(cave_m_idx[y][x]);
	  
			/* Delete the monster */
			delete_monster_idx(cave_m_idx[y][x]);
	  
			/* Give detailed messages if destroyed */
			if (note) msg_format("%^s%s", m_name, note);
		}
      
		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);
	  
			/* Hack -- Pain message */
			else if (dam > 0) message_pain(cave_m_idx[y][x], dam);
	  
			/* Hack -- handle sleep */
			if (do_sleep)
			{
				/* Sleep */
				m_ptr->csleep = do_sleep;
	      
				/* Go inactive */
				m_ptr->mflag &= ~(MFLAG_ACTV);
			}
		}
	}
  
	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;
      
		/* Hurt the monster, check for fear and death */
		if (mon_take_hit(cave_m_idx[y][x], dam, &fear, note_dies))
		{
			/* Dead monster */
		}
      
		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);
	  
			/* Hack -- Pain message */
			else if (dam > 0) message_pain(cave_m_idx[y][x], dam);
	  
			/* Take note */
			if ((fear || do_fear) && (m_ptr->ml))
			{
				/* Sound */
				sound(SOUND_FLEE);
	      
				/* Message */
				msg_format("%^s flees in terror!", m_name);
			}
	  
			/* Hack -- handle sleep */
			if (do_sleep)
			{
				/* Sleep */
				m_ptr->csleep = do_sleep;

				/* Go inactive */
				m_ptr->mflag &= ~(MFLAG_ACTV);
			}
		}
	}
  
	/* Verify this code XXX XXX XXX */
  
	/* Update the monster */
	update_mon(cave_m_idx[y][x], FALSE);
  
	/* Redraw the monster grid */
	lite_spot(y, x);
  
  
	/* Update monster recall window */
	if (p_ptr->monster_race_idx == m_ptr->r_idx)
	{
		/* Window stuff */
		p_ptr->window |= (PW_MONSTER);
	}
  
  
	/* Track it */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;
  
  
	/* Hack -- Sound attacks are extremely noisy. */
	if (typ == GF_SOUND) 
		add_wakeup_chance = 10000;
  
	/* Otherwise, if this is the first monster hit, the spell was capable 
	 * of causing damage, and the player was the source of the spell, 
	 * make noise. -LM-
	 */
	else if ((project_m_n == 1) && (who <= 0) && (dam)) 
		add_wakeup_chance = p_ptr->base_wakeup_chance / 2 + 2500;
  
	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player.
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * We return "TRUE" if any "obvious" effects were observed.
 * Actually, for historical reasons, we just assume that the effects were
 * obvious.  XXX XXX XXX
 *
 * Variable damage reductions for player resistances use the following formulas:
 *   dam *= 5; dam /= 9 + rand_int(3):  0.56 to 0.45
 *        On average, resistance reduces damage almost in half.
 *   dam *= 5; dam /= 8 + rand_int(5):  0.63 to 0.42
 *	  On average, resistance reduces damage almost in half.
 *
 */
static bool project_p(int who, int d, int y, int x, int dam, int typ)
{
	int k = 0;
  
	/* Self Inflicted? */
	bool self = FALSE;
  
	/* Adjustment to damage caused by terrain, if any. */
	int terrain_adjustment = 0;
  
	/* Hack -- assume obvious */
	bool obvious = TRUE;
  
	/* Player blind-ness */
	bool blind = (p_ptr->blind ? TRUE : FALSE);
  
	/* Player needs a "description" (he is blind) */
        bool fuzzy = FALSE;

        /* Source monster and its race */
	monster_type *m_ptr = NULL;
	monster_race *r_ptr = NULL;

        /* Monster name (for attacks) */
        char m_name[80];
  
	/* Monster name (for damage) */
	char killer[80];
  
	/* Hack -- messages */
	cptr act = NULL;
  
	/* No player here */
	if (!(cave_m_idx[y][x] < 0)) return (FALSE);
  
	/* Check for self inflicted damage */
	if (!(who > 0)) self = TRUE;
  
	/* Limit maximum damage XXX XXX XXX */
	if (dam > 1600) dam = 1600;
  
	/* Determine if terrain is capable of adjusting physical damage. */
	switch (cave_feat[y][x])
	{
		/* A player behind rubble takes less damage. */
	case FEAT_RUBBLE:
	{
		if (randint(10) == 1) 
		{
			msg_print("You duck behind a boulder!");
			return (FALSE);
		}
		else terrain_adjustment -= dam / 6;
		break;
	}
      
	/* Fire-based spells suffer, but other spells benefit slightly 
	 * (player is easier to hit).  Water spells come into their own.
	 */
	case FEAT_WATER:
	{
		if ((typ == GF_FIRE) || (typ == GF_HELLFIRE) ||
		    (typ == GF_PLASMA)) terrain_adjustment -= dam / 4;
		else if ((typ == GF_WATER) || (typ == GF_STORM))
			terrain_adjustment = dam / 2;
		else terrain_adjustment = dam / 10;
		break;
	}
      
	/* Cold and water-based spells suffer, and fire-based spells benefit. */
	case FEAT_LAVA:
	{
		if ((typ == GF_COLD) || (typ == GF_ICE) ||
		    (typ == GF_WATER) || (typ == GF_STORM))
			terrain_adjustment -= dam / 4;
		else if ((typ == GF_FIRE) || (typ == GF_HELLFIRE) ||
			 (typ == GF_PLASMA)) terrain_adjustment = dam / 4;
		break;
	}
	/* Ranger and druids can duck, and any player will take less damage. */
	case FEAT_TREE:
	{
		if ((randint(8) == 1) && (check_ability(SP_WOODSMAN)))
		{
			msg_print("You dodge behind a tree!");
			return (FALSE);
		}
		else terrain_adjustment -= dam / 6;
		break;
	}
	}
  
	/* Hack -
	 * Darkness protects those who serve it.
	 */
	if (((cave_info[p_ptr->py][p_ptr->px] & (CAVE_GLOW)) == 0) && 
	    (p_ptr->cur_lite <= 0) && 
	    check_ability(SP_UNLIGHT)) terrain_adjustment -= dam / 4;
  
	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;
  
	/* Is this cast by a monster*/
	if (!self)
	{
		/* Get the source monster */
		m_ptr = &m_list[who];
      
		/* Get the monster race. */
		r_ptr = &r_info[m_ptr->r_idx];
      
		/* Get the monster name */
		monster_desc(m_name, m_ptr, 0);
      
		/* Get the monster's real name */
		monster_desc(killer, m_ptr, 0x88);
	}
  
	/* Default 'killer' for self inflicted damage */
	else strcpy(killer, "Dangerous Forces");
  
	/* Analyze the damage */
	switch (typ)
	{
		/* Boulders -- Can be dodged.  Crushing.  Armour protects a little. */
	case GF_ROCK:
	{
		int dodging = 0;

		/* Try for Evasion */
		if (check_ability(SP_EVASION) & (randint(75) <= p_ptr->evasion_chance))
		{
			/* Message */
			msg_print("You Evade the boulder!");
			dam = 0;
		}
	
		/* Dodging takes alertness, agility, speed, and a light pack. */
		else if ((!p_ptr->blind) && (!p_ptr->confused) && (!p_ptr->paralyzed))
		{
			/* Value for dodging should normally be between 18 and 75. */
			dodging = 2 * (adj_dex_ta[p_ptr->stat_ind[A_DEX]] - 124) + 
				extract_energy[p_ptr->pspeed] + 
				5 * adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100 / 
				(p_ptr->total_weight > 300 ? p_ptr->total_weight : 300);
	    
			/* Do we dodge the boulder? */
			if ((!self) && (dodging > 10 + rand_int(r_ptr->level)))
			{
				msg_print("You nimbly dodge aside.");
				dam = 0;
			}
		}
	
		/* We've been hit - check for damage, crushing. */
		if (dam)
		{
			/* Affected by terrain. */
			dam += terrain_adjustment;
	    
			/* Player armor reduces total damage (a little) */
			dam -= (dam * ((p_ptr->ac + p_ptr->to_a < 150) ? 
				       p_ptr->ac + p_ptr->to_a : 150) / 300);
	    
			/* Player can be crushed. */
			if (rand_int(3) == 0)
			{
				if (fuzzy) msg_print("You are crushed by a boulder!");
				else msg_print("You are crushed!");
		
				/* Be careful not to knock out the player immediately. */
				(void)set_stun(p_ptr->stun + ((dam / 2 > 50) ? 50 : dam / 2));
			}
			else
			{
				if (fuzzy) msg_print("You are hit by a boulder.");
				else msg_print("You are hit.");
			}
	    
			/* Take the damage. */
			take_hit(dam, killer);
		}
	
		break;
	}
      
	/* Sling shot -- Stunning, wounding.  Heavy armour protects well.  */
	case GF_SHOT:
	{
		/* Try for Evasion */
		if (check_ability(SP_EVASION) & (randint(75) <= p_ptr->evasion_chance))
		{
			/* Message */
			msg_print("You Evade the missile!");
			dam = 0;
		}
	
		/* Test for deflection - Only base armour counts here. */
		else if ((!self) && (p_ptr->ac > 10 + rand_int(r_ptr->level)))
		{
			if (fuzzy) msg_print("A missile glances off your armour.");
	    
			msg_print("The missile glances off your armour.");
	    
			/* No damage. */
			dam = 0;
		}
	
		/* Reduce damage if missile did not get deflected. */
		else dam -= (dam * ((p_ptr->ac + p_ptr->to_a < 150) ? 
				    p_ptr->ac + p_ptr->to_a : 150) / 250);
	
		/* We've been hit - check for stunning, wounding. */
		if (dam)
		{
			if (fuzzy) msg_print("You are hit by a sling shot.");
			else msg_print("You are hit.");
	    
			/* Affected by terrain. */
			dam += terrain_adjustment;
	    
			/* Player can be stunned. */
			if (rand_int(4) == 0)
			{
				msg_print("You are stunned.");
		
				/* Be careful not to knock out the player immediately. */
				(void)set_stun(p_ptr->stun + ((dam / 3 > 30) ? 30 : dam / 3));
			}
			/* Player can be wounded. */
			if (rand_int(4) == 0)
			{
				msg_print("You have been wounded.");
		
				/* Wound the player. */
				(void)set_cut(p_ptr->cut + (dam / 3 > 30 ? 30 : dam / 3));
			}
	    
			/* Take the damage. */
			take_hit(dam, killer);
		}
	
		break;
	}

	/* Edged physical missiles -- Frequent wounding.  Armour protects some. */
	case GF_ARROW:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;

		/* Try for Evasion */
		if (check_ability(SP_EVASION) & (randint(75) <= p_ptr->evasion_chance))
		{
			/* Message */
			msg_print("You Evade the missile!");
			dam = 0;
		}

		/* Test for a miss or armour deflection. */
		else if ((!self) && ((p_ptr->ac + p_ptr->to_a < 150 ? p_ptr->ac + p_ptr->to_a :
				      150) > randint((10 + r_ptr->level) * 5)))
		{
			if ((p_ptr->ac > 9) && (rand_int(2) == 0))
				msg_print("The missile glances off your armour.");
			else msg_print("The missile misses.");
	    
			/* No damage. */
			dam = 0;
		}
	
		/* Test for a deflection. */
		else if ((inventory[INVEN_ARM].k_idx) &&
			 (!p_ptr->shield_on_back) &&
			 (inventory[INVEN_ARM].ac  + (check_ability(SP_SHIELD_MAST) ? 3 : 0) >
			  rand_int(MAX_SHIELD_BASE_AC * 4)))
		{
			msg_print("The missile ricochets off your shield.");

			/* No damage. */
			dam = 0;
		}
	
		/* Reduce damage if missile did not get deflected. */
		else dam -= (dam * ((p_ptr->ac + p_ptr->to_a < 150) ? 
				    p_ptr->ac + p_ptr->to_a : 150) / 250);
	
		if (dam)
		{
			/* Hit the player */
			if (fuzzy) msg_print("You are hit by something hard!");
			take_hit(dam, killer);
	    
			/* Player can be wounded. */
			if (rand_int(2) == 0)
			{
				msg_print("You have been wounded.");
		
				/* Wound the player. */
				(void)set_cut(p_ptr->cut + (dam / 2 > 50 ? 50 : dam / 2));
			}
		}
	
		break;
	}
      
	/* Miscellaneous physical missiles.  Can dodge. Armour reduces damage. */
	/* Also venomous missiles.  These get nasty with Morgul-magic. */
	/* Ringwraiths and Sauron are very dangerous. */
	case GF_MISSILE:
	case GF_PMISSILE:
	{
		int dodging = 0;
	
		/* Affected by terrain. */
		dam += terrain_adjustment;

		/* Try for Evasion */
		if (check_ability(SP_EVASION) & (randint(75) <= p_ptr->evasion_chance))
		{
			/* Message */
			msg_print("You Evade the missile!");
			dam = 0;
		}
	
		/* Dodging takes alertness, agility, speed, and a light pack. */
		else if ((!p_ptr->blind) && (!p_ptr->confused) && (!p_ptr->paralyzed))
		{
			/* Value for dodging should normally be between 18 and 75. */
			dodging = 2 * (adj_dex_ta[p_ptr->stat_ind[A_DEX]] - 124) + 
				extract_energy[p_ptr->pspeed] + 
				5 * adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100 / 
				(p_ptr->total_weight > 300 ? p_ptr->total_weight : 300);
	    
			/* Do we dodge the missile (not an easy thing to do)? */
			if ((!self) && (dodging > rand_int(40 + 3 * r_ptr->level / 2)))
			{
				msg_print("You nimbly dodge aside.");
				dam = 0;
			}
		}
	
		/* Hit the player with a missile. */
		if (dam)
		{
			/* A poisonous missile hits the player. */
			if (typ == GF_PMISSILE)
			{
				/* Monster has Morgul-magic. */
				if ((!self) && (r_ptr->flags2 & (RF2_MORGUL_MAGIC)))
				{
					/* Hack - cannot rapid-fire morgul missiles. */
					if ((r_ptr->flags2 & (RF2_ARCHER)) && (m_ptr->cdis > 1) && 
					    (randint(100) > r_ptr->freq_ranged))
					{
						k = 1;
					}

					/* Hack - The Ringwraiths and Sauron are very dangerous. */
					else if ((prefix(m_name, "Sauron, the Sorcerer")) ||
						 ((r_ptr->d_char == 'W') && (r_ptr->flags1 & (RF1_UNIQUE))))
					{
						/* 40% chance of Black Breath. */
						k = randint(5);
					}
					/* Other monsters with Morgul-magic. */
					else 
					{
						/* 17% chance of Black Breath. */
						k = randint(2);
						if ((r_ptr->level > 50) && (randint(3) == 0))
							k += 2;
					}
				}
		
				/* Standard poisonous missile. */
				else k = 1;
			}
	    
			/* Ordinary missile. */
			else k = 0;
	    
			/* Hit the player */
			if (fuzzy) msg_print("You are hit by a missile.");
			else msg_print("You are hit.");
	    
			/* Armour reduces damage, if missile does not carry 
			 * the Black Breath.
			 */
			if (k < 4)
			{
				dam -= (dam * ((p_ptr->ac + p_ptr->to_a < 150) ? 
					       p_ptr->ac + p_ptr->to_a : 150) / 250);
			}
	    
			/* Ordinary missile. */
			if (k == 0) 
			{
				/* No special damage. */
				take_hit(dam, killer);
			}
	    
			/* Standard poisonous missile. */
			if (k == 1) 
			{
				/* Damage is not affected by poison. */
				take_hit(dam, killer);
		
				/* Player may be poisoned in addition. */
				pois_hit(dam + 5);
			}
	    
			/* Drain life . */
			if (k == 2) 
			{
				/* First the raw damage, */
				take_hit(dam, killer);
		
				/* Then the poison, */
				pois_hit(dam + 5);
		
				/* Then the life draining. */
				if (p_ptr->hold_life && (randint(100) > 75))
				{
					msg_print("You feel your life slipping away!");
					lose_exp(200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
				}
				else
				{
					msg_print("You feel your life draining away!");
					lose_exp(200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
				}
			}
	    
			/* Reduce stats. */
			if (k == 3) 
			{
				/* Oh no. */
				msg_print("Foul magics assault body and mind!");
		
				/* First the raw damage, */
				take_hit(dam, killer);
		
				/* Then the poison, */
				pois_hit(dam + 5);
		
				/* Then the stat loss. */
				/* Reduce all unsustained stats by 1. */
				for (k = 0; k < A_MAX; k++)
				{
					(void)do_dec_stat(k);
				}
				p_ptr->update |= (PU_BONUS);
			}
	    
			/* Inflict the Black Breath. */
			if (k >= 4) 
			{
				/* First the raw damage, */
				take_hit(dam, killer);
		
				/* Then the poison, */
				pois_hit(dam + 5);
		
				/* Then the life draining. */
				if (p_ptr->hold_life && (randint(100) > 75))
				{
					msg_print("You feel your life slipping away!");
					lose_exp(200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
				}
				else
				{
					msg_print("You feel your life draining away!");
					lose_exp(200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
				}
		
				/* Then the Black Breath. */
				if (p_ptr->black_breath == FALSE)
				{
					/* Messages. */
					msg_print("Your foe calls upon your soul!");
					msg_print(NULL);
					msg_print("You feel the Black Breath slowly draining you of life...");
				}
				p_ptr->black_breath = TRUE;
			}
		}
	
		break;
	}
      
	/* Whip or spitting attack -- pure damage */
	case GF_WHIP:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if ((!self) && (r_ptr->flags3 & (RF3_ANIMAL)))
		{
			if (fuzzy) msg_print("You have been spat upon.");
	    
			/* Ordinary spit doesn't do any damage. */
			dam = 0;
		}
		else
		{
			if (fuzzy) msg_print("You are struck by a whip!");
			take_hit(dam, killer);
		}
		break;
	}
      
	/* Standard damage -- hurts inventory */
	case GF_ACID:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by acid!");
		acid_dam(dam, killer);
		break;
	}
      
	/* Standard damage -- hurts inventory, can stun. */
	case GF_ELEC:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by lightning!");
		elec_dam(dam, killer);
		break;
	}
      
	/* Standard damage -- hurts inventory */
	case GF_FIRE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by fire!");
		fire_dam(dam, killer);
		break;
	}
      
	/* Standard damage -- hurts inventory */
	/* Morgul-cold can be dangerous, if strong and not well-resisted. */
	case GF_COLD:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by cold!");
		cold_dam(dam, killer);
	
		/* Strong Morgul-cold can have extra side effects. */
	
		/* 100% of the time if no resistance, 33% if some resistance. */
		if ((!self) && (r_ptr->flags2 & (RF2_MORGUL_MAGIC)) &&
		    ((!p_resist_pos(P_RES_COLD)) ||
		     ((!p_immune(P_RES_COLD)) && (rand_int(3) == 0))))
		{
			k = randint(3);

			if ((k == 1) && (dam >= 150))
			{
				msg_print("The cold seeps into your bones.");
				(void)do_dec_stat(A_CON);
			}
			if ((k == 2) && (dam >= 250) && (!p_ptr->hold_life))
			{
				msg_print("A deadly chill withers your lifeforce.");
				lose_exp(200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
			if ((k == 3) && (dam >= 400))
			{
				msg_print("A deadly chill drives daggers into your soul!");
				if (!p_ptr->free_act)
				{
					(void)set_paralyzed(p_ptr->paralyzed + rand_int(3) + 2);
				}
				if (!p_ptr->no_fear)
				{
					(void)set_afraid(p_ptr->afraid + rand_int(21) + 10);
				}
				if (!p_ptr->hold_life)
				{
					/* Very serious, but temporary, loss of exp. */
					lose_exp(200 + (p_ptr->exp/20) * MON_DRAIN_LIFE);
				}
			}
		}
	
		break;
	}
      
	/* Standard damage -- also poisons player */
	/* Monsters with Morgul-magic have nasty poison. */
	case GF_POIS:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		/* Morgul-poison is also acidic. */
		if ((!self) && (r_ptr->flags2 & (RF2_MORGUL_MAGIC))) 
		{
			if (fuzzy) msg_print("You are hit by acidic venom.");
			acid_dam(dam / 3, killer);
		}
		else if (fuzzy) msg_print("You are hit by poison!");

		/* Poison Damage and Add Posion */
		pois_dam(dam, killer);

		/* Some nasty possible side-effects of Morgul-poison.  Poison
		 * resistance (but not acid resistance) reduces the damage 
		 * counted when determining effects.
		 */
		if ((!self) && (r_ptr->flags2 & (RF2_MORGUL_MAGIC)))
		{
			/* Paralyzation. */
			if ((!p_ptr->free_act) && (!check_save(dam / 2 + 20))) 
			{
				msg_print("The deadly vapor overwhelms you, and you faint away!");
				(void)set_paralyzed(p_ptr->paralyzed + rand_int(3) + 2);
			}
	    
			if ((!p_ptr->no_blind) && (!check_save(dam / 2 + 20)))
			{
				(void)set_blind(p_ptr->blind + rand_int(17) + 16);
				msg_print("The deadly vapor blinds you!");
			}
		}
	
		break;
	}
      
	/* Plasma -- Combines fire and electricity. */
	case GF_PLASMA:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by plasma!");
		elec_dam((dam + 2) / 2, killer);
		fire_dam((dam + 2) / 2, killer);
		break;
	}
      
	/* Hellfire (Udun-fire) is fire and darkness, plus nastiness. */
	case GF_HELLFIRE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by hellfire!");
		fire_dam(2 * dam / 3, killer);
	
		/* Resist Darkness */
		dam -= resist_damage(dam, P_RES_DARK, 1);

		/* Blind the player */
		if (!blind && !p_ptr->no_blind && !p_resist_pos(P_RES_DARK))
		{
			(void)set_blind(p_ptr->blind + randint(5) + 2);
		}
		take_hit((dam+2) / 3, killer);
	
		/* Test player's saving throw. */
		if ((!self) && (!check_save(5 * r_ptr->level / 4)))
		{
			msg_print("Visions of hell invade your mind!");
	    
			/* Possible fear, hallucination and confusion. */
			if (!p_ptr->no_fear)
			{
				(void)set_afraid(p_ptr->afraid + randint(30) + r_ptr->level * 2);                               }
			if (!p_resist_pos(P_RES_CHAOS))
			{
				(void)set_image(p_ptr->image + rand_int(101) + 100);
			}
			if (!p_resist_pos(P_RES_CONFU))
			{
				(void)set_confused(p_ptr->confused + rand_int(31) + 30);
			}
		}
	
		break;
	}
      
	/* Ice -- cold plus stun plus cuts */
	case GF_ICE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;

		if (fuzzy) msg_print("You are hit by something sharp!");
		cold_dam(dam, killer);
		if (!p_resist_pos(P_RES_SHARD))
		{
			(void)set_cut(p_ptr->cut + damroll(5, 8));
		}
		if (!p_resist_pos(P_RES_SOUND))
		{
			(void)set_stun(p_ptr->stun + randint(15));
		}
		break;
	}
      
	/* Lite -- blinding */
	case GF_LITE:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are hit by something!");
	
		/* Resist Damage */
		dam -= resist_damage(dam, P_RES_LITE, 1);

		/* Apply Blindness */
		if (!blind && !p_ptr->no_blind  && !p_resist_pos(P_RES_LITE))
		{
			(void)set_blind(p_ptr->blind + randint(5) + ((dam > 40) ? 2 : 0));
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Dark -- blinding */
	case GF_DARK:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are hit by something!");
	
		/* Resist Damage */
		dam -= resist_damage(dam, P_RES_DARK, 1);

		/* Blind the player */
		if (!blind && !p_ptr->no_blind && !p_resist_pos(P_RES_DARK))
		{
			(void)set_blind(p_ptr->blind + randint(5) + 2);
		}
		take_hit(dam, killer);
		break;
	}

	/* Morgul-dark -- very dangerous if sufficiently powerful. */
	case GF_MORGUL_DARK:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You feel a deadly blackness surround you!");
	
		/* Resist Damage */
		dam -= resist_damage(dam, P_RES_DARK, 1);

		/* Blind the player */
		if (!blind && !p_ptr->no_blind && !p_resist_pos(P_RES_DARK))
		{
			(void)set_blind(p_ptr->blind + randint(5) + 2);
		}
		take_hit(dam, killer);
	
		/* Determine power of attack - usually between 25 and 350. */
		if (!self) k = dam * r_ptr->level / 100;
		else k = 0;

		/* Hack - The Ringwraiths and Sauron are very dangerous. */
		if ((!self) && ((prefix(m_name, "Sauron, the Sorcerer")) ||
                                ((r_ptr->d_char == 'W') && (r_ptr->flags1 & (RF1_UNIQUE)))))
		{
			if (k < 175) k = 175;
		}
	
		/* Various effects, depending on power. */
		if (rand_int(k) > 20)
		{
			/* Extremely frightening. */
			if (!p_ptr->no_fear)
			{
				/* Paralyze.  If has free action, max of 1 turn. */
				if ((!p_ptr->free_act) || rand_int(3) == 0)
				{
					(void)set_paralyzed(p_ptr->paralyzed +
							    (p_ptr->free_act ? 1 : rand_int(3) + 2));

					msg_print("You are paralyzed with fear!");
				}
				(void) set_afraid(p_ptr->afraid + rand_int(k));
			}
	    
			/* Use up some of the power. */
			k = 2 * k / 3;
		}
	
		if (rand_int(k) > 40)
		{
			/* Poisoning */
			pois_hit(dam);
	    
			/* Use up some of the power. */
			k = 2 * k / 3;
		}
	
		if (rand_int(k) > 80)
		{
			/* Reduce experience. */
			if (p_ptr->hold_life)
			{
				if (randint(100) > 75)
				{
					msg_print("You feel your life slipping away!");
					lose_exp(200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
				}
			}
			else
			{
				msg_print("You feel your life draining away!");
				lose_exp(200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
			/* Use up some of the power. */
			k = 2 * k / 3;
		}
	
		if (rand_int(k) > 120)
		{
			/* Disenchantment. */
			if (!p_resist_pos(P_RES_DISEN))
			{
				msg_print("You feel a force attacking the magic around you.");
				(void)apply_disenchant(0);
			}
	    
			/* Use up some of the power. */
			k = 2 * k / 3;
		}
	
		if (rand_int(k) > 120)
		{
			/* Loss of memory. */
			if (!check_save(k))
			{
				if (lose_all_info())
				{
					msg_print("The blackness invades your mind - your memories fade away.");
				}
			}
			/* Use up some of the power. */
			k = 2 * k / 3;
		}
	
		if (rand_int(k) > 160)
		{
			/* Dagger bearing the Black Breath (rare). */
			msg_print("Out of the uttermost shadow leaps a perilous blade!");
	    
			if (p_ptr->black_breath == FALSE)
			{
				/* Message. */
				msg_print("You feel the Black Breath slowly draining you of life...");
				p_ptr->black_breath = TRUE;
			}
			else
			{
				msg_print("You feel the Black Breath sucking away your lifeforce!");
				p_ptr->exp -= p_ptr->lev * 20;
				p_ptr->max_exp -= p_ptr->lev * 20;
				check_experience();
			}
		}
	
		break;
	}
      
	/* Pure confusion */
	case GF_CONFUSION:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are hit by something!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_CONFU, 1);

		if (!p_resist_pos(P_RES_CONFU))
		{
			(void)set_confused(p_ptr->confused + randint(20) + 10);
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Sound -- mostly stunning and confusing, can paralyze */
	case GF_SOUND:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are surrounded by sound.");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_SOUND, 1);

		/* Side effects of powerful sound attacks. */
		if ((dam > rand_int(30 + dam / 2)) && !p_resist_pos(P_RES_SOUND))
		{
			/* Confuse the player (a little). */
			if (!p_resist_pos(P_RES_CONFU))
			{
				k = (randint((dam > 400) ? 21 : (1 + dam / 20)));
				(void)set_confused(p_ptr->confused + k);
			}
	    
			/* Stun the player. */
			k = (randint((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(p_ptr->stun + k);
	    
			/* Sometimes, paralyze the player briefly. */
			if (!check_save(dam))
			{
				/* Warning */
				msg_print("The noise shatters your wits, and you struggle to recover.");
		
				/* Hack - directly reduce player energy. */
				p_ptr->energy -= (s16b)rand_int(dam / 2);
			}
		}
		take_hit(dam, killer);

		/* Resistance to sound - much less inventory destruction . */
		if (!p_resist_pos(P_RES_SOUND)) k = dam / 3;
		else k = dam;

		/* Blow up flasks and potions sometimes. */
		if (k > 12) inven_damage(set_cold_destroy, 
					 ((k / 13 > 30) ? 30 : k / 13));
	
		break;
	}
      
	/* Shards -- mostly cutting.  Shields may offer some protection. */
	case GF_SHARD:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		/* Test for partial shield protection. */
		if ((inventory[INVEN_ARM].k_idx) && 
		    (!p_ptr->shield_on_back) && 
		    (inventory[INVEN_ARM].ac + (check_ability(SP_SHIELD_MAST) ? 3 : 0) > 
		     rand_int(MAX_SHIELD_BASE_AC * 2)))
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
	
		if (fuzzy) msg_print("You are hit by something sharp!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_SHARD, 1);

		/* Cut the player */
		if (!p_resist_pos(P_RES_SHARD))
		{
			(void)set_cut(p_ptr->cut + dam);

		}

		/* Resistance to shards - much less inventory destruction. */
		if (p_resist_pos(P_RES_SHARD)) k = dam / 3;
		else k = dam;

		/* Blow up flasks and potions on rare occasions. */
		if (k > 19) inven_damage(set_cold_destroy, 
					 ((k / 20 > 20) ? 20 : k / 20));
	
		take_hit(dam, killer);
		break;
	}
      
	/* Inertia -- slowness */
	case GF_INERTIA:
	{
		if (fuzzy) msg_print("You are hit by something strange!");
		(void)set_slow(p_ptr->slow + rand_int(5) + (dam >= 100 ? 6 : 4));
		take_hit(dam, killer);
		break;
	}
      
	/* Gravity -- stunning and slowness. */
	case GF_GRAVITY:
	{
		(void)set_slow(p_ptr->slow + rand_int(3) + dam >= 100 ? 4 : 2);

		/* May Stun */
		if (!p_resist_pos(P_RES_SOUND))
		{
			int k = (randint((dam > 90) ? 35 : (dam / 5 + 5)));
			(void)set_stun(p_ptr->stun + k);
		}
		take_hit(dam, killer);
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Force -- mostly stun */
	case GF_FORCE:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by something!");

		/* May Stun */
		if (!p_resist_pos(P_RES_SOUND))
		{
			(void)set_stun(p_ptr->stun + randint(20));
		}
		take_hit(dam, killer);
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Water -- stun/confuse */
	case GF_WATER:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;

		if (fuzzy) msg_print("You are hit by something!");
		if ((!p_resist_pos(P_RES_SOUND)) && (rand_int(2) == 0))
		{
			(void)set_stun(p_ptr->stun + randint(5 + dam / 10));
		}
		if ((!p_resist_pos(P_RES_CONFU)) && (rand_int(2) == 0))
		{
			(void)set_confused(p_ptr->confused + rand_int(4) + 3);
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Storm -- Electricity, also acid (acidic water) and cold. */
	/* Inventory damage, stunning, confusing, . */
	case GF_STORM:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		/* Message */
		if (fuzzy) msg_print("You are enveloped in a storm!");
	
		/* Pure (wind-driven water + flying objects) damage. */
		take_hit(dam / 2, killer);
	
		/* Electrical damage. */
		if (rand_int(3) == 0)
		{
			/* Lightning strikes. */
			msg_print("You are struck by lightning!");
			elec_dam(dam, killer);
		}
		/* Lightning doesn't strike - at least not directly. */
		else elec_dam(dam / 4, killer);
	
		/* Possibly cold and/or acid damage. */
		if (rand_int(2) == 0)
		{
			if (rand_int(3) != 0)
				msg_print("You are blasted by freezing winds.");
			else msg_print("You are bombarded with hail.");
	    
			cold_dam(dam / 4, killer);
		}
		if (rand_int(2) == 0)
		{
			msg_print("You are drenched by acidic rain.");
			acid_dam(dam / 4, killer);
		}

		/* Sometimes, confuse the player. */
		if ((rand_int(2) == 0) && (!p_resist_pos(P_RES_CONFU)))
		{
			(void)set_confused(p_ptr->confused + 5 + randint(dam / 3));
		}
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Nexus -- Effects processed later, in "project_t()" */
	case GF_NEXUS:
	{
		if (fuzzy) msg_print("You are hit by something strange!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_NEXUS, 1);
	
		take_hit(dam, killer);
	
		/* Mark grid for later processing. */
		cave_info[y][x] |= (CAVE_TEMP);
	
		break;
	}
      
	/* Nether -- drain experience */
	case GF_NETHER:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are hit by something strange!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_NETHR, 1);

		/* Drain Exp */
		if (!p_resist_pos(P_RES_NETHR))
		{
			if (p_ptr->hold_life && (rand_int(100) < 75))
			{
				msg_print("You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print("You feel your life slipping away!");
				lose_exp(200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print("You feel your life draining away!");
				lose_exp(200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Chaos -- many effects.  */
	case GF_CHAOS:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;

		if (fuzzy) msg_print("You are hit by something strange!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_CHAOS, 2);

		if (!p_resist_pos(P_RES_CHAOS))
		{
			if (!p_resist_pos(P_RES_CONFU))
				(void)set_confused(p_ptr->confused + rand_int(20) + 10);
			(void)set_image(p_ptr->image + randint(10));
		}
		if (!p_resist_pos(P_RES_CHAOS) && !p_resist_pos(P_RES_NETHR))
		{
			if (p_ptr->hold_life && (rand_int(100) < 75))
			{
				msg_print("You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print("You feel your life slipping away!");
				lose_exp(500 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print("You feel your life draining away!");
				lose_exp(2000 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Disenchantment -- see above */
	case GF_DISENCHANT:
	{
		if (fuzzy) msg_print("You are hit by something strange!");
	
		/* Resist damage */
		dam -= resist_damage(dam, P_RES_DISEN, 1);

		/* Disenchant Gear */
		if (!p_resist_pos(P_RES_DISEN))
		{
			(void)apply_disenchant(dam);
			remove_player_mana(dam/5);
			apply_dispel(dam/5);
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Time -- bolt fewer effects XXX */
	case GF_TIME:
	{
		if (fuzzy) msg_print("You are hit by something strange!");

		switch (randint(10))
		{
		case 1: case 2: case 3: case 4: case 5:
		{
			msg_print("You feel life has clocked back.");
			lose_exp(100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
			break;
		}

		case 6: case 7: case 8: case 9:
		{
			switch (randint(A_MAX))
			{
			case 1: k = A_STR; act = "strong"; break;
			case 2: k = A_INT; act = "bright"; break;
			case 3: k = A_WIS; act = "wise"; break;
			case 4: k = A_DEX; act = "agile"; break;
			case 5: k = A_CON; act = "hale"; break;
			case 6: k = A_CHR; act = "beautiful"; break;
			}
	      
			msg_format("You're not as %s as you used to be...", act);
	      
			p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
			if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			p_ptr->update |= (PU_BONUS);
			break;
		}
	    
		case 10:
		{
			msg_print("You're not as powerful as you used to be...");
	      
			for (k = 0; k < A_MAX; k++)
			{
				p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
				if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			}
			p_ptr->update |= (PU_BONUS);
			break;
	      
		}
		}
		take_hit(dam, killer);
		break;
	}
      
	/* Pure damage */
	case GF_MANA:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by something!");
		take_hit(dam, killer);
		break;
	}
      
	/* Holy Orb -- Most players only take partial damage */
	case GF_HOLY_ORB:
	{
		/* Slightly affected by terrain. */
		dam += terrain_adjustment / 2;
	
		if (fuzzy) msg_print("You are hit by something!");
		if (!check_ability(SP_EVIL)) dam /= 2;
		take_hit(dam, killer);
		break;
	}
      
	/* Pure damage */
	case GF_METEOR:
	{
		/* Affected by terrain. */
		dam += terrain_adjustment;
	
		if (fuzzy) msg_print("You are hit by something!");
		take_hit(dam, killer);
		break;
	}
      
      
	/* Default */
	default:
	{
		/* No damage */
		dam = 0;
	
		break;
	}
	}
  
	/* Disturb */
	disturb(1, 0);
  
	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * Helper function for "project()" below.
 *
 * Handle movement of monsters and the player.  Handle the alteration of 
 * grids that affect damage.  -LM-
 *
 * This function only checks grids marked with the CAVE_TEMP flag, and 
 * always clears this flag.  To help creatures get out of each other's 
 * way, this function processes from outside in.
 *
 * This accomplishes two things:  A creature now cannot be damaged/blinked 
 * more than once in a single projection, if all teleport functions also 
 * clear the CAVE_TEMP flag.  Terrain now affects damage taken, and 
 * only then gets altered.
 *
 * XXX XXX -- Hack -- because the CAVE_TEMP flag may be erased by certain 
 * updates, we must be careful not to allow any of the teleport functions 
 * called by this function to ask for one.  This work well in practice, but 
 * is a definite hack.
 *
 * This function assumes that most messages have already been shown.
 */
static bool project_t(int who, int y, int x, int dam, int typ, int flg)
{
	monster_type *m_ptr = NULL;
	monster_race *r_ptr = NULL;
	monster_lore *l_ptr = NULL;
  
	cptr name = NULL;
	char m_name[80];
  
	int k, d;
  
	bool seen = FALSE;
	bool obvious = FALSE;
  
	bool affect_player = FALSE;
	bool affect_monster = FALSE;
  
	int do_dist = 0;
  
	/* Assume no note */
	cptr note = NULL;
  
	/* Only process marked grids. */
	if (!cave_info[y][x] & (CAVE_TEMP)) return (FALSE);
  
	/* Clear the cave_temp flag. */
	cave_info[y][x] &= ~(CAVE_TEMP);
  
  
	/* Projection will be affecting a player. */
	if ((flg & (PROJECT_PLAY)) && (cave_m_idx[y][x] < 0)) 
		affect_player = TRUE;
  
	/* Projection will be affecting a monster. */
	if ((flg & (PROJECT_KILL)) && (cave_m_idx[y][x] > 0))
	{
		affect_monster = TRUE;
		m_ptr = &m_list[cave_m_idx[y][x]];
		r_ptr = &r_info[m_ptr->r_idx];
		l_ptr = &l_list[m_ptr->r_idx];
      
		/* Get the monster name */
		monster_desc(m_name, m_ptr, 0);
	}
  
	if (affect_player)
	{
		obvious = TRUE;
	}
  
	if (affect_monster)
	{
		/* Obtain monster name */
		name = (r_name + r_ptr->name);
      
		/* Get monster information */
		l_ptr = &l_list[m_ptr->r_idx];
      
		/* Sight check. */
		if (m_ptr->ml) seen = TRUE;
	}
  
	/* Analyze the type */
	switch (typ)
	{
		/* Sufficiently intense cold can solidify lava. */
	case GF_COLD:
	case GF_ICE:
	{
		if (dam > randint(900) + 300)
		{
			if (cave_feat[y][x] == FEAT_LAVA)
			{
		
				/* Forget the lava */
				cave_info[y][x] &= ~(CAVE_MARK);
		
				/* Destroy the lava */
				if (randint(3) != 1) cave_set_feat(y, x, FEAT_FLOOR);
				else cave_set_feat(y, x, FEAT_RUBBLE);
			}
		}
	
		break;
	}
      
	/* Fire and plasma can create lava, evaporate water, and burn trees. */
	case GF_FIRE:
	case GF_HELLFIRE:
	case GF_PLASMA:
	{
		/* Can create lava if extremely powerful. */
		if (dam > randint(1800) + 600)
		{
			if ((cave_feat[y][x] == FEAT_FLOOR) || 
			    (cave_feat[y][x] == FEAT_RUBBLE))
			{
		
				/* Forget the floor or rubble. */
				cave_info[y][x] &= ~(CAVE_MARK);
		
				/* Make lava. */
				cave_set_feat(y, x, FEAT_LAVA);
			}
		}
	
		/* Can boil water if very strong. */
		if (cave_feat[y][x] == FEAT_WATER)
		{
			k = 0;
	    
			/* Look around for nearby water. */
			for (d = 0; d < 8; d++)
			{
				/* Extract adjacent (legal) location */
				int yy = y + ddy_ddd[d];
				int xx = x + ddx_ddd[d];
		
				/* Count the water grids. */
				if (cave_feat[yy][xx] == FEAT_WATER) k++;
			}
	    
			/* Is the fire strong enough? Large ponds are difficult 
			 * to evaporate, as Smaug found out the hard way.
			 */
			if (dam > randint(600 + k * 300) + 200)
			{
				/* Forget the water */
				cave_info[y][x] &= ~(CAVE_MARK);
		
				/* Destroy the water */
				cave_set_feat(y, x, FEAT_FLOOR);
			}
		}

		/* Can burn trees if strong. */
		if ((cave_feat[y][x] == FEAT_TREE) && (dam > randint(400) + 100))
		{
			/* Forget the tree */
			cave_info[y][x] &= ~(CAVE_MARK);
	    
			/* Destroy the tree */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
        
		break;
	}
      
	/* Gravity -- totally random blink */
	case GF_GRAVITY:
	{
		if (affect_player)
		{
			if (((p_resist_pos(P_RES_NEXUS)) || (p_ptr->ffall)) &&
			    (rand_int(2) == 0))
			{
				msg_print("You barely hold your ground.");
			}
			else
			{
				msg_print("Gravity warps around you.");
				teleport_player(6, FALSE);
			}
		}
	
		if (affect_monster)
		{
			if (r_ptr->flags4 & (RF4_BRTH_GRAV)) do_dist = 0;
			else do_dist = 10;
			if (seen) obvious = TRUE;
		}
	
		break;
	}

	/* Force -- thrust target away from caster */
	case GF_FORCE:
	{
		/* Force breathers are immune. */
		if ((affect_monster) && 
		    (r_ptr->flags4 & (RF4_BRTH_FORCE))) break;
	
		if ((affect_monster) || (affect_player))
		{
			/* Thrust monster or player away. */
			thrust_away(who, y, x, 3 + dam / 20);
	    
			/* Hack -- get new location */
			if (affect_monster)
			{
				y = m_ptr->fy;
				x = m_ptr->fx;
			}
	    
		}
	
		break;
	}
      
	/* Water/storm can make pools.  Water nearby makes it easier. */
	case GF_WATER:
	case GF_STORM:
	{
		if ((typ == GF_STORM) && (affect_player))
		{
			/* Sometimes, if no feather fall, throw the player around. */
			if ((!p_ptr->ffall) && (rand_int(3) != 0) && 
			    (rand_int(dam / 2) > p_ptr->lev))
			{
				msg_print("The wind grabs you, and whirls you around!");
				teleport_player(6, FALSE);
			}
		}
	
		if ((typ == GF_STORM) && (affect_monster))
		{
			/* Gravity breathers are immune. */
			if (r_ptr->flags4 & (RF4_BRTH_GRAV)) do_dist = 0;
	    
			/* Big monsters are immune, if the caster is a monster. */
			else if ((who > 0) && (m_ptr->maxhp) < rand_int(dam * 3)) 
			{
				do_dist = 10;
				if (seen) obvious = TRUE;
			}
		}
	
		/* Require strong attack.  Require floor. */
		if ((dam >= 60) && (cave_feat[y][x] == FEAT_FLOOR))
		{
			k = 0;
	    
			/* Look around for nearby water. */
			for (d = 0; d < 8; d++)
			{
				/* Extract adjacent (legal) location */
				int yy = y + ddy_ddd[d];
				int xx = x + ddx_ddd[d];
		
				/* Count the water grids. */
				if (cave_feat[yy][xx] == FEAT_WATER) k++;
			}
	    
			/* If enough water available, make pool. */
			if ((dam + (k * 20)) > 100 + (rand_int(400)))
			{
				/* Forget the floor */
				cave_info[y][x] &= ~(CAVE_MARK);
		
				/* Create water */
				cave_set_feat(y, x, FEAT_WATER);
			}
		}
		break;
	}
      
	/* Nexus - various effects if not resisted, mostly movement */
	case GF_NEXUS:
	{
		if (affect_player)
		{
			if (!p_resist_pos(P_RES_NEXUS))
			{
				/* Get caster */
				monster_type *n_ptr = &m_list[who];
		
				/* Various effects. */
				apply_nexus(n_ptr);
			}
		}
	
		if (affect_monster)
		{
			if (! ((r_ptr->flags4 & (RF4_BRTH_NEXUS)) ||
			       prefix(name, "Nexus")) )
			{
				do_dist = 2 + dam / 5;
			}
		}
		break;
	}
      
	/* Teleport undead (Use "dam" as "power") */
	case GF_AWAY_UNDEAD:
	{
		if (affect_monster)
		{
			/* Only affect undead */
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (seen) obvious = TRUE;
				if (seen) l_ptr->flags3 |= (RF3_UNDEAD);
				do_dist = dam;
			}
		}
		break;
	}
      
	/* Teleport evil (Use "dam" as "power") */
	case GF_AWAY_EVIL:
	{
		if (affect_monster)
		{
			/* Only affect evil */
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				if (seen) obvious = TRUE;
				if (seen) l_ptr->flags3 |= (RF3_EVIL);
				do_dist = dam;
			}
		}
		break;
	}
      
	/* Teleport monsters and player (Use "dam" as "power") */
	case GF_AWAY_ALL:
	{
		if (affect_player)
		{
			teleport_player(dam, FALSE);
		}
	
		if (affect_monster)
		{
			/* Obvious */
			if (seen) obvious = TRUE;
	    
			/* Prepare to teleport */
			do_dist = dam;
		}
		break;
	}
      
	/* All other projection types have no effect. */
	default:
	{
		return (FALSE);
	}
	}
  
	/* Handle teleportion of monster */
	if (do_dist)
	{
		/* Obvious */
		if (seen) obvious = TRUE;
      
		/* Message */
		note = " disappears!";

		/* Teleport */
		teleport_away(cave_m_idx[y][x], do_dist);
      
		/* Hack -- get new location */
		if (affect_monster)
		{
			y = m_ptr->fy;
			x = m_ptr->fx;
		}
	}
	if (affect_monster)
	{
		/* Give detailed messages if visible */
		if (note && seen)
		{
			msg_format("%^s%s", m_name, note);
		}
      
		/* Update the monster */
		update_mon(cave_m_idx[y][x], FALSE);
      
		/* Redraw the monster grid */
		lite_spot(y, x);
      
		/* Update monster recall window */
		if (p_ptr->monster_race_idx == m_ptr->r_idx)
		{
			/* Window stuff */
			p_ptr->window |= (PW_MONSTER);
		}
	}
  
	return (obvious);
}



/*
 * Generic "beam"/"bolt"/"ball" projection routine.  
 *   -BEN-, some changes by -LM-
 *
 * Input:
 *   who: Index of "source" monster (negative for the character)
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 20 = ball), or maximum
 *	  length of arc from the source.
 *   y,x: Target location (or location to travel towards)
 *   dam: Base damage to apply to monsters, terrain, objects, or player
 *   typ: Type of projection (fire, frost, dispel demons etc.)
 *   flg: Extra bit flags that control projection behavior
 *   degrees_of_arc: How wide an arc spell is (in degrees).
 *   diameter_of_source: how wide the source diameter is.
 *
 * Return:
 *   TRUE if any effects of the projection were observed, else FALSE
 *
 *
 * At present, there are five major types of projections:
 *
 * Point-effect projection:  (no PROJECT_BEAM flag, radius of zero, and either 
 *   jumps directly to target or has a single source and target grid)
 * A point-effect projection has no line of projection, and only affects one 
 *   grid.  It is used for most area-effect spells (like dispel evil) and 
 *   pinpoint strikes like the monster Holding prayer.
 * 
 * Bolt:  (no PROJECT_BEAM flag, radius of zero, has to travel from source to 
 *   target)
 * A bolt travels from source to target and affects only the final grid in its 
 *   projection path.  If given the PROJECT_STOP flag, it is stopped by any 
 *   monster or character in its path (at present, all bolts use this flag).
 *
 * Beam:  (PROJECT_BEAM)
 * A beam travels from source to target, affecting all grids passed through 
 *   with full damage.  It is never stopped by monsters in its path.  Beams 
 *   may never be combined with any other projection type.
 *
 * Ball:  (positive radius, unless the PROJECT_ARC flag is set)
 * A ball travels from source towards the target, and always explodes.  Unless 
 *   specified, it does not affect wall grids, but otherwise affects any grids 
 *   in LOS from the center of the explosion.
 * If used with a direction, a ball will explode on the first occupied grid in 
 *   its path.  If given a target, it will explode on that target.  If a 
 *   wall is in the way, it will explode against the wall.  If a ball reaches 
 *   MAX_RANGE without hitting anything or reaching its target, it will 
 *   explode at that point.
 *
 * Arc:  (positive radius, with the PROJECT_ARC flag set)
 * An arc is a portion of a source-centered ball that explodes outwards 
 *   towards the target grid.  Like a ball, it affects all non-wall grids in 
 *   LOS of the source in the explosion area.  The width of arc spells is con-
 *   trolled by degrees_of_arc.
 * An arc is created by rejecting all grids that form the endpoints of lines 
 *   whose angular difference (in degrees) from the centerline of the arc is 
 *   greater than one-half the input "degrees_of_arc".  See the table "get_
 *   angle_to_grid" in "util.c" for more information.
 * Note:  An arc with a value for degrees_of_arc of zero is actually a beam of
 *   defined length.
 *
 * Projections that effect all monsters in LOS are handled through the use 
 *   of "project_hack()", which applies a single-grid projection to individual 
 *   monsters.  Projections that light up rooms or effect all monsters on the 
 *   level are more efficiently handled through special functions.
 *
 *
 * Variations:
 *
 * PROJECT_STOP forces a path of projection to stop at the first occupied grid 
 *   it hits.  This is used with bolts, and also by ball spells travelling in 
 *   a specific direction rather than towards a target.
 *
 * PROJECT_THRU allows a path of projection towards a target to continue 
 *   past that target.  It also allows a spell to affect wall grids adjacent 
 *   to a grid in LOS of the center of the explosion.
 * 
 * PROJECT_JUMP allows a projection to immediately set the source of the pro-
 *   jection to the target.  This is used for all area effect spells (like 
 *   dispel evil), and can also be used for bombardments.
 * 
 * PROJECT_HIDE erases all graphical effects, making the projection invisible.
 *
 * PROJECT_GRID allows projections to affect terrain features.
 *
 * PROJECT_ITEM allows projections to affect objects on the ground.
 *
 * PROJECT_KILL allows projections to affect monsters.
 *
 * PROJECT_PLAY allows projections to affect the player.
 *
 * degrees_of_arc controls the width of arc spells.  With a value for 
 *   degrees_of_arc of zero, arcs act like beams of defined length.
 *
 * diameter_of_source controls how quickly explosions lose strength with dis-
 *   tance from the target.  Most ball spells have a source diameter of 10, 
 *   which means that they do 1/2 damage at range 1, 1/3 damage at range 2, 
 *   and so on.   Caster-centered balls usually have a source diameter of 20, 
 *   which allows them to do full damage to all adjacent grids.   Arcs have 
 *   source diameters ranging up to 20, which allows the spell designer to 
 *   fine-tune how quickly a breath loses strength outwards from the breather.
 *   It is expected, but not required, that wide arcs lose strength more 
 *   quickly over distance.
 *
 *
 * Implementation notes:
 *
 * If the source grid is not the same as the target, we project along the path
 *   between them.  Bolts stop if they hit anything, beams stop if they hit a
 *   wall, and balls and arcs may exhibit either bahavior.  When they reach
 *   the final grid in the path, balls and arcs explode.  We do not allow beams
 *   to be combined with explosions.
 * Balls affect all floor grids in LOS (optionally, also wall grids adjacent
 *   to a grid in LOS) within their radius.  Arcs do the same, but only within
 *   their cone of projection.
 * Because affected grids are only scanned once, and it is really helpful to 
 *   have explosions that travel outwards from the source, they are sorted by 
 *   distance.  For each distance, an adjusted damage is calculated.
 * In successive passes, the code then displays explosion graphics, erases 
 *   these graphics, marks terrain for possible later changes, affects 
 *   objects, monsters, the character, and finally changes features and 
 *   teleports monsters and characters in marked grids.
 * 
 *
 * Usage and graphics notes:
 *
 * If the option "fresh_before" is on, or the delay factor is anything other 
 * than zero, bolt and explosion pictures will be momentarily shown on screen.
 *
 * Only 256 grids can be affected per projection, limiting the effective 
 * radius of standard ball attacks to nine units (diameter nineteen).  Arcs 
 * can have larger radii; an arc capable of going out to range 20 should not 
 * be wider than 70 degrees.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters on 
 * both sides of a wall. 
 *
 * Note that for consistency, we pretend that the bolt actually takes time
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 *
 * Hack -- when only a single monster is affected, we automatically track
 * (and recall) that monster, unless "PROJECT_JUMP" is used.
 *
 * Note that we must call "handle_stuff()" after affecting terrain features
 * in the blast radius, in case the illumination of the grid was changed,
 * and "update_view()" and "update_monsters()" need to be called.
 */
bool project(int who, int rad, int y, int x, int dam, int typ, int flg, 
	     int degrees_of_arc, byte diameter_of_source)
{
	int py = p_ptr->py;
	int px = p_ptr->px;
  
	int i, j, k, dist;
  
	u32b dam_temp;
  
	int y0, x0;
	int y1, x1;
	int y2, x2;
  
	int n1y = 0;
	int n1x = 0;
  
	int msec = op_ptr->delay_factor * op_ptr->delay_factor;
  
	/* Assume the player sees nothing */
	bool notice = FALSE;
  
	/* Assume the player has seen nothing */
	bool visual = FALSE;
  
	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;
  
	/* Is the player blind? */
	bool blind = (p_ptr->blind ? TRUE : FALSE);
  
	/* Number of grids in the "path" */
	int path_n = 0;
  
	/* Actual grids in the "path" */
	u16b path_g[512];
  
	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;
  
	/* Coordinates of the affected grids */
	byte gx[256], gy[256];
  
	/* Distance to each of the affected grids. */
	byte gd[256];
  
	/* Precalculated damage values for each distance. */
#ifdef _WIN32_WCE
	int *dam_at_dist = malloc((MAX_RANGE + 1) * sizeof(*dam_at_dist));
#else
	int dam_at_dist[MAX_RANGE+1];
#endif
  
	/* Hack -- Flush any pending output */
	handle_stuff();
  
	/* Hack -- Jump to target */
	if (flg & (PROJECT_JUMP))
	{
		y1 = y;
		x1 = x;
      
		/* Clear the flag */
		flg &= ~(PROJECT_JUMP);
	}
  
	/* Start at player */
	else if (who < 0)
	{
		y1 = py;
		x1 = px;
	}
  
	/* Start at monster */
	else if (who > 0)
	{
		y1 = m_list[who].fy;
		x1 = m_list[who].fx;
	}
  
	/* Oops */
	else
	{
		y1 = y;
		x1 = x;
	}
  
	/* Default destination */
	y2 = y;
	x2 = x;
  
	/* Default center of explosion (if any) */
	y0 = y1;
	x0 = x1;
  
	/*
	 * An arc spell with no width and a non-zero radius is actually a 
	 * beam of defined length.  Mark it as such.
	 */
	if ((flg & (PROJECT_ARC)) && (degrees_of_arc == 0) && (rad != 0))
	{
		/* No longer an arc */
		flg &= ~(PROJECT_ARC);
      
		/* Now considered a beam */
		flg |= (PROJECT_BEAM);
		flg |= (PROJECT_THRU);
	}
  
  
	/* If a single grid is both source and destination, store it. */
	if ((x1 == x2) && (y1 == y2))
	{
		gy[grids] = y;
		gx[grids] = x;
		gd[grids] = 0;
		grids++;
	}
  
	/* Otherwise, travel along the projection path. */
	else
	{
		/* Calculate the projection path */
		path_n = project_path(path_g, MAX_RANGE, y1, x1, y2, x2, flg);
      
		/* Start from caster */
		y = y1;
		x = x1;
      
		/* Some beams have limited length. */
		if (flg & (PROJECT_BEAM))
		{
			/* Use length limit, if any is given. */
			if ((rad > 0) && (rad < path_n)) path_n = rad;
		}
      
      
		/* Project along the path (except for arcs) */
		if (!(flg & (PROJECT_ARC))) for (i = 0; i < path_n; ++i)
					    {
						    int oy = y;
						    int ox = x;
	  
						    int ny = GRID_Y(path_g[i]);
						    int nx = GRID_X(path_g[i]);
	  
	  
						    /* Hack -- Balls explode before reaching walls. */
						    if (!cave_passable_bold(ny, nx) && (rad > 0)) break;
	  
						    /* Advance */
						    y = ny;
						    x = nx;
	  
						    /* If a beam, collect all grids in the path. */
						    if (flg & (PROJECT_BEAM))
						    {
							    gy[grids] = y;
							    gx[grids] = x;
							    gd[grids] = 0;
							    grids++;
						    }
	  
						    /* Otherwise, collect only the final grid in the path. */
						    else if (i == path_n - 1)
						    {
							    gy[grids] = y;
							    gx[grids] = x;
							    gd[grids] = 0;
							    grids++;
						    }
	  
						    /* Only do visuals if requested and within range limit. */
						    if (!blind && !(flg & (PROJECT_HIDE)))
						    {
	      
							    /* Only do visuals if the player can "see" the bolt */
							    if (panel_contains(y, x) && player_has_los_bold(y, x))
							    {
								    u16b p;
		  
								    byte a;
								    char c;
		  
								    /* Obtain the bolt pict */
								    p = bolt_pict(oy, ox, y, x, typ);
		  
								    /* Extract attr/char */
								    a = PICT_A(p);
								    c = PICT_C(p);
		  
								    /* Visual effects */
								    print_rel(c, a, y, x);
								    move_cursor_relative(y, x);
								    if ((op_ptr->delay_factor) || (fresh_before)) Term_fresh();
								    Term_xtra(TERM_XTRA_DELAY, msec);
								    lite_spot(y, x);
								    if ((op_ptr->delay_factor) || (fresh_before)) Term_fresh();
		  
								    /* Display "beam" grids */
								    if (flg & (PROJECT_BEAM))
								    {
		      
									    /* Obtain the explosion pict */
									    p = bolt_pict(y, x, y, x, typ);
		      
									    /* Extract attr/char */
									    a = PICT_A(p);
									    c = PICT_C(p);
		      
									    /* Visual effects */
									    print_rel(c, a, y, x);
								    }
		  
								    /* Hack -- Activate delay */
								    visual = TRUE;
							    }
	      
							    /* Hack -- delay anyway for consistency */
							    else if (visual)
							    {
								    /* Delay for consistency */
								    Term_xtra(TERM_XTRA_DELAY, msec);
							    }
						    }
					    }
	}
  
	/* Save the "blast epicenter" */
	y0 = y;
	x0 = x;
  
	/* Beams have already stored all the grids they will affect. */
	if (flg & (PROJECT_BEAM))
	{
		/* No special actions */
	}
  
	/* 
	 * All non-beam projections with a positive radius explode in some way.
	 */
	else if (rad > 0)
	{
      
		/* Some projection types always PROJECT_THRU. */
		if ((typ == GF_KILL_WALL) || (typ == GF_KILL_DOOR))
		{
			flg |= (PROJECT_THRU);
		}
      
		/* Pre-calculate some things for arcs. */
		if ((flg & (PROJECT_ARC)) && (path_n != 0))
		{
			/* Explosion centers on the caster. */
			y0 = y1;
			x0 = x1;
	  
			/* The radius of arcs cannot be more than 20 */
			if (rad > 20) rad = 20;
	  
			/* Ensure legal table access */
			if (path_n < 21) i = path_n - 1;
			else i = 20;
	  
			/* Reorient the grid forming the end of the arc's centerline. */
			n1y = GRID_Y(path_g[i]) - y0 + 20;
			n1x = GRID_X(path_g[i]) - x0 + 20;
		}
      
		/* 
		 * If the center of the explosion hasn't been 
		 * saved already, save it now. 
		 */
		if (grids == 0)
		{
			gy[grids] = y0;
			gx[grids] = x0;
			gd[grids] = 0;
			grids++;
		}
      
		/* 
		 * Scan every grid that might possibly 
		 * be in the blast radius. 
		 */	
		for (y = y0 - rad; y <= y0 + rad; y++)
		{
			for (x = x0 - rad; x <= x0 + rad; x++)
			{
				/* Center grid has already been stored. */
				if ((y == y0) && (x == x0)) continue;
	      
				/* Precaution: Stay within area limit. */
				if (grids >= 255) break;
	      
				/* Ignore "illegal" locations */
				if (!in_bounds(y, x)) continue;
	      
				/* Some explosions are allowed to affect one layer of walls */
				/* All exposions can affect one layer of rubble or trees -BR- */
				if ((flg & (PROJECT_THRU)) || (cave_passable_bold(y, x)))
				{
					/* If this is a wall grid, ... */
					if (!cave_floor_bold(y, x))
					{
						/* Check neighbors */
						for (i = 0, k = 0; i < 8; i++)
						{
							int yy = y + ddy_ddd[i];
							int xx = x + ddx_ddd[i];
			  
							if (los(y0, x0, yy, xx))
							{
								k++;
								break;
							}
						}
		      
						/* Require at least one adjacent grid in LOS. */
						if (!k) continue;
					}
				}
	      
				/* Most explosions are immediately stopped by walls. */
				else if (!cave_floor_bold(y, x)) continue;
	      
				/* Must be within maximum distance. */
				dist = (distance(y0, x0, y, x));
				if (dist > rad) continue;
	      
	      
				/* If not an arc, accept all grids in LOS. */
				if (!(flg & (PROJECT_ARC)))
				{
					if (los(y0, x0, y, x))
					{
						gy[grids] = y;
						gx[grids] = x;
						gd[grids] = dist;
						grids++;
					}
				}
	      
				/* Use angle comparison to delineate an arc. */
				else
				{
					int n2y, n2x, tmp, rotate, diff;
		  
					/* Reorient current grid for table access. */
					n2y = y - y1 + 20;
					n2x = x - x1 + 20;
		  
					/*
					 * Find the angular difference (/2) between 
					 * the lines to the end of the arc's center-
					 * line and to the current grid.
					 */
					rotate = 90 - 
						get_angle_to_grid[n1y][n1x];
					tmp = ABS(get_angle_to_grid[n2y][n2x] + 
						  rotate) % 180;
					diff = ABS(90 - tmp);
		  
					/*
					 * If difference is not greater then that 
					 * allowed, and the grid is in LOS, accept it.
					 */
					if (diff < (degrees_of_arc + 6) / 4)
					{
						if (los(y0, x0, y, x))
						{
							gy[grids] = y;
							gx[grids] = x;
							gd[grids] = dist;
							grids++;
						}
					}
				}
			}
		}
	}
  
	/* Calculate and store the actual damage at each distance. */
	for (i = 0; i <= MAX_RANGE; i++)
	{
		/* No damage outside the radius. */
		if (i > rad) dam_temp = 0;
      
		/* Standard damage calc. for 10' source diameters, or at origin. */
		else if ((!diameter_of_source) || (i == 0))
		{
			dam_temp = (dam + i) / (i + 1);
		}
      
		/* If a particular diameter for the source of the explosion's 
		 * energy is given, calculate an adjusted damage.
		 */
		else
		{
			dam_temp = (diameter_of_source * dam) / ((i + 1) * 10);
			if (dam_temp > (u32b)dam) dam_temp = dam;
		}
      
		/* Store it. */
		dam_at_dist[i] = dam_temp;
	}
  
  
	/* Sort the blast grids by distance, starting at the origin. */
	for (i = 0, k = 0; i < rad; i++)
	{
		int tmp_y, tmp_x, tmp_d;
      
		/* Collect all the grids of a given distance together. */
		for (j = k; j < grids; j++)
		{
			if (gd[j] == i)
			{
				tmp_y = gy[k];
				tmp_x = gx[k];
				tmp_d = gd[k];
	      
				gy[k] = gy[j];
				gx[k] = gx[j];
				gd[k] = gd[j];
	      
				gy[j] = tmp_y;
				gx[j] = tmp_x;
				gd[j] = tmp_d;
	      
				/* Write to next slot */
				k++;
			}
		}
	}
  
	/* Display the blast area if allowed. */
	if (!blind && !(flg & (PROJECT_HIDE)))
	{
		/* Do the blast from inside out */
		for (i = 0; i < grids; i++)
		{
			/* Extract the location */
			y = gy[i];
			x = gx[i];
	  
			/* Only do visuals if the player can "see" the blast */
			if (panel_contains(y, x) && player_has_los_bold(y, x))
			{
				u16b p;
	      
				byte a;
				char c;
	      
				drawn = TRUE;
	      
				/* Obtain the explosion pict */
				p = bolt_pict(y, x, y, x, typ);
	      
				/* Extract attr/char */
				a = PICT_A(p);
				c = PICT_C(p);
	      
				/* Visual effects -- Display */
				print_rel(c, a, y, x);
			}
	  
			/* Hack -- center the cursor */
			move_cursor_relative(y0, x0);
	  
			/* New radius is about to be drawn */
			if ((i == grids) || (gd[i + 1] > gd[i]))
			{
				/* Flush each radius seperately */
				if ((op_ptr->delay_factor) || (fresh_before)) Term_fresh();
	      
				/* Delay (efficiently) */
				if (visual || drawn)
				{
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}
		}
      
		/* Flush the erasing */
		if (drawn)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];
	      
				/* Hack -- Erase if needed */
				if (panel_contains(y, x) && player_has_los_bold(y, x))
				{
					lite_spot(y, x);
				}
			}
	  
			/* Hack -- center the cursor */
			move_cursor_relative(y0, x0);
	  
			/* Flush the explosion */
			if ((op_ptr->delay_factor) || (fresh_before)) Term_fresh();
		}
	}
  
  
	/* Check features */
	if (flg & (PROJECT_GRID))
	{
		/* Scan for features */
		for (i = 0; i < grids; i++)
		{
			/* Get the grid location */
			y = gy[i];
			x = gx[i];
	  
			/* Affect the feature in that grid */
			if (project_f(who, y, x, gd[i], dam_at_dist[gd[i]], typ)) 
				notice = TRUE;
		}
	}
  
	/* Check objects */
	if (flg & (PROJECT_ITEM))
	{
		/* Scan for objects */
		for (i = 0; i < grids; i++)
		{
			/* Get the grid location */
			y = gy[i];
			x = gx[i];
	  
			/* Affect the object in the grid */
			if (project_o(who, y, x, dam_at_dist[gd[i]], typ)) 
				notice = TRUE;
		}
	}
  
	/* Check monsters */
	if (flg & (PROJECT_KILL))
	{
		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
      
		/* Scan for monsters */
		for (i = 0; i < grids; i++)
		{
			/* Get the grid location */
			y = gy[i];
			x = gx[i];
	  
			/* Affect the monster in the grid */
			if (project_m(who, y, x, dam_at_dist[gd[i]], typ, flg)) 
				notice = TRUE;
		}
      
		/* Player affected one monster (without "jumping") */
		if ((who < 0) && (project_m_n == 1) && !(flg & (PROJECT_JUMP)))
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;
	  
			/* Track if possible */
			if (cave_m_idx[y][x] > 0)
			{
				monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
	      
				/* Hack -- auto-recall */
				if (m_ptr->ml) monster_race_track(m_ptr->r_idx);
	      
				/* Hack - auto-track */
				if (m_ptr->ml) health_track(cave_m_idx[y][x]);
			}
		}
	}
  
	/* Check player */
	if (flg & (PROJECT_PLAY))
	{
		/* Scan for player */
		for (i = 0; i < grids; i++)
		{
			/* Get the grid location */
			y = gy[i];
			x = gx[i];
	  
			/* Affect the player */
			if (project_p(who, rad, y, x, dam_at_dist[gd[i]], typ)) 
				notice = TRUE;
		}
	}
  
	/* Teleport monsters and player around, alter certain features. */
	for (i = 0; i < grids; i++)
	{
		/* Get the grid location */
		y = gy[i];
		x = gx[i];
      
		/* Grid must be marked. */
		if (!(cave_info[y][x] & (CAVE_TEMP))) continue;
      
		/* Affect marked grid */
		if (project_t(who, y, x, dam_at_dist[gd[i]], typ, flg)) 
			notice = TRUE;
	}
  
	/* Update stuff if needed */
	if (p_ptr->update) update_stuff();
  
#ifdef _WIN32_WCE
	free(dam_at_dist);
#endif
  
	/* Return "something was noticed" */
	return (notice);
}
