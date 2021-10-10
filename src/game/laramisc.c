#include "game/lara.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/splash.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/sndpc.h"
#include "util.h"
#include "game/text.h"

#include <stddef.h>
#include <stdint.h>

int32_t lara_jump_error = 0;
TEXTSTRING* LaraText = NULL;
TEXTSTRING* LaraText2 = NULL;

PHD_3DPOS_F lara_float_pos;
double lara_fall_speed_f = 0.0;
double lara_speed_F = 0.0;

void LaraControl(int16_t item_num)
{
    COLL_INFO coll = { 0 };

    ITEM_INFO *item = LaraItem;
    ROOM_INFO *r = &RoomInfo[item->room_number];
    int32_t room_submerged = r->flags & RF_UNDERWATER;

    if (Input & IN_ITEM_CHEAT) {
        LaraCheatGetStuff();
    }

    if (Lara.water_status != LWS_CHEAT && (Input & IN_FLY_CHEAT)) {
        if (Lara.water_status != LWS_UNDERWATER || item->hit_points <= 0) {
            item->pos.y -= 0x80;
            lara_float_pos.y -= 0x80;
            item->current_anim_state = AS_SWIM;
            item->goal_anim_state = AS_SWIM;
            item->anim_number = AA_SWIMGLIDE;
            item->frame_number = Anims[item->anim_number].frame_base;
            item->gravity_status = 0;
            item->pos.x_rot = 30 * PHD_DEGREE;
            item->fall_speed = 30;
            lara_fall_speed_f = 30.0;
            Lara.head_x_rot = 0;
            Lara.head_y_rot = 0;
            Lara.torso_x_rot = 0;
            Lara.torso_y_rot = 0;
        }
        Lara.water_status = LWS_CHEAT;
        Lara.spaz_effect_count = 0;
        Lara.spaz_effect = NULL;
        Lara.hit_frame = 0;
        Lara.hit_direction = -1;
        Lara.air = LARA_AIR;
        Lara.death_count = 0;
        Lara.mesh_effects = 0;
        LaraInitialiseMeshes(CurrentLevel);
    }

    if (Lara.water_status == LWS_ABOVEWATER && room_submerged) {
        Lara.water_status = LWS_UNDERWATER;
        Lara.air = LARA_AIR;
        item->pos.y += 100;
        lara_float_pos.y += 100;
        item->gravity_status = 0;
        UpdateLaraRoom(item, 0);
        StopSoundEffect(SFX_LARA_FALL, NULL);
        if (item->current_anim_state == AS_SWANDIVE) {
            item->goal_anim_state = AS_DIVE;
            item->pos.x_rot = -45 * PHD_DEGREE;
            AnimateLara(item);
            item->fall_speed *= 2/ANIM_SCALE;
            lara_fall_speed_f *= 2.0/ANIM_SCALE;
        } else if (item->current_anim_state == AS_FASTDIVE) {
            item->goal_anim_state = AS_DIVE;
            item->pos.x_rot = -85 * PHD_DEGREE;
            AnimateLara(item);
            item->fall_speed *= 2/ANIM_SCALE;
            lara_fall_speed_f *= 2.0/ANIM_SCALE;
        } else {
            item->current_anim_state = AS_DIVE;
            item->goal_anim_state = AS_SWIM;
            item->anim_number = AA_JUMPIN;
            item->frame_number = AF_JUMPIN * ANIM_SCALE;
            item->pos.x_rot = -45 * PHD_DEGREE;
            item->fall_speed = ((item->fall_speed * 3) / 2) /ANIM_SCALE;
            lara_fall_speed_f *= 1.5/(double)ANIM_SCALE;
        }
        Lara.head_x_rot = 0;
        Lara.head_y_rot = 0;
        Lara.torso_x_rot = 0;
        Lara.torso_y_rot = 0;
        Splash(item);
    } else if (Lara.water_status == LWS_UNDERWATER && !room_submerged) {
        int16_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh != NO_HEIGHT && ABS(wh - item->pos.y) < STEP_L) {
            Lara.water_status = LWS_SURFACE;
            Lara.dive_count = DIVE_COUNT + 1;
            item->current_anim_state = AS_SURFTREAD;
            item->goal_anim_state = AS_SURFTREAD;
            item->anim_number = AA_SURFTREAD;
            item->frame_number = AF_SURFTREAD * ANIM_SCALE;
            item->fall_speed = 0;
            lara_fall_speed_f = 0.0;
            item->pos.y = wh + 1;
            lara_float_pos.y = wh + 1;
            item->pos.x_rot = 0;
            item->pos.z_rot = 0;
            Lara.head_x_rot = 0;
            Lara.head_y_rot = 0;
            Lara.torso_x_rot = 0;
            Lara.torso_y_rot = 0;
            UpdateLaraRoom(item, -LARA_HITE / 2);
            SoundEffect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        } else {
            Lara.water_status = LWS_ABOVEWATER;
            Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = AS_FORWARDJUMP;
            item->goal_anim_state = AS_FORWARDJUMP;
            item->anim_number = AA_FALLDOWN;
            item->frame_number = AF_FALLDOWN * ANIM_SCALE;
            item->speed = item->fall_speed / 4 ;
            lara_speed_F = lara_fall_speed_f / 4.0 ;
            LaraSpeedError = 0;
            item->fall_speed = 0;
            lara_fall_speed_f = 0.0;
            item->gravity_status = 1;
            item->pos.x_rot = 0;
            item->pos.z_rot = 0;
            Lara.head_x_rot = 0;
            Lara.head_y_rot = 0;
            Lara.torso_x_rot = 0;
            Lara.torso_y_rot = 0;
        }
    } else if (Lara.water_status == LWS_SURFACE && !room_submerged) {
        Lara.water_status = LWS_ABOVEWATER;
        Lara.gun_status = LGS_ARMLESS;
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->speed = item->fall_speed / 4;
        lara_speed_F = lara_fall_speed_f / 4.0;
        LaraSpeedError = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0;
        item->gravity_status = 1;
        item->pos.x_rot = 0;
        item->pos.z_rot = 0;
        Lara.head_x_rot = 0;
        Lara.head_y_rot = 0;
        Lara.torso_x_rot = 0;
        Lara.torso_y_rot = 0;
    }

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (!Lara.death_count) {
            S_MusicStop();
        }
        Lara.death_count++;
        // make sure the enemy healthbar is no longer rendered. If Lara later
        // is resurrected with DOZY, she should no longer aim at the target.
        Lara.target = NULL;
    }

    switch (Lara.water_status) {
    case LWS_ABOVEWATER:
        Lara.air = LARA_AIR;
        LaraAboveWater(item, &coll);
        break;

    case LWS_UNDERWATER:
        if (item->hit_points >= 0) {
            Lara.air--;
            if (Lara.air < 0) {
                Lara.air = -1;
                item->hit_points -= 5;
            }
        }
        LaraUnderWater(item, &coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            Lara.air += 10;
            if (Lara.air > LARA_AIR) {
                Lara.air = LARA_AIR;
            }
        }
        LaraSurface(item, &coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_HITPOINTS;
        Lara.death_count = 0;
        LaraUnderWater(item, &coll);
        if (CHK_ANY(Input, IN_SLOW)
            && !CHK_ANY(Input, IN_LOOK | IN_FLY_CHEAT)) {
            int16_t wh = GetWaterHeight(
                item->pos.x, item->pos.y, item->pos.z, item->room_number);
            if (room_submerged || (wh != NO_HEIGHT && wh > 0)) {
                Lara.water_status = LWS_UNDERWATER;
            } else {
                Lara.water_status = LWS_ABOVEWATER;
                item->anim_number = AA_STOP;
                item->frame_number = Anims[item->anim_number].frame_base;
                item->pos.x_rot = item->pos.z_rot = 0;
                Lara.head_x_rot = 0;
                Lara.head_y_rot = 0;
                Lara.torso_x_rot = 0;
                Lara.torso_y_rot = 0;
            }
            Lara.gun_status = LGS_ARMLESS;
        }
        break;
    }
}

void LaraSwapMeshExtra()
{
    if (!Objects[O_LARA_EXTRA].loaded) {
        return;
    }
    int i;
    for (i = 0; i < LM_NUMBER_OF; i++) {
        Lara.mesh_ptrs[i] = Meshes[Objects[O_LARA_EXTRA].mesh_index + i];
    }
}

int32_t frameCounter = 0;

void AnimateLara(ITEM_INFO *item)
{
    int16_t *command;
    ANIM_STRUCT *anim;

    item->frame_number++;
    anim = &Anims[item->anim_number];
    if (anim->number_changes > 0 && GetChange(item, anim)) {
        anim = &Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            command = &AnimCommands[anim->command_index];
            int i;
            for (i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    TranslateItem(item, command[0], command[1], command[2]);
                    TranslateItem_f(&lara_float_pos, command[0], command[1], command[2], item->pos.y_rot);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                {			
					int16_t dummy = *(command++);					// Get Upward Velocity
					item->fall_speed = dummy;
					lara_fall_speed_f = (double)dummy;		
                    dummy = *(command++);
                    item->speed = dummy/ANIM_SCALE;                 // Get Forward Velocity
                    lara_speed_F = (double)dummy/(double)ANIM_SCALE;
                    item->gravity_status = 1;                       // turn gravity ON
                    if ( Lara.calc_fall_speed )                     // If Upward velocity is  Pre-Calculated
                    {
                        item->fall_speed = Lara.calc_fall_speed;    // then use that Value instead..
                        lara_fall_speed_f = Lara.calc_fall_speed;
                        Lara.calc_fall_speed = 0;
                    }
                 }
                    break;

                case AC_ATTACK_READY:
                    Lara.gun_status = LGS_ARMLESS;
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    command += 2;
                    break;
                }
            }
        }

        item->anim_number = anim->jump_anim_num;
        item->frame_number = anim->jump_frame_num;

        anim = &Anims[anim->jump_anim_num];
        item->current_anim_state = anim->current_anim_state;
    }

    if (anim->number_commands > 0) {
        command = &AnimCommands[anim->command_index];
        int i;
        for (i = 0; i < anim->number_commands; i++) {
            switch (*command++) {
            case AC_MOVE_ORIGIN:
                command += 3;
                break;

            case AC_JUMP_VELOCITY:
                command += 2;
                break;

            case AC_SOUND_FX:
                if (item->frame_number == command[0]) {
                    SoundEffect(command[1], &item->pos, SPM_ALWAYS);
                }
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_number == command[0]) {
                    EffectRoutines[command[1]](item);
                }
                command += 2;
                break;
            }
        }
    }
	if ( ANIM_SCALE == 1) {
		if (item->gravity_status) {
			int16_t speed = anim->velocity + anim->acceleration * (item->frame_number - anim->frame_base - 1);
						
			item->speed -= (int16_t)(speed >> 16);
			speed += anim->acceleration;
			item->speed += (int16_t)(speed >> 16);

			item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
			item->pos.y += item->fall_speed;
		} else {
			int32_t speed = anim->velocity;
			if (anim->acceleration) {
				speed +=
					anim->acceleration * (item->frame_number - anim->frame_base);
			}
			item->speed = (int16_t)(speed >> 16);
		}
		
		item->pos.x += (phd_sin(Lara.move_angle) * item->speed) >> W2V_SHIFT;
		item->pos.z += (phd_cos(Lara.move_angle) * item->speed) >> W2V_SHIFT;
		
		lara_float_pos.x = item->pos.x;
		lara_float_pos.z = item->pos.z;
		lara_float_pos.y = item->pos.y;
		lara_fall_speed_f = item->fall_speed;
		lara_speed_F = item->speed;
	}
	else // ANIM_SCALE = 2
	{
		if (item->gravity_status) {
			
			double speed = (double)(anim->velocity + anim->acceleration)/ANIM_SCALE * (((item->frame_number - anim->frame_base) / (double)ANIM_SCALE) - 1.0);
			
			lara_speed_F -= speed/65536.0;
			speed += (double)anim->acceleration / (double)ANIM_SCALE;
			lara_speed_F += speed/65536.0;

			lara_fall_speed_f += (item->fall_speed < FASTFALL_SPEED) ? (double)GRAVITY /(double)ANIM_SCALE : 1.0/(double)ANIM_SCALE;
			
			lara_float_pos.y += lara_fall_speed_f / (double)ANIM_SCALE;

			item->fall_speed = lara_fall_speed_f;
			item->speed = lara_speed_F;
			frameCounter += 1;
			
		} else {
			frameCounter = 0;
			double speed = (double)anim->velocity/(double)ANIM_SCALE;
			LaraSpeedError = 0; //since speed is explicity set this is now 0
			if (anim->acceleration) {
				speed +=
					((double)anim->acceleration/(double)ANIM_SCALE) * ((item->frame_number - anim->frame_base) / (double)ANIM_SCALE);
			}
			lara_speed_F = speed / 65536.0;
			item->speed = lara_speed_F; //(int16_t)(speed >> 16);
			lara_jump_error = 0;
		}
		
		lara_float_pos.x += (phd_sin_f(Lara.move_angle) * lara_speed_F) / View2World;
		lara_float_pos.z += (phd_cos_f(Lara.move_angle) * lara_speed_F) / View2World;
		
		item->pos.x = lara_float_pos.x;
		item->pos.z = lara_float_pos.z;
		item->pos.y = lara_float_pos.y;
	}
#if 0   
    const double scale = 0.8;
    const int32_t text_height = 17 * scale;
    const int32_t text_offset_x = 0;
    const int32_t screen_margin_h = -20;
    const int32_t screen_margin_v = 18;

    char ammostring[80] = "";
    char speedString[80] = "";
    
    sprintf(ammostring,"%d,%d,%d - %d : %g,%g,%g",item->pos.x, item->pos.y, item->pos.z, LaraItem->pos.y_rot, lara_float_pos.x, lara_float_pos.y, lara_float_pos.z);
    sprintf(speedString,"%d - %g",item->speed, lara_speed_F);
    
    if (LaraText) {
        T_ChangeText(LaraText, ammostring);
        T_ChangeText(LaraText2, speedString);
    } else {
        LaraText = T_Print(
            -screen_margin_h - text_offset_x, text_height + screen_margin_v,
            ammostring);
        T_SetScale(LaraText, PHD_ONE * scale, PHD_ONE * scale);
        //T_RightAlign(LaraText, 1);
        
        LaraText2 = T_Print(
            -screen_margin_h - text_offset_x, (text_height*2) + screen_margin_v,
            ammostring);
        T_SetScale(LaraText2, PHD_ONE * scale, PHD_ONE * scale);
        //T_RightAlign(LaraText2, 1);
    }
#endif
}

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        AnimateLara(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void UseItem(int16_t object_num)
{
    //LOG_INFO("%d", object_num);
    switch (object_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
        Lara.request_gun_type = LGT_PISTOLS;
        if (Lara.gun_status == LGS_ARMLESS && Lara.gun_type == LGT_PISTOLS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        Lara.request_gun_type = LGT_SHOTGUN;
        if (Lara.gun_status == LGS_ARMLESS && Lara.gun_type == LGT_SHOTGUN) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        Lara.request_gun_type = LGT_MAGNUMS;
        if (Lara.gun_status == LGS_ARMLESS && Lara.gun_type == LGT_MAGNUMS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        Lara.request_gun_type = LGT_UZIS;
        if (Lara.gun_status == LGS_ARMLESS && Lara.gun_type == LGT_UZIS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        if (LaraItem->hit_points <= 0
            || LaraItem->hit_points >= LARA_HITPOINTS) {
            return;
        }
        LaraItem->hit_points += LARA_HITPOINTS / 2;
        if (LaraItem->hit_points > LARA_HITPOINTS) {
            LaraItem->hit_points = LARA_HITPOINTS;
        }
        Inv_RemoveItem(O_MEDI_ITEM);
        SoundEffect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        if (LaraItem->hit_points <= 0
            || LaraItem->hit_points >= LARA_HITPOINTS) {
            return;
        }
        LaraItem->hit_points = LaraItem->hit_points + LARA_HITPOINTS;
        if (LaraItem->hit_points > LARA_HITPOINTS) {
            LaraItem->hit_points = LARA_HITPOINTS;
        }
        Inv_RemoveItem(O_BIGMEDI_ITEM);
        SoundEffect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;
    }
}

void ControlLaraExtra(int16_t item_num)
{
    AnimateItem(&Items[item_num]);
}

void InitialiseLaraLoad(int16_t item_num)
{
    Lara.item_number = item_num;
    LaraItem = &Items[item_num];
    LaraSetFloatPosFromFixed();
}

void InitialiseLara()
{
    LaraItem->collidable = 0;
    LaraItem->data = &Lara;
    LaraItem->hit_points = LARA_HITPOINTS;
    if (T1MConfig.disable_healing_between_levels) {
        LaraItem->hit_points = StoredLaraHealth;
    }

    Lara.air = LARA_AIR;
    Lara.torso_y_rot = 0;
    Lara.torso_x_rot = 0;
    Lara.torso_z_rot = 0;
    Lara.head_y_rot = 0;
    Lara.head_x_rot = 0;
    Lara.head_z_rot = 0;
    Lara.calc_fall_speed = 0;
    Lara.mesh_effects = 0;
    Lara.hit_frame = 0;
    Lara.hit_direction = 0;
    Lara.death_count = 0;
    Lara.target = NULL;
    Lara.spaz_effect = NULL;
    Lara.spaz_effect_count = 0;
    Lara.turn_rate = 0;
    Lara.move_angle = 0;
    Lara.right_arm.flash_gun = 0;
    Lara.left_arm.flash_gun = 0;
    Lara.right_arm.lock = 0;
    Lara.left_arm.lock = 0;

    if (RoomInfo[LaraItem->room_number].flags & 1) {
        Lara.water_status = LWS_UNDERWATER;
        LaraItem->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        LaraItem->goal_anim_state = AS_TREAD;
        LaraItem->current_anim_state = AS_TREAD;
        LaraItem->anim_number = AA_TREAD;
        LaraItem->frame_number = AF_TREAD * ANIM_SCALE;
    } else {
        Lara.water_status = LWS_ABOVEWATER;
        LaraItem->goal_anim_state = AS_STOP;
        LaraItem->current_anim_state = AS_STOP;
        LaraItem->anim_number = AA_STOP;
        LaraItem->frame_number = AF_STOP * ANIM_SCALE;
    }

    Lara.current_active = 0;

    InitialiseLOT(&Lara.LOT);
    Lara.LOT.step = WALL_L * 20;
    Lara.LOT.drop = -WALL_L * 20;
    Lara.LOT.fly = STEP_L;

    InitialiseLaraInventory(CurrentLevel);
}

void InitialiseLaraInventory(int32_t level_num)
{
    Inv_RemoveAllItems();

    START_INFO *start = &SaveGame.start[level_num];

    Lara.pistols.ammo = 1000;
    if (start->got_pistols) {
        Inv_AddItem(O_GUN_ITEM);
    }

    if (start->got_magnums) {
        Inv_AddItem(O_MAGNUM_ITEM);
        Lara.magnums.ammo = start->magnum_ammo;
        GlobalItemReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
    } else {
        int32_t ammo = start->magnum_ammo / MAGNUM_AMMO_QTY;
        int i;
        for (i = 0; i < ammo; i++) {
            Inv_AddItem(O_MAG_AMMO_ITEM);
        }
        Lara.magnums.ammo = 0;
    }

    if (start->got_uzis) {
        Inv_AddItem(O_UZI_ITEM);
        Lara.uzis.ammo = start->uzi_ammo;
        GlobalItemReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
    } else {
        int32_t ammo = start->uzi_ammo / UZI_AMMO_QTY;
        int i;
        for (i = 0; i < ammo; i++) {
            Inv_AddItem(O_UZI_AMMO_ITEM);
        }
        Lara.uzis.ammo = 0;
    }

    if (start->got_shotgun) {
        Inv_AddItem(O_SHOTGUN_ITEM);
        Lara.shotgun.ammo = start->shotgun_ammo;
        GlobalItemReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
    } else {
        int32_t ammo = start->shotgun_ammo / SHOTGUN_AMMO_QTY;
        int i;
        for (i = 0; i < ammo; i++) {
            Inv_AddItem(O_SG_AMMO_ITEM);
        }
        Lara.shotgun.ammo = 0;
    }
    int i;
    for (i = 0; i < start->num_scions; i++) {
        Inv_AddItem(O_SCION_ITEM);
    }

    for (i = 0; i < start->num_medis; i++) {
        Inv_AddItem(O_MEDI_ITEM);
    }

    for (i = 0; i < start->num_big_medis; i++) {
        Inv_AddItem(O_BIGMEDI_ITEM);
    }

    Lara.gun_status = start->gun_status;
    Lara.gun_type = start->gun_type;
    Lara.request_gun_type = start->gun_type;

    LaraInitialiseMeshes(level_num);
    InitialiseNewWeapon();
}

void LaraInitialiseMeshes(int32_t level_num)
{
    START_INFO *start = &SaveGame.start[level_num];
    int i;

    if (start->costume) {        
        for (i = 0; i < LM_NUMBER_OF; i++) {
            int32_t use_orig_mesh = i == LM_HEAD;
            Lara.mesh_ptrs[i] = Meshes
                [Objects[use_orig_mesh ? O_LARA : O_LARA_EXTRA].mesh_index + i];
        }
        return;
    }

    for (i = 0; i < LM_NUMBER_OF; i++) {
        Lara.mesh_ptrs[i] = Meshes[Objects[O_LARA].mesh_index + i];
    }

    int16_t gun_type = start->gun_type;
    if (gun_type == LGT_SHOTGUN) {
        if (start->got_uzis || start->got_magnums || start->got_pistols) {
            gun_type = LGT_PISTOLS;
        } else {
            gun_type = LGT_UNARMED;
        }
    }

    int16_t holster_object_num = -1;
    switch (gun_type) {
    case LGT_PISTOLS:
        holster_object_num = O_PISTOLS;
        break;
    case LGT_MAGNUMS:
        holster_object_num = O_MAGNUM;
        break;
    case LGT_UZIS:
        holster_object_num = O_UZI;
        break;
    }

    int16_t back_object_num = -1;
    if (start->got_shotgun) {
        back_object_num = O_SHOTGUN;
    }

    if (holster_object_num != -1) {
        Lara.mesh_ptrs[LM_THIGH_L] =
            Meshes[Objects[holster_object_num].mesh_index + LM_THIGH_L];
        Lara.mesh_ptrs[LM_THIGH_R] =
            Meshes[Objects[holster_object_num].mesh_index + LM_THIGH_R];
    }

    if (back_object_num != -1) {
        Lara.mesh_ptrs[LM_TORSO] =
            Meshes[Objects[back_object_num].mesh_index + LM_TORSO];
    }
}

void LaraCheatGetStuff()
{
    if (CurrentLevel == GF.gym_level_num) {
        return;
    }

    // play istols drawing sound
    SoundEffect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);

    if (Objects[O_GUN_OPTION].loaded && !Inv_RequestItem(O_GUN_ITEM)) {
        Inv_AddItem(O_GUN_ITEM);
    }

    if (Objects[O_SHOTGUN_OPTION].loaded) {
        if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
            Inv_AddItem(O_SHOTGUN_ITEM);
        }
        Lara.shotgun.ammo = SaveGame.bonus_flag & GBF_NGPLUS ? 10001 : 300;
    }

    if (Objects[O_MAGNUM_OPTION].loaded) {
        if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
            Inv_AddItem(O_MAGNUM_ITEM);
        }
        Lara.magnums.ammo = SaveGame.bonus_flag & GBF_NGPLUS ? 10001 : 1000;
    }

    if (Objects[O_UZI_OPTION].loaded) {
        if (!Inv_RequestItem(O_UZI_ITEM)) {
            Inv_AddItem(O_UZI_ITEM);
        }
        Lara.uzis.ammo = SaveGame.bonus_flag & GBF_NGPLUS ? 10001 : 2000;
    }

    int i;
    for (i = 0; i < 10; i++) {
        if (Objects[O_MEDI_OPTION].loaded
            && Inv_RequestItem(O_MEDI_ITEM) < 240) {
            Inv_AddItem(O_MEDI_ITEM);
        }
        if (Objects[O_BIGMEDI_OPTION].loaded
            && Inv_RequestItem(O_BIGMEDI_ITEM) < 240) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }
    }

    if (Objects[O_KEY_OPTION1].loaded && !Inv_RequestItem(O_KEY_ITEM1)) {
        Inv_AddItem(O_KEY_ITEM1);
    }
    if (Objects[O_KEY_OPTION2].loaded && !Inv_RequestItem(O_KEY_ITEM2)) {
        Inv_AddItem(O_KEY_ITEM2);
    }
    if (Objects[O_KEY_OPTION3].loaded && !Inv_RequestItem(O_KEY_ITEM3)) {
        Inv_AddItem(O_KEY_ITEM3);
    }
    if (Objects[O_KEY_OPTION4].loaded && !Inv_RequestItem(O_KEY_ITEM4)) {
        Inv_AddItem(O_KEY_ITEM4);
    }
    if (Objects[O_PUZZLE_OPTION1].loaded && !Inv_RequestItem(O_PUZZLE_ITEM1)) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }
    if (Objects[O_PUZZLE_OPTION2].loaded && !Inv_RequestItem(O_PUZZLE_ITEM2)) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }
    if (Objects[O_PUZZLE_OPTION3].loaded && !Inv_RequestItem(O_PUZZLE_ITEM3)) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }
    if (Objects[O_PUZZLE_OPTION4].loaded && !Inv_RequestItem(O_PUZZLE_ITEM4)) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }
    if (Objects[O_PICKUP_OPTION1].loaded && !Inv_RequestItem(O_PICKUP_ITEM1)) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }
    if (Objects[O_PICKUP_OPTION2].loaded && !Inv_RequestItem(O_PICKUP_ITEM2)) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }
}

void LaraSetFloatPosFromFixed() {
	lara_float_pos.x = LaraItem->pos.x;
    lara_float_pos.y = LaraItem->pos.y;
    lara_float_pos.z = LaraItem->pos.z;
    lara_fall_speed_f = LaraItem->fall_speed;
	lara_speed_F = LaraItem->speed;
	//LOG_DEBUG("%d,%d,%d", LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z);
}

void (*LaraControlRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
    LaraAsWalk,      LaraAsRun,       LaraAsStop,      LaraAsForwardJump,
    LaraAsPose,      LaraAsFastBack,  LaraAsTurnR,     LaraAsTurnL,
    LaraAsDeath,     LaraAsFastFall,  LaraAsHang,      LaraAsReach,
    LaraAsSplat,     LaraAsTread,     LaraAsLand,      LaraAsCompress,
    LaraAsBack,      LaraAsSwim,      LaraAsGlide,     LaraAsNull,
    LaraAsFastTurn,  LaraAsStepRight, LaraAsStepLeft,  LaraAsRoll2,
    LaraAsSlide,     LaraAsBackJump,  LaraAsRightJump, LaraAsLeftJump,
    LaraAsUpJump,    LaraAsFallBack,  LaraAsHangLeft,  LaraAsHangRight,
    LaraAsSlideBack, LaraAsSurfTread, LaraAsSurfSwim,  LaraAsDive,
    LaraAsPushBlock, LaraAsPullBlock, LaraAsPPReady,   LaraAsPickup,
    LaraAsSwitchOn,  LaraAsSwitchOff, LaraAsUseKey,    LaraAsUsePuzzle,
    LaraAsUWDeath,   LaraAsRoll,      LaraAsSpecial,   LaraAsSurfBack,
    LaraAsSurfLeft,  LaraAsSurfRight, LaraAsUseMidas,  LaraAsDieMidas,
    LaraAsSwanDive,  LaraAsFastDive,  LaraAsGymnast,   LaraAsWaterOut,
};

void (*LaraCollisionRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
    LaraColWalk,      LaraColRun,       LaraColStop,      LaraColForwardJump,
    LaraColPose,      LaraColFastBack,  LaraColTurnR,     LaraColTurnL,
    LaraColDeath,     LaraColFastFall,  LaraColHang,      LaraColReach,
    LaraColSplat,     LaraColTread,     LaraColLand,      LaraColCompress,
    LaraColBack,      LaraColSwim,      LaraColGlide,     LaraColNull,
    LaraColFastTurn,  LaraColStepRight, LaraColStepLeft,  LaraColRoll2,
    LaraColSlide,     LaraColBackJump,  LaraColRightJump, LaraColLeftJump,
    LaraColUpJump,    LaraColFallBack,  LaraColHangLeft,  LaraColHangRight,
    LaraColSlideBack, LaraColSurfTread, LaraColSurfSwim,  LaraColDive,
    LaraColPushBlock, LaraColPullBlock, LaraColPPReady,   LaraColPickup,
    LaraColSwitchOn,  LaraColSwitchOff, LaraColUseKey,    LaraColUsePuzzle,
    LaraColUWDeath,   LaraColRoll,      LaraColSpecial,   LaraColSurfBack,
    LaraColSurfLeft,  LaraColSurfRight, LaraColUseMidas,  LaraColDieMidas,
    LaraColSwanDive,  LaraColFastDive,  LaraColGymnast,   LaraColWaterOut,
};

void T1MInjectGameLaraMisc()
{
    INJECT(0x00427850, LaraControl);
    INJECT(0x00427BD0, LaraSwapMeshExtra);
    INJECT(0x00427C00, AnimateLara);
    INJECT(0x00427E80, UseItem);
    INJECT(0x00427FD0, ControlLaraExtra);
    INJECT(0x00427FF0, InitialiseLaraLoad);
    INJECT(0x00428020, InitialiseLara);
    INJECT(0x00428170, InitialiseLaraInventory);
    INJECT(0x00428340, LaraInitialiseMeshes);
}
