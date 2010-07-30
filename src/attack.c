/* File: attack.c */

/*
 * The non-magical attack code.
 *
 * Hit chance, critical hits in melee and when shooting/throwing, calculate
 * ego multiplier, calculate Deadliness adjustment.  Melee attacks (and shield
 * bashes).  Chance of object breakage, the shooting code, the throwing code.
 *
 * Copyright (c) 2001
 * Leon Marrick, Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


#include "angband.h"


/*
 * Determine if the player hits a monster (non-magical combat).
 *
 * 5% of all attacks are guaranteed to hit, and another 5% to miss.
 * The remaining 90% compare attack chance to armour class.
 */
static bool test_hit_combat(int chance, int ac, int visible)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Invisible monsters are harder to hit */
	if (!visible) chance = chance / 2;

	/* Power competes against armor */
	if ((chance > 0) && (rand_int(chance) >= ac)) return (TRUE);

	/* Assume miss */
	return (FALSE);
}



/*
 * Player attempts to inflict the Black Breath on a monster in combat. -BR-
 */
void hit_monster_black_breath(int power, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	char m_name[80];

	/* Extract monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* The undead and Morgul casters are immune */
	if ((r_ptr->flags3 & (RF3_UNDEAD)) || (r_ptr->flags2 & (RF2_MORGUL_MAGIC)))
	{
		if (m_ptr->ml) /* control for visibility */
		{
			l_ptr->flags3 |= (RF3_UNDEAD);
			l_ptr->flags2 |= (RF2_MORGUL_MAGIC);
		}

		msg_format("%^s is immune!", m_name);
	}
	/* All other monsters get a saving throw. */
	else if (rand_int(power) < r_ptr->level + randint(10))
	{
		msg_format("%^s wards off your deadly attack.", m_name);
	}
	/* Tasting some of their own medicine... */
	else
	{
		m_ptr->black_breath = TRUE;
		message_format(MSG_HIT, 0, "%^s is stricken with the Black Breath!", m_name);
	}
}



/*
 * Player attempts to confuse a monster in combat. -BR-
 *
 * Not currently used for confusion from shield bashes.
 */
void hit_monster_confuse(int power, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	char m_name[80];

	/* Extract monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Check for confusion */
	if (r_ptr->flags3 & (RF3_NO_CONF))
	{
		if (m_ptr->ml)
		{
			l_ptr->flags3 |= (RF3_NO_CONF);
		}

		msg_format("%^s is unaffected.", m_name);
	}
	else if (rand_int(power) < r_ptr->level + randint(10))
	{
		msg_format("%^s is unaffected.", m_name);
	}
	else if (m_ptr->confused > 0)
	{
		if (m_ptr->ml) message_format(MSG_HIT, 0, "%^s appears more confused.", m_name);
		else message_format(MSG_HIT, 0, "%^s sounds more confused.", m_name);
		m_ptr->confused += 4 + rand_int(p_ptr->lev) / 12;
	}
	else
	{
		if (m_ptr->ml) message_format(MSG_HIT, 0, "%^s appears confused.", m_name);
		else message_format(MSG_HIT, 0, "%^s sounds confused.", m_name);
		m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5;
	}
}



/*
 * Player attempts to slow a monster in combat. -BR-
 */
void hit_monster_slow(int power, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	char m_name[80];

	/* Extract monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Check for slow */
	if (rand_int(power) < r_ptr->level + randint(10))
	{
		msg_format("%^s is unaffected.", m_name);
	}
	else if (m_ptr->mspeed > 60)
	{
		if (r_ptr->speed - m_ptr->mspeed <= 10)
		{
			m_ptr->mspeed -= 10;
			if (m_ptr->ml) message_format(MSG_HIT, 0, "%^s starts moving slower.", m_name);
			else message_format(MSG_HIT, 0, "%^s sounds slower.", m_name);
		}
	}
}



/*
 * Calculates parameters needed to spread_target, which randomly
 * retargets within about +/- 8 degrees of the original target.
 *
 * y, x are source position
 * ty, tx are the actual target
 *
 * spread_y_cen, spread_x_cen are the centers of the random targeting
 * zone.  These are coordinates on a line from (y,x) to (ty,tx) at a
 * distance of 20 from (y,x)
 *
 * spread_roll is the magnitude of the randomness in the targeting
 * roll needed to yield the 8 deg spread.
 *
 * spread_sign_y, spread_sign_x give a direction roughly perpendicular
 * to the line of fire from (y,x) to (ty,tx); the direction on the
 * perpendicular is random.
 */
void spread_target_prepare(int y, int x, int ty, int tx,
			   int *spread_y_cen, int *spread_x_cen, int *spread_roll, int *spread_sign_y, int *spread_sign_x)
{
	int spread_dis;
	int y_rel, x_rel;

	/* Find distance to current target */
	spread_dis = distance(y, x, ty, tx);

	/* Special Case - Self targetted */
	/* Target semi-randomly.  Maybe later change this to a true random spread from the source */
	if (spread_dis == 0)
	{
		*spread_y_cen = y;
		*spread_x_cen = x;
		*spread_roll = 10;
		*spread_sign_y = rand_int(3) -1;
		*spread_sign_x = rand_int(3) -1;
		return;
	}

	/* relative coordinates */
	y_rel = (ty - y);
	x_rel = (tx - x);

	/* Scale out to range 20 for center of spread */
	*spread_y_cen = (y_rel * 20 / spread_dis) + y;
	*spread_x_cen = (x_rel * 20 / spread_dis) + x;

	/*
	 * Seed the scatter direction to something
	 * roughly perpendicular to the path of
	 * fire.
	 */
	/* Shooting vertical - spread horizontal */
	if ((x_rel == 0) || (abs(y_rel / 2) > abs(x_rel)))
	{
		*spread_sign_y = 0;
		if (rand_int(2) == 0) *spread_sign_x = 1;
		else *spread_sign_x = -1;
		*spread_roll = 4;
	}
	/* Shooting horizontal - spread vertical */
	else if ((y_rel == 0) || (abs(x_rel / 2) > abs(y_rel)))
	{
		if (rand_int(2) == 0) *spread_sign_y = 1;
		else *spread_sign_y = -1;
		*spread_sign_x = 0;
		*spread_roll = 4;
	}
	/* Shooting diagonal - spread the other diagonal */
	else if ((x_rel / y_rel) > 0)
	{
		if (rand_int(2) == 0) *spread_sign_y = 1;
		else *spread_sign_y = -1;
		*spread_sign_x = -1 * *spread_sign_y;
		*spread_roll = 3;
	}
	/* Shooting diagonal - spread the other diagonal */
	else
	{
		if (rand_int(2) == 0) *spread_sign_y = 1;
		else *spread_sign_y = -1;
		*spread_sign_x = 1 * *spread_sign_y;
		*spread_roll = 3;
	}
}



/*
 * Using parameters Calculated in spread_target_prepare, randomly
 * retargets within about +/- 8 degrees of the original target.
 *
 * spread_y_cen, spread_x_cen are the centers of the random targeting
 * zone.  These are coordinates on a line from (y,x) to (ty,tx) at a
 * distance of 20 from (y,x)
 *
 * spread_roll is the magnitude of the randomness in the targeting
 * roll needed to yield the 8 deg spread.
 *
 * spread_sign_y, spread_sign_x give a direction roughly perpendicular
 * to the line of fire from (y,x) to (ty,tx); the direction on the
 * perpendicular is random.
 *
 * new_y, new_x are the modified target position.
 *
 * If "flip" is true, the direction indicated by spread_sign_* is
 * reversed to ensure relatively even spreading.
 */
void spread_target(int spread_y_cen, int spread_x_cen, int spread_roll, int *spread_sign_y, int *spread_sign_x,
	      bool flip, int *new_y, int *new_x)
{
	int spread_d;

	/* Spread perpendicular to line of fire */
	spread_d = rand_int(spread_roll);
	*new_y = spread_y_cen + (spread_d * *spread_sign_y);
	*new_x = spread_x_cen + (spread_d * *spread_sign_x);

	/* Reverse scatter direction for next time */
	if (flip)
	{
		*spread_sign_y *= -1;
		*spread_sign_x *= -1;
	}
}


/*
 * Calculation of critical hits by the player in hand-to-hand combat.
 * -LM-
 *
 * Critical hits represent the ability of a skilled fighter to make his
 * weapon go where he wants it to.  This increased control is represented
 * by adding damage dice; this makes the attack both more powerful and
 * more reliable.
 *
 * Weapons with few damage dice are less reliable in combat (because the
 * damage they do is more variable).  Their great saving grace is precisely
 * this variablity; criticals benefit them more.
 *
 * Vorpal blades/weapons of concussion get lots of criticals.
 * -- Not currently used in Oangband -BR-
 *
 * This function is responsible for the basic melee combat messages, which
 * vary according to the quality of the hit.  A distinction is made between
 * visible and invisible monsters.
 */
static sint critical_melee(int chance, int sleeping_bonus, bool visible,
	char m_name[], const object_type *o_ptr)
{
	bool vorpal = FALSE;
	bool armsman = FALSE;

	/* Extract melee power. */
	int power = (chance + sleeping_bonus);

	/* Assume no added dice */
	int add_dice = 0;

	/* Special quality (vorpal blades/weapons of concussion) */
	/*if (o_ptr->flags1 & (TR1_VORPAL))
	{
		power *= 2;            (this may be a little too much)
		vorpal = TRUE;
	}*/

	/* Specialty Ability */
	if ((visible) && (check_ability(SP_ARMSMAN)) && (rand_int(6) == 0))
	{
		armsman = TRUE;
	}

	/* Test for critical hit. */
	if ((armsman) || (randint(power + 240) <= power))
	{
		/* Determine level of critical hit. */
		if      (rand_int(40) == 0) add_dice = 5;
		else if (rand_int(12) == 0) add_dice = 4;
		else if (rand_int(3)  == 0) add_dice = 3;
		else                        add_dice = 2;

		/* Encourage the player to beat on sleeping monsters. */
		if ((sleeping_bonus) && (visible))
		{
			/* More "interesting" messages if we get a seriously good hit. */
			if ((add_dice >= 4) && (check_ability(SP_BACKSTAB)))
			{
				message(MSG_HIT, 0, "You ruthlessly sneak attack!");
			}

			/* Standard "wakeup call". */
			else
			{
				message(MSG_HIT, 0, "You rudely awaken the monster.");
			}
		}

                /* Credit where credit is due - but not if we already have a special message */
                else if (armsman) message(MSG_HIT, 0, "Armsman hit!");

		/* Print special messages if monster is visible. */
		if (visible)
		{
			/*
			 * Messages depend on quality of critical hit.  A distinction
			 * is often made between edged and blunt weapons.  Unfortu-
			 * nately, whips sometimes display rather odd messages...
			 */
			if (add_dice <= 2)
			{
				message_format(MSG_HIT, 0, "You strike %s.", m_name);
			}

			else if (add_dice == 3)
			{
				if ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))
				{
					message_format(MSG_HIT, 0, "You hack at %s.", m_name);
				}
				else
				{
					message_format(MSG_HIT, 0, "You pound %s.", m_name);
				}
			}

			else if (add_dice == 4)
			{
				if ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))
				{
					if (vorpal)
					{
						message_format(MSG_HIT, 0,
							"Your vorpal blade goes snicker-snack!", m_name);
					}
					else
					{
						message_format(MSG_HIT, 0, "You slice into %s.", m_name);
					}
				}
				else
				{
					message_format(MSG_HIT, 0, "You bludgeon %s.", m_name);
				}
			}

			else if (add_dice >= 5)
			{
				if ((vorpal) && ((o_ptr->tval == TV_SWORD) ||
					(o_ptr->tval == TV_POLEARM)))
				{
					message_format(MSG_HIT, 0,
						"Your vorpal blade goes snicker-snack!", m_name);
				}
				else
				{
					message_format(MSG_HIT, 0, "You *smite* %s!", m_name);
				}
			}
		}
	}

	/* If the blow is not a critical hit, then the default message is shown. */
	else if (visible)
	{
		message_format(MSG_HIT, 0, "You hit %s.", m_name);
	}

	/* Hits on non-visible monsters always generate the same message. */
	if (!visible)
	{
		message(MSG_HIT, 0, "You hit something.");
	}

	/* Return the number of damage dice to add. */
	return (add_dice);
}



/*
 * Calculation of critical hits for objects fired or thrown by the player.
 * -LM-
 *
 * Critical shots represent the ability of a skilled fighter to make his
 * missiles go where he wants them to.  This increased control is
 * represented by adding damage dice; this makes the attack both more
 * powerful and more reliable.  Because ammo normally rolls one die,
 * critical hits are very powerful in archery.
 *
 * This function is responsible for the basic archery and throwing combat
 * messages, which vary according to the quality of the hit.  A distinction
 * is made between visible and invisible monsters.
 */
static sint critical_shot(int chance, int sleeping_bonus, bool thrown_weapon,
	bool visible, char m_name[], object_type *o_ptr)
{
	char o_name[80];

	bool marksman = FALSE;

	/* Extract missile power. */
	int power = (chance + sleeping_bonus);

	/* Assume no added dice */
	int add_dice = 0;

	/* Throwing weapons get lots of critical hits. */
	if (thrown_weapon) power = power * 3 / 2;

	/* Special quality (vorpal blades/weapons of concussion) */
	/*if (o_ptr->flags1 & (TR1_VORPAL))
	{
		power *= 2;            (this may be a little too much)
		vorpal = TRUE;
	}*/

	/* Specialty Ability */
	if ((visible) && (check_ability(SP_MARKSMAN)) && (rand_int(6) == 0))
	{
		marksman = TRUE;
	}

	/* Obtain an item description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Test for critical hit. */
	if (marksman || (randint(power + 360) <= power))
	{
		/* Determine level of critical hit. */
		if (rand_int(50) == 0) add_dice = 3;
		else if (rand_int(10) == 0) add_dice = 2;
		else add_dice = 1;

		/* Encourage the player to throw and shoot things at sleeping monsters. */
		if ((sleeping_bonus) && (visible))
		{
			if ((thrown_weapon) && (add_dice >= 2))
			{
				message(MSG_HIT, 0, "Assassin strike!");
			}

			else
			{
				message(MSG_HIT, 0, "You rudely awaken the monster.");
			}
		}

                /* Credit where credit is due - but not if we already have a special message */
                else if (marksman) message(MSG_HIT, 0, "Marksmanship!");

		/* Print special messages if monster is visible. */
		if (visible)
		{
			/* Messages depend on quality of critical hit. */
			if (add_dice == 1)
			{
				message_format(MSG_HIT, 0, "The %s penetrates %s.", o_name, m_name);
			}

			else if (add_dice == 2)
			{
				message_format(MSG_HIT, 0, "The %s drives into %s.", o_name, m_name);
			}

			else if (add_dice >= 3)
			{
				message_format(MSG_HIT, 0, "The %s transpierces %s!", o_name, m_name);
			}
		}
	}

	/* If the shot is not a critical hit, then the default message is shown. */
	else if (visible)
	{
		message_format(MSG_HIT, 0, "The %s hits %s.", o_name, m_name);
	}

	/* Hits on non-visible monsters always generate the same message. */
	if (!visible)
	{
		message_format(MSG_HIT, 0, "The %s finds a mark.", o_name);
	}

	/* Return the number of damage dice to add. */
	return (add_dice);
}


/*
 * Handle all special adjustments to the damage done by a non-magical attack.
 *
 * At present, only weapons (including digging tools) and ammo have their
 * damage adjusted.  Flasks of oil could do fire damage, but they currently
 * don't.
 *
 * Most slays are x2, except Slay Animal (x1.7), and Slay Evil (x1.5).
 * Weapons of *slaying* now get a larger bonus.  All brands are x1.7.
 * All slays and brands also add to the base damage.
 *
 * Examples:  (assuming monster is an orc)
 * Dagger of Slay Orc:    1d4 * 2.0 + 10:   Average: 15 damage.
 * Dagger of *Slay Orc*:  1d4 * 2.5 + 15:   Average: just over 21 damage.
 *
 * Players may have temporary magic branding.  Paladins do not get to apply
 * temporary brands to missiles.  A nasty hack, but necessary. -LM-
 */
static sint adjust_dam(long *die_average, object_type *o_ptr, monster_type *m_ptr, u32b f1, u32b f2, u32b f3)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	/*
	 * Assume no special adjustments to damage.  We normally multiply damage
	 * by 10 for accuracy.
	 */
	int mul = 10;
	int add = 0;


	/* Add temporary bonuses */
	/* Hack -- paladins (and priests) cannot take advantage of
	 * temporary elemental brands to rescue their
	 * lousy shooting skill.
	 *
	 * Missile weapons are "kind of" edged, right?
	 */
	if (((o_ptr->tval == TV_SHOT) ||
	     (o_ptr->tval == TV_ARROW) ||
	     (o_ptr->tval == TV_BOLT)) &&
	    (check_ability(SP_BLESS_WEAPON)))
	{
		if (p_ptr->special_attack & (ATTACK_ACID)) f1 |= TR1_BRAND_ACID;
		if (p_ptr->special_attack & (ATTACK_FIRE)) f1 |= TR1_BRAND_FIRE;
		if (p_ptr->special_attack & (ATTACK_ELEC)) f1 |= TR1_BRAND_ELEC;
		if (p_ptr->special_attack & (ATTACK_COLD)) f1 |= TR1_BRAND_COLD;
		if (p_ptr->special_attack & (ATTACK_POIS)) f1 |= TR1_BRAND_POIS;
	}
	if (p_ptr->special_attack & (ATTACK_HOLY)) f1 |= TR1_SLAY_EVIL;

	/* Wielded weapons and diggers and fired missiles may do extra damage. */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			/* Slay Animal */
			if ((f1 & (TR1_SLAY_ANIMAL)) && (r_ptr->flags3 & (RF3_ANIMAL)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_ANIMAL);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 20)) mul = 20;
				else if (mul < 17) mul = 17;
			}

			/* Slay Evil */
			if ((f1 & (TR1_SLAY_EVIL)) && (r_ptr->flags3 & (RF3_EVIL)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_EVIL);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 17)) mul = 17;
				else if (mul < 15) mul = 15;
			}

			/* Slay Undead */
			if ((f1 & (TR1_SLAY_UNDEAD)) && (r_ptr->flags3 & (RF3_UNDEAD)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_UNDEAD);
				}

				if (f1 & (TR1_SLAY_KILL) && (mul < 25)) mul = 25;
				else if (f1 & (TR1_SLAY_UNDEAD) && (mul < 20)) mul = 20;
			}

			/* Slay Demon */
			if ((f1 & (TR1_SLAY_DEMON)) && (r_ptr->flags3 & (RF3_DEMON)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_DEMON);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 25)) mul = 25;
				else if (mul < 20) mul = 20;
			}

			/* Slay Orc */
			if ((f1 & (TR1_SLAY_ORC)) && (r_ptr->flags3 & (RF3_ORC)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_ORC);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 25)) mul = 25;
				else if (mul < 20) mul = 20;
			}

			/* Slay Troll */
			if ((f1 & (TR1_SLAY_TROLL)) && (r_ptr->flags3 & (RF3_TROLL)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_TROLL);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 25)) mul = 25;
				else if (mul < 20) mul = 20;
			}

			/* Slay Giant */
			if ((f1 & (TR1_SLAY_GIANT)) && (r_ptr->flags3 & (RF3_GIANT)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_GIANT);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 25)) mul = 25;
				else if (mul < 20) mul = 20;
			}

			/* Slay Dragon */
			if ((f1 & (TR1_SLAY_DRAGON)) && (r_ptr->flags3 & (RF3_DRAGON)))
			{
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_DRAGON);
				}

				if ((f1 & (TR1_SLAY_KILL)) && (mul < 25)) mul = 25;
				else if (mul < 20) mul = 20;
			}

			/* Brand (Acid) */
			if (f1 & (TR1_BRAND_ACID))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_ACID))
				{
					if (m_ptr->ml)
					{
						l_ptr->flags3 |= (RF3_IM_ACID);
					}
				}

				/* Otherwise, take extra damage */
				else if (mul < 17) mul = 17;
			}

			/* Brand (Elec) */
			if (f1 & (TR1_BRAND_ELEC))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_ELEC))
				{
					if (m_ptr->ml)
					{
						l_ptr->flags3 |= (RF3_IM_ELEC);
					}
				}

				/* Otherwise, take extra damage */
				else if (mul < 17) mul = 17;
			}

			/* Brand (Fire) */
			if (f1 & (TR1_BRAND_FIRE))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_FIRE))
				{
					if (m_ptr->ml)
					{
						l_ptr->flags3 |= (RF3_IM_FIRE);
					}
				}

				/* Otherwise, take extra damage */
				else
				{
					if ((o_ptr->name2 == EGO_BALROG) &&
						(mul < 30)) mul = 30;
					else if (mul < 17) mul = 17;
				}
			}

			/* Brand (Cold) */
			if (f1 & (TR1_BRAND_COLD))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_COLD))
				{
					if (m_ptr->ml)
					{
						l_ptr->flags3 |= (RF3_IM_COLD);
					}
				}

				/* Otherwise, take extra damage */
				else if (mul < 17) mul = 17;
			}

			/* Brand (Poison) */
			if (f1 & (TR1_BRAND_POIS))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_POIS))
				{
					if (m_ptr->ml)
					{
						l_ptr->flags3 |= (RF3_IM_POIS);
					}
				}

				/* Otherwise, take extra damage */
				else if (mul < 17) mul = 17;
			}

			/* Additional bonus for Holy Light */
			if (check_ability(SP_HOLY_LIGHT))
			{
				/* +2 or +3 versus Undead and light-sensitive creatures */
				if ((r_ptr->flags3 & (RF3_UNDEAD)) || (r_ptr->flags3 & (RF3_HURT_LITE)))
				{
					mul += (mul + 10) / 10;
				}

				/* +1 or +2 versus other Evil creatures */
				else if (r_ptr->flags3 & (RF3_EVIL))
				{
					mul += mul / 10;
				}

				/* Notice */
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (r_ptr->flags3 & RF3_UNDEAD);
					l_ptr->flags3 |= (r_ptr->flags3 & RF3_EVIL);
					l_ptr->flags3 |= (r_ptr->flags3 & RF3_HURT_LITE);
				}
			}

			break;
		}
	}

	/* Hack - Sometimes, a temporary Holy Attack becomes exhusted. */
	if ((p_ptr->special_attack & (ATTACK_HOLY)) && (randint(20) == 1))
	{
		p_ptr->special_attack &= ~(ATTACK_HOLY);
		msg_print("Your temporary Holy attack has dissipated.");

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATUS);
	}


	/*
	 * In addition to multiplying the base damage, slays and brands also
	 * add to it.  This means that a dagger of Slay Orc (1d4) is a lot
	 * better against orcs than is a dagger (1d9).
	 */
	if (mul > 10) add = (mul - 10);


	/* Apply multiplier to the die average now. */
	*die_average *= mul;

	/* Return the addend for later handling. */
	return (add);
}



/*
 * Calculate the damage done by a druid fighting barehanded, display an
 * appropriate message, and determine if the blow is capable of causing
 * monster confusion. -LM-
 *
 * Added minimum attack quality and a bonus for deadliness.  Also changed
 * the chance of confusion to depend on skill rather than plev.
 */
static int get_druid_damage(int plev, char m_name[], int power, int deadliness)
{
	cptr description;
	int dd = 0, ds = 0;
	int chance, n, n_chances, i;
	int damage;
	int b_select;
	bool power_strike = FALSE;

        /* Specialty Ability */
        if ((check_ability(SP_MARTIAL_ARTS) && (rand_int(6) == 0))
	    || (check_ability(SP_POWER_STRIKE) && (rand_int(8) == 0)))
	{
		power_strike = TRUE;
	}

	/*
	 * Minimum attack quality.
	 * That this is not zero makes very little difference in
	 * terms of mean damage.  It just makes players feel better
	 * to not get the low end attacks at high plev. -BR-
	 */
	b_select = 1 + (plev / 10);

	/*
	 * How many chances we get to choose an attack is
	 * based on deadliness.  For each chance we will
	 * choose an attack, and and keep the best one -BR-
	 */

	/* Minimum deadliness (No negative deadliness for druid blows) */
	if (deadliness < 1) i = 0;

	/* Otherwise */
	else i = deadliness_conversion[deadliness];

	/* Get between 2 and 5 chances */
	n_chances = 2 + (i / 100);

	/* Deal with fractional chances */
	if (rand_int(100) < (i % 100)) n_chances++;

	/* Loop over number of number of chances */
	for (n=0; n < n_chances; n++)
	{
		/* Choose a (level restricted) attack */
		chance = randint(2 * plev / 5);

		/* Keep the best attack */
		if (chance > b_select) b_select = chance;
	}

	/* Better Hits on a Power Strike - sometimes better than normally allowed */
	if (power_strike && (b_select < NUM_D_BLOWS)) b_select++;

	/* Just to make sure... */
	if (b_select  > NUM_D_BLOWS) b_select = NUM_D_BLOWS;

	/*
	 * Now, get the description and calculate the damage.
	 * Maximize for power stikes.
	 */
	description = d_blow[b_select - 1].description;
	dd = d_blow[b_select - 1].dd;
	ds = d_blow[b_select - 1].ds;
	if (power_strike) damage = (dd * ds);
	else damage = damroll(dd, ds);

	/* Record the damage for display */
	for (i = 0; i < 11; i++)
	  p_ptr->barehand_dam[i] = p_ptr->barehand_dam[i + 1];
	p_ptr->barehand_dam[11] = (u16b)damage;
  
	/* Druids can also confuse monsters. */
	if ((power_strike && (rand_int(3) != 0)) || (power > rand_int(500) + 25))
	{
		/* Use the special druid confusion attack. */
		p_ptr->special_attack |= (ATTACK_DRUID_CONFU);

		/* And display the attack message. Feedback for Power Strike Specialty */
		if (power_strike) message_format(MSG_HIT, 0, "Power Strike! You attempt to confuse %s.", m_name);
		else message_format(MSG_HIT, 0, "You %s and attempt to confuse %s.", description, m_name);
	}
	else
	{
		/* Basic attack message. */
		if (power_strike) message_format(MSG_HIT, 0, "Power Strike! You %s %s.", description, m_name);
		else message_format(MSG_HIT, 0, "You %s %s.", description, m_name);
	}
	return(damage);
}


/*
 * Deadliness multiplies the damage done by a percentage, which varies
 * from 0% (no damage done at all) to at most 355% (damage is multiplied
 * by more than three and a half times!).
 *
 * We use the table "deadliness_conversion" to translate internal plusses
 * to deadliness to percentage values.
 *
 * This function multiplies damage by 100.
 */
static void apply_deadliness(long *die_average, int deadliness)
{
	int i;

	/* Paranoia - ensure legal table access. */
	if (deadliness >  150) deadliness =  150;
	if (deadliness < -150) deadliness = -150;

	/* Deadliness is positive - damage is increased */
	if (deadliness >= 0)
	{
		i = deadliness_conversion[deadliness];

		*die_average *= (100 + i);
	}

	/* Deadliness is negative - damage is decreased */
	else
	{
		i = deadliness_conversion[ABS(deadliness)];

		if (i >= 100) *die_average = 0;
		else *die_average *= (100 - i);
	}
}


/*
 * Player attacks a (poor, defenseless) creature in melee.
 * -RAK-, -BEN-, -LM-, -BR-
 *
 * Handle various bonuses, and try for a shield bash.
 *
 * (What happens when the character hits a monster)
 *      Critical hits may increase the number of dice rolled.  Slays, brands,
 * monster resistances, and the Deadliness percentage all affect the average
 * value of each die (this produces the same effect as simply multiplying
 * the damage, but produces a smoother graph of possible damages).
 *      Once both the number of dice and the dice sides are adjusted, they
 * are rolled.  Any special adjustments to damage (like those from slays)
 * are then applied.
 *
 * Example:
 * 1) Dagger (1d4):
 *    1 die rolling four
 * 2) Gets a small critical hit for +2 damage dice:
 *    3 dice rolling four.  Average of each die is 2.5
 * 3) Dagger is a weapon of Slay Orc, and monster is an orc:
 *    average of each die is 2.5 * 2.0 = 5.0
 * 4) Player has a Deadliness value of +72, which gives a bonus of 200%:
 *    5.0 * (100% + 200%) -> average of each die is now 15
 * 5) Roll three dice, each with an average of 15, to get an average damage
 *    of 45.
 * 6) Finally, the special bonus for orc slaying weapons against orcs is
 *    added:
 *    45 + 10 bonus = average damage of 55.
 *
 * If a weapon is not wielded, characters get fist strikes.
 *
 * Successful hits may induce various special effects, including earthquakes,
 * confusion blows, monsters panicking, and so on.
 */
void py_attack(int y, int x)
{
	/* Damage */
	long damage;
	bool bonus_attack = FALSE;

	/* blow count */
	int num = 0;
	int blows = p_ptr->num_blow;

	/* Bonus to attack if monster is sleeping. */
	int sleeping_bonus = 0;

	/* AC Bonus (penalty) for terrain */
	int terrain_bonus = 0;

	/* Skill and Deadliness */
	int bonus, chance, total_deadliness;

	/* Variables for the bashing code */
	int bash_chance, bash_quality, bash_dam;

	bool first_hit = TRUE;

	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_lore *l_ptr;

	object_type *o_ptr;
	u32b f1, f2, f3;

	char m_name[80];

	bool fear = FALSE;

	bool do_quake = FALSE;

	bool did_burn = FALSE;

	/* Get the monster */
	m_ptr = &m_list[cave_m_idx[y][x]];
	r_ptr = &r_info[m_ptr->r_idx];
	l_ptr = &l_list[m_ptr->r_idx];

	/*
	 * If the monster is sleeping and visible, it can be hit more easily.
	 */
	if ((m_ptr->csleep) && (m_ptr->ml))
	{
		sleeping_bonus =  5 + 1 * p_ptr->lev / 5;
		if (check_ability(SP_BACKSTAB))
			sleeping_bonus *= 2;
		else if (check_ability(SP_ASSASSINATE))
		        sleeping_bonus = (3 * sleeping_bonus / 2);
	}

	/* Disturb the monster */
	m_ptr->csleep = 0;

	/* Disturb the player */
	disturb(0, 0);

	/* Auto-Recall if possible and visible */
	if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

	/* Track a new monster */
	if (m_ptr->ml) health_track(cave_m_idx[y][x]);

	/* Extract monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Handle player fear */
	if (p_ptr->afraid)
	{
		if (m_ptr->ml)
		{
			/* Message */
			msg_format("You are too afraid to attack %s!", m_name);
		}
		else
		{
			/* Special Message */
			msg_print("Something scary is in your way!");
		}

		/* Done */
		return;
	}

	/* Monsters in rubble can take advantage of cover. */
	if (cave_feat[y][x] == FEAT_RUBBLE)
	{
		terrain_bonus = r_ptr->ac / 7 + 5;
	}
	/* Monsters in trees can take advantage of cover, except
	 * from players who know nature lore.
	 */
	if ((cave_feat[y][x] == FEAT_TREE) &&
	    (!check_ability(SP_WOODSMAN)))
	{
		terrain_bonus = r_ptr->ac / 7 + 5;
	}
	/* Monsters in water are vulnerable.  */
	if (cave_feat[y][x] == FEAT_WATER)
	{
		terrain_bonus -= r_ptr->ac / 5;
	}


	/**** The monster bashing code. -LM-  ****/

	/* No shield on arm, no bash.  */
	if ((!inventory[INVEN_ARM].k_idx) || (p_ptr->shield_on_back))
	{
		bash_chance = 0;
	}

	/*
	 * Players do not bash if they could otherwise take advantage of special
	 * bonuses against sleeping monsters, or if the monster is low-level or
	 * not visible.
	 */
	else if ((sleeping_bonus) || (!m_ptr->ml) || (r_ptr->level < p_ptr->lev / 2))
	{
		bash_chance = 0;
	}

	/* Bashing chance depends on melee Skill, Dex, and a class level bonus. */
	else bash_chance = p_ptr->skill_thn +
		(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128
		+ ((check_ability(SP_STRONG_BASHES)) ?
		p_ptr->lev : 0);

	/* Spell casters don't bash much - except for those who like the "blunt" nature of bashes. */
	if ((check_ability(SP_STRONG_MAGIC)) && (!(check_ability(SP_BLESS_WEAPON))))
	{
		bash_chance /= 3;
	}

	/* Players bash more often when they see a real need. */
	/* Unarmed and unskilled */
	if ((!(inventory[INVEN_WIELD].k_idx)) && (bash_chance) &&
	    (!(check_ability(SP_UNARMED_COMBAT))) && (!(check_ability(SP_MARTIAL_ARTS))))
	{
		bash_chance *= 4;
	}

	/* ... or armed with a puny weapon */
	if ((inventory[INVEN_WIELD].k_idx) && (bash_chance) &&
	    ((inventory[INVEN_WIELD].dd * inventory[INVEN_WIELD].ds * blows)
	     < (inventory[INVEN_ARM].dd * inventory[INVEN_ARM].ds * 3)))
	{
		bash_chance *= 2;
	}

	/* Try to get in a shield bash. */
	if (bash_chance > rand_int(240 + r_ptr->level * 9))
	{
		message(MSG_HIT, 0, "You get in a shield bash!");

		/* Calculate attack quality, a mix of momentum and accuracy. */
		bash_quality = p_ptr->skill_thn + (p_ptr->wt / 8) +
			(p_ptr->total_weight / 80) + (inventory[INVEN_ARM].weight / 3);

		/* Enhanced for shield masters */
		if (check_ability(SP_SHIELD_MAST))
		        bash_quality += inventory[INVEN_ARM].weight / 5;

		/* Calculate damage.  Big shields are deadly. */
		bash_dam = damroll(inventory[INVEN_ARM].dd, inventory[INVEN_ARM].ds);

		/* Multiply by quality and experience factors */
		bash_dam *= bash_quality / 20 + p_ptr->lev / 7;

		/* Strength bonus. */
		bash_dam += (adj_str_td[p_ptr->stat_ind[A_STR]] - 128);

		/* Paranoia. */
		if (bash_dam > 125) bash_dam = 125;

		/* Encourage the player to keep wearing that heavy shield. */
		if (randint(bash_dam) > 30 + randint(bash_dam / 2))
		{
			message(MSG_HIT, 0, "WHAMM!");
		}

		/* Damage, check for fear and death. */
		if (mon_take_hit(cave_m_idx[y][x], bash_dam, &fear, NULL))
		{
			/*
			 * Hack -- High-level warriors can spread their attacks out
			 * among weaker foes.
			 * In this case a shield bash kill takes the same time one
			 * attack.
			 */
			if ((check_ability(SP_SPREAD_ATTACKS)) && (p_ptr->energy_use) &&
			    (p_ptr->lev > 39))
			{
				p_ptr->energy_use = p_ptr->energy_use / blows;
			}

			/* Specialty Ability Fury */
			if (check_ability(SP_FURY))
			{
				int boost_value;

				/* Mega-Hack - base bonus value on energy used this round */
				/* value = 15 * (energy_use/100) * (10/energy per turn) */
				boost_value = (3 * p_ptr->energy_use) / (extract_energy[p_ptr->pspeed] * 2);

				/* Minimum boost */
				boost_value = ((boost_value > 0) ? boost_value : 1);

				add_speed_boost(boost_value);
			}

			/* Fight's over. */
			return;
		}

		/* Stunning. */
		if (bash_quality + p_ptr->lev > randint(200 + r_ptr->level * 4))
		{
		       message_format(MSG_HIT, 0, "%^s is stunned.", m_name);

			m_ptr->stunned += rand_int(p_ptr->lev / 5) + 4;
			if (m_ptr->stunned > 24) m_ptr->stunned = 24;
		}

		/* Confusion. */
		if (bash_quality + p_ptr->lev > randint(300 + r_ptr->level * 6) &&
			!(r_ptr->flags3 & (RF3_NO_CONF)))
		{
			message_format(MSG_HIT, 0, "%^s appears confused.", m_name);

			m_ptr->confused += rand_int(p_ptr->lev / 5) + 4;
		}

		/* The player will sometimes stumble. */
		if ((30 + adj_dex_th[p_ptr->stat_ind[A_DEX]] - 128) < randint(60))
		{
			blows -= randint(blows);

			message(MSG_GENERIC, 0, "You stumble!");
		}
	}


	/* Get the weapon */
	o_ptr = &inventory[INVEN_WIELD];

	/* Extract the object's flags. */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Calculate the attack quality */
	bonus = p_ptr->to_h + o_ptr->to_h;
	chance = p_ptr->skill_thn + (BTH_PLUS_ADJ * bonus);

	/* Calculate deadliness */
	total_deadliness = p_ptr->to_d + o_ptr->to_d;

	/* Paranoia.  Ensure legal table access. */
	if (total_deadliness > 150) total_deadliness = 150;

	/* Specialty Ability */
	if ((check_ability(SP_FAST_ATTACK)) && (randint(8) <= p_ptr->num_blow))
	{
		blows++;
		bonus_attack = TRUE;
	}

	/* Attack once for each legal blow */
	while (num++ < blows)
	{
		int crit_dice = 0;

		/* Credit where credit is due */
		if (bonus_attack && (num == blows)) msg_print("Extra Attack!");

		/* Test for hit */
		if (test_hit_combat(chance + sleeping_bonus, r_ptr->ac + terrain_bonus, m_ptr->ml))
		{
			/* Sound */
			sound(SOUND_HIT);

			/* If this is the first hit, make some noise. */
			if (first_hit)
			{
				/* Only 1 first hit per round */
				first_hit = FALSE;

				/* Hack -- Rogues and Assassins are silent melee
				 * killers. */
				if ((check_ability(SP_BACKSTAB)) ||
				    (check_ability(SP_ASSASSINATE)))
				add_wakeup_chance = p_ptr->base_wakeup_chance / 4 + 500;

				/* Otherwise, make the standard amount of noise. */
				 else add_wakeup_chance =
					p_ptr->base_wakeup_chance / 2 + 1000;
			}


			/* Character is wielding a weapon. */
			if (o_ptr->k_idx)
			{
				int dice, add;
				long die_average, temp, sides;

				/* Base damage dice */
				dice = o_ptr->dd;

				/* Calculate criticals. */
				crit_dice = critical_melee(chance, sleeping_bonus, m_ptr->ml,
				                       m_name, o_ptr);

				/* Critical hits may add damage dice. */
				dice += crit_dice;

				/* Mana Burn Specialty */
				if ((crit_dice > 0) && (!did_burn) && check_ability(SP_MANA_BURN) && (m_ptr->mana))
				{
					/* Base burn amount */
					int burn = BASE_MANA_BURN;

					/* Only one per round! */
					did_burn = TRUE;

					/* Can only burn what we have */
					if (burn > m_ptr->mana) burn = m_ptr->mana;

					/* Hit mana and add to damage */
					m_ptr->mana -= burn;
					dice++;

					message(MSG_HIT, 0, "Mana Burn!");
				}

				/* Get the average value of a single damage die. (x10) */
				die_average = (10 * (o_ptr->ds + 1)) / 2;

				/* Adjust the average for slays and brands. (10x inflation) */
				add = adjust_dam(&die_average, o_ptr, m_ptr, f1, f2, f3);

				/* Apply deadliness to average. (100x inflation) */
				apply_deadliness(&die_average, total_deadliness);


				/* Convert die average to die sides. */
				temp = (2 * die_average) - 10000;

				/* Calculate the actual number of sides to each die. */
				sides = (temp / 10000) +
				        (rand_int(10000) < (temp % 10000) ? 1 : 0);


				/* Roll out the damage. */
				damage = damroll(dice, (s16b)sides);

				/* Apply any special additions to damage. */
				damage += add;

			}

			else
			{
				/* Hack - If no weapon is wielded, Druids are
				 * able to fight effectively.   All other classes only get 1 damage, plus
				 * whatever bonus their strength gives them. -LM-
				 */
				if ((check_ability(SP_UNARMED_COMBAT)) || check_ability(SP_MARTIAL_ARTS))
				{
					damage = get_druid_damage(p_ptr->lev, m_name, chance + sleeping_bonus, total_deadliness);
				}
				else
				{
					damage = 1 + ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
					message_format(MSG_HIT, 0, "You punch %s.", m_name);
				}
			}

			/* Paranoia -- No negative damage */
			if (damage < 0) damage = 0;

			/* Hack -- check for earthquake. */
			if (p_ptr->impact && (damage > 49)) do_quake = TRUE;

			/* The verbose wizard message has been moved to mon_take_hit. */

			/* Damage, check for fear and death. */
			if (mon_take_hit(cave_m_idx[y][x], (s16b)damage, &fear, NULL))
			{
				/*
				 * Hack -- High-level warriors can spread their attacks out
				 * among weaker foes.
				 */
				if ((check_ability(SP_SPREAD_ATTACKS)) && (p_ptr->energy_use) &&
				    (p_ptr->lev > 39) && (num < blows))
				{
					p_ptr->energy_use = p_ptr->energy_use * num / blows;
				}

				/* No "holding over" druid confusion attacks */
				p_ptr->special_attack &= ~(ATTACK_DRUID_CONFU);

				/* Fight's over. */
				break;
			}

			/* Confusion attack */
			if ((p_ptr->special_attack & (ATTACK_CONFUSE)) ||
				(p_ptr->special_attack & (ATTACK_DRUID_CONFU)))
			{
				/* Message */
				if (!( p_ptr->special_attack & (ATTACK_DRUID_CONFU)))
				  message(MSG_HIT, 0, "Your hands stop glowing.");

				/* Cancel special confusion attack */
				p_ptr->special_attack &= ~(ATTACK_CONFUSE);
				p_ptr->special_attack &= ~(ATTACK_DRUID_CONFU);
				p_ptr->redraw |= PR_STATUS;

				/* Confuse the monster */
				hit_monster_confuse(110, m_ptr);
			}

			/* Black Breath attack. -LM- */
			if (p_ptr->special_attack & (ATTACK_BLKBRTH))
			{
				/* Cancel black breath */
				p_ptr->special_attack &= ~(ATTACK_BLKBRTH);

				/* Message */
				msg_print("Your hands stop radiating Night.");

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATUS);

				/* Apply Black Breath to the monster */
				hit_monster_black_breath(160, m_ptr);
			}

			/* Rogue "Hit and Run" attack. -LM- */
			if (p_ptr->special_attack & (ATTACK_FLEE))
			{
				/* Cancel the fleeing spell */
				p_ptr->special_attack &= ~(ATTACK_FLEE);

				/* Message */
				msg_print("You escape into the shadows!");

				/* Teleport. */
				teleport_player(6 + p_ptr->lev / 5, TRUE);

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATUS);

				/* Specialty Ability Fury */
				if (check_ability(SP_FURY))
				{
					int boost_value;

					/* Mega-Hack - base bonus value on energy used this round */
					/* value = 15 * (energy_use/100) * (10/energy per turn) */
					boost_value = (3 * p_ptr->energy_use) / (extract_energy[p_ptr->pspeed] * 2);

					/* Minimum boost */
					boost_value = ((boost_value > 0) ? boost_value : 1);

					add_speed_boost(boost_value);
				}

				/* Fight's over. */
				return;
			}

			/* Monster is no longer asleep */
			sleeping_bonus = 0;
		}

		/* Player misses */
		else
		{
			/* Sound */
			sound(SOUND_MISS);

			/* Message */
			message_format(MSG_MISS, 0, "You miss %s.", m_name);
		}
	}

	/* Specialty Ability Fury */
	if (check_ability(SP_FURY))
	{
		int boost_value;

		/* Mega-Hack - base bonus value on energy used this round */
		/* value = 15 * (energy_use/100) * (10/energy per turn) */
		boost_value = (3 * p_ptr->energy_use) / (extract_energy[p_ptr->pspeed] * 2);

		/* Minimum boost */
		boost_value = ((boost_value > 0) ? boost_value : 1);

		add_speed_boost(boost_value);
	}

	/* Hack -- delayed fear messages */
	if (fear && m_ptr->ml)
	{
		/* Sound */
		sound(SOUND_FLEE);

		/* Message */
		message_format(MSG_FLEE, m_ptr->r_idx,
			       "%^s flees in terror!", m_name);
	}


	/* Mega-Hack -- apply earthquake brand.   Radius reduced in Oangband. */
	if (do_quake)
	{
		int py = p_ptr->py;
		int px = p_ptr->px;

		earthquake(py, px, 4, FALSE);
	}
}



/*
 * Determine the odds of an object breaking when thrown at a monster.
 *
 * Note that artifacts never break; see the "drop_near()" function.
 */
static int breakage_chance(object_type *o_ptr)
{
	/* Examine the item type */
	switch (o_ptr->tval)
	{
		/* Always break */
		case TV_FLASK:
		case TV_POTION:
		case TV_BOTTLE:
		case TV_FOOD:
		case TV_JUNK:
		{
			return (100);
		}

		/* Often break */
		case TV_LITE:
		case TV_SCROLL:
		case TV_SKELETON:
		{
			return (40);
		}
		/* Frequently break */
		case TV_ARROW:
		{
			return (30);
		}

		/* Sometimes break */
		case TV_WAND:
		case TV_SHOT:
		case TV_BOLT:
		case TV_SPIKE:
		{
			return (20);
		}
	}

	/* Rarely break */
	return (10);
}


/*
 * Fire an object from the pack or floor.
 *
 * You may only fire items that match your missile launcher.
 *
 * Project the missile along the path from player to target.
 * If it enters a grid occupied by a monster, check for a hit.
 *
 * The basic damage calculations in archery are the same as in melee,
 * except that a launcher multiplier is also applied to the dice sides.
 *
 * Apply any special attack or class bonuses, and check for death.
 * Drop the missile near the target (or end of path), sometimes breaking it.
 *
 * Returns true if a shot is actually fired.
 */
void do_cmd_fire(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int dir, item;
	int i, j, y, x, ty, tx;
	int tdis;

	int break_chance;

	int armour, bonus, chance, total_deadliness;

	int sleeping_bonus = 0;
	int terrain_bonus = 0;

	int damage = 0;

	int multi;
	int spread_y_cen = 0;
	int spread_x_cen = 0;
	int spread_sign_y = 0;
	int spread_sign_x = 0;
	int spread_roll = 1;

	/* Assume no weapon of velocity or accuracy bonus. */
	bool special_dam = FALSE;
	bool special_hit = FALSE;

	object_type *o_ptr;
	object_type *j_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	u32b f1_temp, f2_temp, f3_temp;
	u32b f1, f2, f3;

	bool hit_body = FALSE;
	bool did_pierce = FALSE;
	bool did_miss = FALSE;

	byte missile_attr;
	char missile_char;

	char o_name[120];
	char m_name[80];

	int path_n = 0;
	u16b path_g[256];

	cptr q, s;

	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	/* Nothing fired -NRM- */
	did_fire = FALSE;

	/* Get the "bow" (if any) */
	o_ptr = &inventory[INVEN_BOW];

	/* Require a usable launcher */
	if (!o_ptr->tval || !p_ptr->ammo_tval)
	{
		msg_print("You have nothing to fire with.");
		fire_mode = FIRE_MODE_NORMAL;
		return;
	}

	/* Require proper missile */
	item_tester_tval = p_ptr->ammo_tval;

	/* Do we have an item? */
	if (p_ptr->command_item) {
	        item = handle_item();
		if (!get_item_allow(item)) return;
	}

	/* Get an item */
	else
	{
	        q = "Fire which item? ";
		s = "You have nothing to fire.";
		if (!get_item(&item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR))) 
		  {
		    fire_mode = FIRE_MODE_NORMAL;
		    return;
		  }
	}

	/* Access the item (if in the pack) */
	if (item >= 0)
	{
		j_ptr = &inventory[item];
	}
	else
	{
		j_ptr = &o_list[0 - item];
	}

	/* Get a direction (or cancel) */
	if (!get_aim_dir(&dir)) 
	  {
		fire_mode = FIRE_MODE_NORMAL;
		return;
	  }

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, j_ptr);

	/* Single object */
	i_ptr->number = 1;

	/* Reduce and describe inventory */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}

	/* Extract the ammo flags. */
	object_flags(i_ptr, &f1_temp, &f2_temp, &f3_temp);

	/* Store them */
	f1 = f1_temp;
	f2 = f2_temp;
	f3 = f3_temp;

	/* Extract the launcher flags. */
	object_flags(o_ptr, &f1_temp, &f2_temp, &f3_temp);

	/* Store them */
	f1 |= f1_temp;
	f2 |= f2_temp;
	f3 |= f3_temp;

	/* Extra flags from modal shots */
	if (fire_mode == FIRE_MODE_STORM) f1 |= TR1_BRAND_ELEC;
	if (fire_mode == FIRE_MODE_POISON) f1 |= TR1_BRAND_POIS;
	if (fire_mode == FIRE_MODE_SLAY_DRAGON) f1 |= TR1_SLAY_DRAGON;
	if (fire_mode == FIRE_MODE_SCOURGE)
	{
		f1 |= TR1_SLAY_ORC;
		f1 |= TR1_SLAY_TROLL;
	}

	/* Take a (partial) turn */
	p_ptr->energy_use = (1000 / p_ptr->num_fire);

	/* Sound */
	sound(SOUND_SHOOT);

	/* Missile launchers of Velocity sometimes "supercharge" */
	if ((o_ptr->name2 == EGO_VELOCITY) && (rand_int(5) == 0))
	{
		object_desc(o_name, o_ptr, FALSE, 0);

		/* Set special damage */
		special_dam = TRUE;

		/* Give a hint to the player. */
		if (!object_known_p(o_ptr)) msg_format("You feel a strange aura of power around your %s.", o_name);
		else msg_format("Your %s feels very powerful.", o_name);
	}

	/* Missile launchers of Accuracy sometimes "supercharge" */
	if ((o_ptr->name2 == EGO_ACCURACY) && (rand_int(5) == 0))
	{
		object_desc(o_name, o_ptr, FALSE, 0);

		/* Set special accuracy */
		special_hit = TRUE;

		/* Give a hint to the player. */
		if (!object_known_p(o_ptr)) msg_format("You feel a strange aura of power around your %s.", o_name);
		else msg_format("Your %s feels very accurate.", o_name);
	}

	/* Fire ammo of backbiting, and it will turn on you.  -LM- */
	if (i_ptr->name2 == EGO_BACKBITING)
	{
		/* Message. */
		msg_print("Your missile turns in midair and strikes you!");

		/* Calculate damage. */
		damage = damroll(p_ptr->ammo_mult * i_ptr->dd * randint(2), i_ptr->ds * 4);
		if (special_dam)
		{
			damage += 15;
		}

		/* Inflict both normal and wound damage. */
		take_hit(damage, "ammo of backbiting");
		set_cut(randint(damage * 3));

		/* That ends that shot! */
		did_fire = TRUE;
		fire_mode = FIRE_MODE_NORMAL;
		return;
	}

	/* Describe the object */
	object_desc(o_name, i_ptr, FALSE, 3);

	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(i_ptr);
	missile_char = object_char(i_ptr);


	/* Base range  (XXX - this formula is a little weird) */
	tdis = 10 + 5 * p_ptr->ammo_mult;

	/* Calculate the quality of the shot */
	bonus = (p_ptr->to_h + o_ptr->to_h + i_ptr->to_h);
	chance = p_ptr->skill_thb + (BTH_PLUS_ADJ * bonus);

	/* Accuracy bonus for deadeye shots */
	if (fire_mode == FIRE_MODE_DEADEYE) chance += 15;

	/* Sum all the applicable additions to Deadliness. */
	total_deadliness = p_ptr->to_d + o_ptr->to_d + i_ptr->to_d;

	/* Start at the player */
	y = py;
	x = px;

	/* Predict the "target" location */
	ty = py + 99 * ddy[dir];
	tx = px + 99 * ddx[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay())
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	/* Calculate the path */
	path_n = project_path(path_g, tdis, py, px, ty, tx, PROJECT_THRU);

	/* Hack -- Handle stuff */
	handle_stuff();

	/* Multishot mode? */
	multi = ((fire_mode == FIRE_MODE_MULTI) ? (randint(3) + 2) : 1);

	/* Later shots may spread out, save some numbers */
	if (multi > 1) spread_target_prepare(py, px, ty, tx, &spread_y_cen, &spread_x_cen, &spread_roll, &spread_sign_y, &spread_sign_x);

	/* Split if needed */
	for (j = 0; j < multi; j++)
	{
		/* Treat each as a fresh shot */
		hit_body = FALSE;
		did_pierce = FALSE;
		did_miss = FALSE;

		/* Multi-shots may spread somewhat */
		if (j > 0)
		{
			int new_y;
			int new_x;

			/* Spread perpendicular to line of fire */
			spread_target(spread_y_cen, spread_x_cen, spread_roll, &spread_sign_y, &spread_sign_x, TRUE, &new_y, &new_x);

			/* Recalculate path */
			path_n = project_path(path_g, tdis, py, px, new_y, new_x, PROJECT_THRU);
		}

		/* Project along the path */
		for (i = 0; i < path_n; ++i)
		{
			int ny = GRID_Y(path_g[i]);
			int nx = GRID_X(path_g[i]);

			/* Hack -- Stop before hitting walls */
			if (!cave_floor_bold(ny, nx)) break;

			/* Advance */
			x = nx;
			y = ny;

			/* Only do visuals if the player can "see" the missile */
			if (panel_contains(y, x) && player_can_see_bold(y, x))
			{
				/* Visual effects */
				print_rel(missile_char, missile_attr, y, x);
				move_cursor_relative(y, x);
				if (fresh_before) Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(y, x);
				if (fresh_before) Term_fresh();
			}

			/* Delay anyway for consistency */
			else
			{
				/* Pause anyway, for consistancy */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}

			/* Handle monster */
			if (cave_m_idx[y][x] > 0)
			{
				monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				bool fear = FALSE;

				int dice, add;
				long die_average, temp, sides;
				int cur_range, base_range, chance2;

				/* Assume a default death */
				cptr note_dies = " dies.";

				/* Find Range */
				cur_range = distance(py, px, y, x);

				/* Most Accurate Range */
				base_range = tdis / 2;
				if (base_range > 10) base_range = 10;

				/* Penalize Range */
				if (cur_range > base_range) chance2 = chance - cur_range + base_range;
				else chance2 = chance;

				/* Note the (attempted) collision */
				hit_body = TRUE;

				/* Get "the monster" or "it" */
				monster_desc(m_name, m_ptr, 0);

				/* 2nd hit on a pierce is easier */
				if (did_pierce) chance2 += chance2/2;

				/* Hard to hit after a miss */
				else if (did_miss) chance2 -= chance2/3;

				/* Sleeping, visible monsters are easier to hit. */
				if ((m_ptr->csleep) && (m_ptr->ml))
				{
					sleeping_bonus = 5 + p_ptr->lev / 5;
				}

				/* Monsters in rubble can take advantage of cover. */
				if (cave_feat[y][x] == FEAT_RUBBLE)
				{
					terrain_bonus = r_ptr->ac / 5 + 5;
				}

				/* Monsters in trees can take advantage of cover, except from
				 * players who know nature lore.
				 */
				if ((cave_feat[y][x] == FEAT_TREE) &&
				    (!check_ability(SP_WOODSMAN)))
				{
					terrain_bonus = r_ptr->ac / 5 + 5;
				}
				/* Monsters in water are vulnerable. */
				if (cave_feat[y][x] == FEAT_WATER)
				{
					terrain_bonus -= r_ptr->ac / 4;
				}

				/* Get effective armour class of monster. */
				armour = r_ptr->ac + terrain_bonus;

				/* Weapons of velocity sometimes almost negate monster armour. */
				if (special_hit) armour /= 3;

				/* Did we miss it (penalize distance travelled) */
				if (!(test_hit_combat(chance2 + sleeping_bonus, armour, m_ptr->ml)))
				{
					did_miss = TRUE;

					/* Keep Going */
					continue;
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

				/* Make some noise.  Hack -- Assassins are silent
				 * missile weapon killers.
				 */
				if (check_ability(SP_ASSASSINATE)) add_wakeup_chance =
					p_ptr->base_wakeup_chance / 4 + 600;
				else add_wakeup_chance =
					p_ptr->base_wakeup_chance / 3 + 1200;

				/* Hack -- Track this monster race, if monster is visible */
				if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

				/* Hack -- Track this monster, if visible */
				if (m_ptr->ml) health_track(cave_m_idx[y][x]);

				/*
				 * The basic damage-determination formula is the same in
				 * archery as it is in melee (apart from the launcher mul-
				 * tiplier).
				 */

				/* base damage dice */
				dice = i_ptr->dd;

				/* Critical hits may add damage dice. */
				dice += critical_shot(chance2, sleeping_bonus, FALSE, m_ptr->ml,
				                       m_name, i_ptr);


				/* Get the average value of a single damage die. (x10) */
				die_average = (10 * (i_ptr->ds + 1)) / 2;

				/* Apply the launcher multiplier. */
				die_average *= p_ptr->ammo_mult;

				/* Adjust the average for slays and brands. (10x inflation) */
				add = adjust_dam(&die_average, i_ptr, m_ptr, f1, f2, f3);

				/* Apply deadliness to average. (100x inflation) */
				apply_deadliness(&die_average, total_deadliness);

				/* Convert die average to die sides. */
				temp = (2 * die_average) - 10000;

				/* Calculate the actual number of sides to each die. */
				sides = (temp / 10000) +
				        (rand_int(10000) < (temp % 10000) ? 1 : 0);

				/* Roll out the damage. */
				damage = damroll(dice, (s16b)sides);

				/* Apply any special additions to damage. */
				damage += add;

				/* If a weapon of velocity activates, increase damage. */
				if (special_dam)
				{
					damage += 15;
				}

				/* If an "enhance missile" spell has been cast, increase
				 * the damage, and cancel the spell.
				 */
				if (p_ptr->special_attack & (ATTACK_SUPERSHOT))
				{
					damage = 5 * damage / 4 + 35;
					p_ptr->special_attack &= ~(ATTACK_SUPERSHOT);

					/* Redraw the state */
					p_ptr->redraw |= (PR_STATUS);
				}

				/* Hack - Assassins are deadly... */
				if (check_ability(SP_STRONG_SHOOT))
				{
					/* Increase damage directly (to avoid excessive total
					 * damage by granting too high a Deadliness).
					 */
					if (p_ptr->ammo_tval == TV_SHOT)
					{
						damage += p_ptr->lev / 2;
					}
					if (p_ptr->ammo_tval == TV_ARROW)
					{
						damage += 2 * p_ptr->lev / 5;
					}
					if (p_ptr->ammo_tval == TV_BOLT)
					{
						damage += p_ptr->lev / 3;
					}
				}

				/* Is this a multishot? */
				if (fire_mode == FIRE_MODE_MULTI) damage = damage / 2;

				/* No negative damage */
				if (damage < 0) damage = 0;

				/* Hit the monster, check for death. */
				if (mon_take_hit(cave_m_idx[y][x], damage, &fear, note_dies))
				{
					/* Dead monster */
				}

				/* No death */
				else
				{
					/* Message */
					message_pain(cave_m_idx[y][x], damage);

					/* Take note */
					if (fear && m_ptr->ml)
					{
						/* Sound */
						sound(SOUND_FLEE);

						/* Message */
						message_format(MSG_FLEE, m_ptr->r_idx,
							       "%^s flees in terror!", m_name);
					}
				}

				/* Storm mode creates thunderclaps */
				if (fire_mode == FIRE_MODE_STORM)
				{
					fire_meteor(0, GF_SOUND, y, x, p_ptr->lev, 1, FALSE);
				}

				/* Check for piercing */
				if ((check_ability(SP_PIERCE_SHOT)) &&
				    (rand_int(2) == 0) &&
				    ((p_ptr->ammo_tval == TV_ARROW) ||
				     (p_ptr->ammo_tval == TV_BOLT)))
				{
					msg_print("Pierce!");
					did_pierce = TRUE;

					continue;
				}

				/* Otherwise Stop looking */
				else break;
			}
		}
	}

	/* Chance of breakage (during attacks) */
	break_chance = (hit_body ? breakage_chance(i_ptr) : 0);

	/* Certain special shots always disappear */
	if (fire_mode == FIRE_MODE_MULTI) break_chance = 100;
	if (fire_mode == FIRE_MODE_STORM) break_chance = 100;

	/* Drop (or break) near that location */
	drop_near(i_ptr, break_chance, y, x);

	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;

	did_fire = TRUE;
	fire_mode = FIRE_MODE_NORMAL;
	return;
}



/*
 * Throw an object from the pack or floor.
 *
 * Now allows for throwing weapons.  Unlike all other thrown objects,
 * throwing weapons can get critical hits and take advantage of bonuses to
 * Skill and Deadliness from other equipped items.
 *
 * Unlikely shooting, thrown weapons do not continue on after a miss.
 * It's too annoying to send your nice throwing weapons halfway across the
 * dungeon.
 *
 * Returns true if a shot is actually fired.
 */
void do_cmd_throw(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int dir, item;
	int i, y, x, ty, tx;
	int chance, tdis;
	int mul, div;

	int break_chance;

	int total_deadliness;
	int sleeping_bonus = 0;
	int terrain_bonus = 0;

	int damage;

	object_type *o_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	bool hit_body = FALSE;

	byte missile_attr;
	char missile_char;

	char o_name[120];
	char m_name[80];

	int path_n = 0;
	u16b path_g[256];

	cptr q, s;

	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	u32b f1, f2, f3;

	/* Nothing thrown -NRM- */
	did_throw = FALSE;

	/* Do we have an item? */
	if (p_ptr->command_item) 
	{
	        item = handle_item();
		if (!get_item_allow(item)) return;
	}

	/* Get an item */
	else
	{
	        q = "Throw which item? ";
		s = "You have nothing to throw.";
		if (!get_item(&item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR))) 
		{
		        throw_mode = THROW_MODE_NORMAL;
		        return;
		}
	}

	/* Access the item (if in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}
	else
	{
		o_ptr = &o_list[0 - item];
	}



	/* Can't unwield cursed items this way! */
	if ((item > INVEN_PACK) && (item < INVEN_BLANK) && (cursed_p(o_ptr)))
	{
	        /* Oops */
	        msg_print("Hmmm, it seems to be cursed.");
      
		/* Nope */
		return;
	}

	/* Get a direction (or cancel) */
	if (!get_aim_dir(&dir)) 
	{
		throw_mode = THROW_MODE_NORMAL;
		return;
	}


	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, o_ptr);

	/* Distribute the charges of rods/wands between the stacks */
	distribute_charges(o_ptr, i_ptr, 1);

	/* Extract the thrown object's flags. */
	object_flags(i_ptr, &f1, &f2, &f3);

	/* Single object */
	i_ptr->number = 1;

	/* Reduce and describe inventory */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}


	/* Description */
	object_desc(o_name, i_ptr, FALSE, 3);

	/* Get the color and symbol of the thrown object */
	missile_attr = object_attr(i_ptr);
	missile_char = object_char(i_ptr);


	/* Extract a "distance multiplier" */
	mul = 10;

	/* Enforce a minimum weight of one pound */
	div = ((i_ptr->weight > 10) ? i_ptr->weight : 10);

	/* Hack -- Distance -- Reward strength, penalize weight */
	tdis = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;

	/* Max distance of 10 */
	if (tdis > 10) tdis = 10;

	/* Specialty Ability Mighty Throw */
	if (check_ability(SP_MIGHTY_THROW)) tdis *= 2;

	/*
	 * Other thrown objects are easier to use, but only throwing weapons
	 * take advantage of bonuses to Skill and Deadliness from other
	 * equipped items.
	 */
	if (f1 & (TR1_THROWING))
	{
		chance = p_ptr->skill_tht + BTH_PLUS_ADJ * (p_ptr->to_h + i_ptr->to_h);
		total_deadliness = p_ptr->to_d + i_ptr->to_d;
	}
	else
	{
		chance = (3 * p_ptr->skill_tht / 2) + (BTH_PLUS_ADJ * i_ptr->to_h);
		total_deadliness = i_ptr->to_d;
	}


	/* Take a turn */
	p_ptr->energy_use = 100;


	/* Start at the player */
	y = py;
	x = px;

	/* Predict the "target" location */
	ty = py + 99 * ddy[dir];
	tx = px + 99 * ddx[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay())
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	/* Calculate the path */
	path_n = project_path(path_g, tdis, py, px, ty, tx, 0);


	/* Hack -- Handle stuff */
	handle_stuff();

	/* Project along the path */
	for (i = 0; i < path_n; ++i)
	{
		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);

		/* Hack -- Stop before hitting walls */
		if (!cave_floor_bold(ny, nx)) break;

		/* Advance */
		x = nx;
		y = ny;

		/* Only do visuals if the player can "see" the missile */
		if (panel_contains(y, x) && player_can_see_bold(y, x))
		{
			/* Visual effects */
			print_rel(missile_char, missile_attr, y, x);
			move_cursor_relative(y, x);
			if (fresh_before) Term_fresh();
			Term_xtra(TERM_XTRA_DELAY, msec);
			lite_spot(y, x);
			if (fresh_before) Term_fresh();
		}

		/* Delay anyway for consistency */
		else
		{
			/* Pause anyway, for consistancy */
			Term_xtra(TERM_XTRA_DELAY, msec);
		}

		/* Handle monster */
		if (cave_m_idx[y][x] > 0)
		{
			monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];

			bool fear = FALSE;

			int dice, add;
			long die_average, temp, sides;
			int cur_range, base_range, chance2;

			/* Assume a default death */
			cptr note_dies = " dies.";

			/* Find Range */
			cur_range = distance(py, px, y, x);

			/* Most Accurate Range */
			base_range = tdis / 2;
			if (base_range > 10) base_range = 10;

			/* Penalize Range */
			if (cur_range > base_range) chance2 = chance - cur_range + base_range;
			else chance2 = chance;

			/* If the monster is sleeping, it'd better pray there are no
			 * Assassins with throwing weapons nearby. -LM-
			 */
			if ((m_ptr->csleep) && (m_ptr->ml) && (f1 & (TR1_THROWING)))
			{
				if (check_ability(SP_ASSASSINATE))
					sleeping_bonus = 15 + p_ptr->lev / 2;
				else sleeping_bonus = 0;
			}

			/* Monsters in rubble can take advantage of cover. */
			if (cave_feat[y][x] == FEAT_RUBBLE)
			{
				terrain_bonus = r_ptr->ac / 5 + 5;
			}
			/* Monsters in trees can take advantage of cover, except from
			 * players who know nature lore.
			 */
			if ((cave_feat[y][x] == FEAT_TREE) &&
			    (!check_ability(SP_WOODSMAN)))
			{
				terrain_bonus = r_ptr->ac / 5 + 5;
			}
			/* Monsters in water are vulnerable. */
			if (cave_feat[y][x] == FEAT_WATER)
			{
				terrain_bonus -= r_ptr->ac / 4;
			}


			/* Get "the monster" or "it" */
			monster_desc(m_name, m_ptr, 0);


			/* Note the collision */
			hit_body = TRUE;

			/* Did we miss it (penalize range)? */
			if (!(test_hit_combat(chance2 + sleeping_bonus, r_ptr->ac + terrain_bonus, m_ptr->ml)))
			{
				/* Keep Going */
				break;
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

			/* Make some noise.  Hack -- Assassins are silent
			 * missile weapon killers.
			 */
			if (check_ability(SP_ASSASSINATE)) add_wakeup_chance =
				p_ptr->base_wakeup_chance / 4 + 300;
			else add_wakeup_chance =
				p_ptr->base_wakeup_chance / 3 + 1200;


			/* Hack -- Track this monster race, if critter is visible */
			if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

			/* Hack -- Track this monster, if visible */
			if (m_ptr->ml) health_track(cave_m_idx[y][x]);

			/*
			 * The basic damage-determination formula is the same in
			 * throwing as it is in melee (apart from the thrown weapon
			 * multiplier, and the ignoring of non-object bonuses to
			 * Deadliness for objects that are not thrown weapons).
			 */

			/* Base damage dice */
			dice = i_ptr->dd;

			/* Object is a throwing weapon. */
			if (f1 & (TR1_THROWING))
			{
				/* Perfectly balanced weapons do even more damage. */
				if (f1 & (TR1_PERFECT_BALANCE)) dice *= 2;

				/* Critical hits may add damage dice. */
				dice += critical_shot(chance2, sleeping_bonus, TRUE,
				                      m_ptr->ml, m_name, i_ptr);

				/*
				 * Multiply the number of damage dice by the throwing
				 * weapon multiplier, if applicable.  This is not the
				 * prettiest equation, but it does at least try to keep
				 * throwing weapons competitive.
				 */
				dice *= 2 + p_ptr->lev / 12;
			}

			/* Ordinary thrown object */
			else
			{
				/* Display a default hit message. */
				if (m_ptr->ml)
				{
					message_format(MSG_HIT, 0, "The %s hits %s.", o_name, m_name);
				}
				else
				{
					message_format(MSG_HIT, 0, "The %s finds a mark.", o_name);
				}
			}

			/* Get the average value of a single damage die. (x10) */
			die_average = (10 * (i_ptr->ds + 1)) / 2;

			/* Adjust the average for slays and brands. (10x inflation) */
			add = adjust_dam(&die_average, i_ptr, m_ptr, f1, f2, f3);

			/* Apply deadliness to average. (100x inflation) */
			apply_deadliness(&die_average, total_deadliness);

			/* Convert die average to die sides. */
			temp = (2 * die_average) - 10000;

			/* Calculate the actual number of sides to each die. */
			sides = (temp / 10000) +
			        (rand_int(10000) < (temp % 10000) ? 1 : 0);

			/* Roll out the damage. */
			damage = damroll(dice, (s16b)sides);

			/* Apply any special additions to damage. */
			damage += add;

			/* No negative damage */
			if (damage < 0) damage = 0;

			/* Hit the monster, check for death */
			if (mon_take_hit(cave_m_idx[y][x], damage, &fear, note_dies))
			{
				/* Dead monster */
			}

			/* No death */
			else
			{
				/* Apply Black Breath to the monster */
				if (throw_mode == THROW_MODE_BLKBRTH) hit_monster_black_breath(120, m_ptr);

				/* Message */
				message_pain(cave_m_idx[y][x], damage);

				/* Take note */
				if (fear && m_ptr->ml)
				{
					/* Sound */
					sound(SOUND_FLEE);

					/* Message */
					msg_format("%^s flees in terror!", m_name);
				}
			}

			/* Object falls to the floor */
			break;
		}
	}

	/* Chance of breakage.   Throwing weapons are designed not to break. */
	if (f1 & (TR1_PERFECT_BALANCE)) break_chance = 0;
	else if (f1 & (TR1_THROWING)) break_chance = (hit_body ? 1 : 0);
	else break_chance = (hit_body ? breakage_chance(i_ptr) : (breakage_chance(i_ptr) / 2));

	/* Drop (or break) near that location */
	drop_near(i_ptr, break_chance, y, x);

	did_throw = TRUE;
	throw_mode = THROW_MODE_NORMAL;
	return;
}
