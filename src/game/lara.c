#include "game/lara.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects/twinkle.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stddef.h>

void LookLeftRight()
{
    Camera.type = CAM_LOOK;
    if (Input & IN_LEFT) {
        Input -= IN_LEFT;
        if (Lara.head_y_rot > -MAX_HEAD_ROTATION) {
            Lara.head_y_rot -= HEAD_TURN/ANIM_SCALE / 2;
        }
    } else if (Input & IN_RIGHT) {
        Input -= IN_RIGHT;
        if (Lara.head_y_rot < MAX_HEAD_ROTATION) {
            Lara.head_y_rot += HEAD_TURN/ANIM_SCALE / 2;
        }
    }
    if (Lara.gun_status != LGS_HANDSBUSY) {
        Lara.torso_y_rot = Lara.head_y_rot;
    }
}

void LookUpDown()
{
    Camera.type = CAM_LOOK;
    if (Input & IN_FORWARD) {
        Input -= IN_FORWARD;
        if (Lara.head_x_rot > MIN_HEAD_TILT_LOOK) {
            Lara.head_x_rot -= HEAD_TURN/ANIM_SCALE / 2;
        }
    } else if (Input & IN_BACK) {
        Input -= IN_BACK;
        if (Lara.head_x_rot < MAX_HEAD_TILT_LOOK) {
            Lara.head_x_rot += HEAD_TURN/ANIM_SCALE / 2;
        }
    }
    if (Lara.gun_status != LGS_HANDSBUSY) {
        Lara.torso_x_rot = Lara.head_x_rot;
    }
}

void ResetLook()
{
    if (Camera.type == CAM_LOOK) {
        return;
    }
    if (Lara.head_x_rot <= -HEAD_TURN / 2 || Lara.head_x_rot >= HEAD_TURN / 2) {
        Lara.head_x_rot = Lara.head_x_rot / -8 + Lara.head_x_rot;
    } else {
        Lara.head_x_rot = 0;
    }
    Lara.torso_x_rot = Lara.head_x_rot;

    if (Lara.head_y_rot <= -HEAD_TURN / 2 || Lara.head_y_rot >= HEAD_TURN / 2) {
        Lara.head_y_rot += Lara.head_y_rot / -8;
    } else {
        Lara.head_y_rot = 0;
    }
    Lara.torso_y_rot = Lara.head_y_rot;
}

void LaraAboveWater(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RAD;
    coll->trigger = NULL;

    coll->lava_is_pit = 0;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->enable_spaz = 1;
    coll->enable_baddie_push = 1;

    if (T1MConfig.enable_enhanced_look && item->hit_points > 0) {
        if (Input & IN_LOOK) {
            LookLeftRight();
        } else {
            ResetLook();
        }
    }

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (Camera.type != CAM_LOOK) {
        if (Lara.head_x_rot > -HEAD_TURN / 2
            && Lara.head_x_rot < HEAD_TURN / 2) {
            Lara.head_x_rot = 0;
        } else {
            Lara.head_x_rot -= Lara.head_x_rot / 8;
        }
        Lara.torso_x_rot = Lara.head_x_rot;

        if (Lara.head_y_rot > -HEAD_TURN / 2
            && Lara.head_y_rot < HEAD_TURN / 2) {
            Lara.head_y_rot = 0;
        } else {
            Lara.head_y_rot -= Lara.head_y_rot / 8;
        }
        Lara.torso_y_rot = Lara.head_y_rot;
    }

    if (item->pos.z_rot >= -LARA_LEAN_UNDO
        && item->pos.z_rot <= LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < -LARA_LEAN_UNDO) {
        item->pos.z_rot += LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= LARA_LEAN_UNDO;
    }

    if (Lara.turn_rate >= -LARA_TURN_UNDO && Lara.turn_rate <= LARA_TURN_UNDO) {
        Lara.turn_rate = 0;
    } else if (Lara.turn_rate < -LARA_TURN_UNDO) {
        Lara.turn_rate += LARA_TURN_UNDO;
    } else {
        Lara.turn_rate -= LARA_TURN_UNDO;
    }
    item->pos.y_rot += Lara.turn_rate;
    /*if( Lara.turn_rate == 0)
    {
		int lockAngle = (item->pos.y_rot / PHD_DEGREE) * PHD_DEGREE;
		item->pos.y_rot = lockAngle; 
	}*/

    AnimateLara(item);
    LaraBaddieCollision(item, coll);
    LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, -LARA_HITE / 2);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

void LaraAsWalk(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (!T1MConfig.enable_tr3_sidesteps
        || (CHK_ANY(Input, IN_SLOW) && CHK_ANY(Input, IN_FORWARD))) {
        if (Input & IN_LEFT) {
            Lara.turn_rate -= LARA_TURN_RATE;
            if (Lara.turn_rate < -LARA_SLOW_TURN) {
                Lara.turn_rate = -LARA_SLOW_TURN;
            }
        } else if (Input & IN_RIGHT) {
            Lara.turn_rate += LARA_TURN_RATE;
            if (Lara.turn_rate > LARA_SLOW_TURN) {
                Lara.turn_rate = LARA_SLOW_TURN;
            }
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else {
        item->goal_anim_state = AS_STOP;
    }
}

void LaraAsRun(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_DEATH;
        return;
    }

    if (Input & IN_ROLL) {
        item->current_anim_state = AS_ROLL;
        item->goal_anim_state = AS_STOP;
        item->anim_number = AA_ROLL;
        item->frame_number = AF_ROLL *ANIM_SCALE;
        return;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_FAST_TURN) {
            Lara.turn_rate = -LARA_FAST_TURN;
        }
        item->pos.z_rot -= LARA_LEAN_RATE;
        if (item->pos.z_rot < -LARA_LEAN_MAX) {
            item->pos.z_rot = -LARA_LEAN_MAX;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_FAST_TURN) {
            Lara.turn_rate = LARA_FAST_TURN;
        }
        item->pos.z_rot += LARA_LEAN_RATE;
        if (item->pos.z_rot > LARA_LEAN_MAX) {
            item->pos.z_rot = LARA_LEAN_MAX;
        }
    }

    if ((Input & IN_JUMP) && !item->gravity_status) {
        item->goal_anim_state = AS_FORWARDJUMP;
    } else if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else {
        item->goal_anim_state = AS_STOP;
    }
}

void LaraAsStop(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_DEATH;
        return;
    }

    if (Input & IN_ROLL) {
        item->current_anim_state = AS_ROLL;
        item->goal_anim_state = AS_STOP;
        item->anim_number = AA_ROLL;
        item->frame_number = AF_ROLL *ANIM_SCALE;
        return;
    }

    item->goal_anim_state = AS_STOP;
    if (Input & IN_LOOK) {
        Camera.type = CAM_LOOK;
        if ((Input & IN_LEFT) && Lara.head_y_rot > -MAX_HEAD_ROTATION) {
            Lara.head_y_rot -= HEAD_TURN / 2;
        } else if ((Input & IN_RIGHT) && Lara.head_y_rot < MAX_HEAD_ROTATION) {
            Lara.head_y_rot += HEAD_TURN / 2;
        }
        Lara.torso_y_rot = Lara.head_y_rot;

        if ((Input & IN_FORWARD) && Lara.head_x_rot > MIN_HEAD_TILT_LOOK) {
            Lara.head_x_rot -= HEAD_TURN / 2 / ANIM_SCALE;
        } else if ((Input & IN_BACK) && Lara.head_x_rot < MAX_HEAD_TILT_LOOK) {
            Lara.head_x_rot += HEAD_TURN / 2 / ANIM_SCALE;
        }
        Lara.torso_x_rot = Lara.head_x_rot;
        return;
    }
    if (Camera.type == CAM_LOOK) {
        Camera.type = CAM_CHASE;
    }

    if (Input & IN_STEPL) {
        item->goal_anim_state = AS_STEPLEFT;
    } else if (Input & IN_STEPR) {
        item->goal_anim_state = AS_STEPRIGHT;
    }

    if (Input & IN_LEFT) {
        if (T1MConfig.enable_tr3_sidesteps && (Input & IN_SLOW)) {
            item->goal_anim_state = AS_STEPLEFT;
        } else {
            item->goal_anim_state = AS_TURN_L;
        }
    } else if (Input & IN_RIGHT) {
        if (T1MConfig.enable_tr3_sidesteps && (Input & IN_SLOW)) {
            item->goal_anim_state = AS_STEPRIGHT;
        } else {
            item->goal_anim_state = AS_TURN_R;
        }
    }

    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_COMPRESS;
    } else if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            LaraAsWalk(item, coll);
        } else {
            LaraAsRun(item, coll);
        }
    } else if (Input & IN_BACK) {
        if (Input & IN_SLOW) {
            LaraAsBack(item, coll);
        } else {
            item->goal_anim_state = AS_FASTBACK;
        }
    }
}

void LaraAsForwardJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->goal_anim_state == AS_SWANDIVE
        || item->goal_anim_state == AS_REACH) {
        item->goal_anim_state = AS_FORWARDJUMP;
    }
    if (item->goal_anim_state != AS_DEATH && item->goal_anim_state != AS_STOP) {
        if ((Input & IN_ACTION) && Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = AS_REACH;
        }
        if ((Input & IN_SLOW) && Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = AS_SWANDIVE;
        }
        if (item->fall_speed > LARA_FASTFALL_SPEED) {
            item->goal_anim_state = AS_FASTFALL;
        }
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_JUMP_TURN) {
            Lara.turn_rate = -LARA_JUMP_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_JUMP_TURN) {
            Lara.turn_rate = LARA_JUMP_TURN;
        }
    }
}

void LaraAsPose(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraAsFastBack(ITEM_INFO *item, COLL_INFO *coll)
{
    item->goal_anim_state = AS_STOP;
    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_MED_TURN) {
            Lara.turn_rate = -LARA_MED_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_MED_TURN) {
            Lara.turn_rate = LARA_MED_TURN;
        }
    }
}

void LaraAsTurnR(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_enhanced_look && (Input & IN_LOOK)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_tr3_sidesteps && CHK_ANY(Input, IN_SLOW)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate += LARA_TURN_RATE;
    if (Lara.gun_status == LGS_READY) {
        item->goal_anim_state = AS_FASTTURN;
    } else if (Lara.turn_rate > LARA_SLOW_TURN) {
        if (Input & IN_SLOW) {
            Lara.turn_rate = LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = AS_FASTTURN;
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else if (!(Input & IN_RIGHT)) {
        item->goal_anim_state = AS_STOP;
    }
}

void LaraAsTurnL(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_enhanced_look && (Input & IN_LOOK)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_tr3_sidesteps && CHK_ANY(Input, IN_SLOW)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate -= LARA_TURN_RATE;
    if (Lara.gun_status == LGS_READY) {
        item->goal_anim_state = AS_FASTTURN;
    } else if (Lara.turn_rate < -LARA_SLOW_TURN) {
        if (Input & IN_SLOW) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = AS_FASTTURN;
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else if (!(Input & IN_LEFT)) {
        item->goal_anim_state = AS_STOP;
    }
}

void LaraAsDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void LaraAsFastFall(ITEM_INFO *item, COLL_INFO *coll)
{
    item->speed = (item->speed * 95) / 100;
    lara_speed_F *= 0.95;
    LaraSpeedError = 0;
    if (item->fall_speed >= DAMAGE_START + DAMAGE_LENGTH) {
        SoundEffect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

void LaraAsHang(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (CHK_ANY(Input, IN_LEFT | IN_STEPL)) {
        item->goal_anim_state = AS_HANGLEFT;
    } else if (CHK_ANY(Input, IN_RIGHT | IN_STEPR)) {
        item->goal_anim_state = AS_HANGRIGHT;
    }
}

void LaraAsReach(ITEM_INFO *item, COLL_INFO *coll)
{
    Camera.target_angle = 85 * PHD_DEGREE;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsSplat(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraAsLand(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraAsCompress(ITEM_INFO *item, COLL_INFO *coll)
{
    if ((Input & IN_FORWARD)
        && LaraFloorFront(item, item->pos.y_rot, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_FORWARDJUMP;
        Lara.move_angle = item->pos.y_rot;
    } else if (
        (Input & IN_LEFT)
        && LaraFloorFront(item, item->pos.y_rot - PHD_90, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_LEFTJUMP;
        Lara.move_angle = item->pos.y_rot - PHD_90;
    } else if (
        (Input & IN_RIGHT)
        && LaraFloorFront(item, item->pos.y_rot + PHD_90, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_RIGHTJUMP;
        Lara.move_angle = item->pos.y_rot + PHD_90;
    } else if (
        (Input & IN_BACK)
        && LaraFloorFront(item, item->pos.y_rot - PHD_180, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_BACKJUMP;
        Lara.move_angle = item->pos.y_rot - PHD_180;
    }

    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if ((Input & IN_BACK) && (Input & IN_SLOW)) {
        item->goal_anim_state = AS_BACK;
    } else {
        item->goal_anim_state = AS_STOP;
    }

    if (!T1MConfig.enable_tr3_sidesteps
        || (CHK_ANY(Input, IN_SLOW) && CHK_ANY(Input, IN_BACK))) {
        if (Input & IN_LEFT) {
            Lara.turn_rate -= LARA_TURN_RATE;
            if (Lara.turn_rate < -LARA_SLOW_TURN) {
                Lara.turn_rate = -LARA_SLOW_TURN;
            }
        } else if (Input & IN_RIGHT) {
            Lara.turn_rate += LARA_TURN_RATE;
            if (Lara.turn_rate > LARA_SLOW_TURN) {
                Lara.turn_rate = LARA_SLOW_TURN;
            }
        }
    }
}

void LaraAsFastTurn(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_enhanced_look && (Input & IN_LOOK)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_tr3_sidesteps && CHK_ANY(Input, IN_SLOW)) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (Lara.turn_rate >= 0) {
        Lara.turn_rate = LARA_FAST_TURN;
        if (!(Input & IN_RIGHT)) {
            item->goal_anim_state = AS_STOP;
        }
    } else {
        Lara.turn_rate = -LARA_FAST_TURN;
        if (!(Input & IN_LEFT)) {
            item->goal_anim_state = AS_STOP;
        }
    }
}

void LaraAsStepRight(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_tr3_sidesteps && CHK_ALL(Input, IN_RIGHT | IN_SLOW)) {
        return;
    }

    if (!(Input & IN_STEPR)) {
        item->goal_anim_state = AS_STOP;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void LaraAsStepLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (T1MConfig.enable_tr3_sidesteps && CHK_ALL(Input, IN_LEFT | IN_SLOW)) {
        return;
    }

    if (!(Input & IN_STEPL)) {
        item->goal_anim_state = AS_STOP;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void LaraAsSlide(ITEM_INFO *item, COLL_INFO *coll)
{
    Camera.flags = NO_CHUNKY;
    Camera.target_elevation = -45 * PHD_DEGREE;
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_FORWARDJUMP;
    }
}

void LaraAsBackJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Camera.target_angle = PHD_DEGREE * 135;
    if (item->fall_speed > LARA_FASTFALL_SPEED * ANIM_SCALE) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsRightJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED * ANIM_SCALE) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsLeftJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED * ANIM_SCALE) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsUpJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED * 2/ANIM_SCALE) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void LaraAsFallBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
    if ((Input & IN_ACTION) && Lara.gun_status == LGS_ARMLESS) {
        item->goal_anim_state = AS_REACH;
    }
}

void LaraAsHangLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (!(Input & IN_LEFT) && !(Input & IN_STEPL)) {
        item->goal_anim_state = AS_HANG;
    }
}

void LaraAsHangRight(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (!(Input & IN_RIGHT) && !(Input & IN_STEPR)) {
        item->goal_anim_state = AS_HANG;
    }
}

void LaraAsSlideBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_BACKJUMP;
    }
}

void LaraAsPushBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 35 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
}

void LaraAsPullBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 35 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
}

void LaraAsPPReady(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 75 * PHD_DEGREE;
    if (!(Input & IN_ACTION)) {
        item->goal_anim_state = AS_STOP;
    }
}

void LaraAsPickup(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -130 * PHD_DEGREE;
    Camera.target_elevation = -15 * PHD_DEGREE;
    Camera.target_distance = WALL_L;
}

void LaraAsSwitchOn(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 80 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
    Camera.target_distance = WALL_L;
}

void LaraAsSwitchOff(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 80 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
    Camera.target_distance = WALL_L;
}

void LaraAsUseKey(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -80 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
    Camera.target_distance = WALL_L;
}

void LaraAsUsePuzzle(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -80 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
    Camera.target_distance = WALL_L;
}

void LaraAsRoll(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraAsRoll2(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraAsSpecial(ITEM_INFO *item, COLL_INFO *coll)
{
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 170 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
}

void LaraAsUseMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    ItemSparkle(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void LaraAsDieMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    int frm = item->frame_number - Anims[item->anim_number].frame_base;
    switch (frm) {
    case 5:
        Lara.mesh_effects |= (1 << LM_FOOT_L);
        Lara.mesh_effects |= (1 << LM_FOOT_R);
        Lara.mesh_ptrs[LM_FOOT_L] =
            Meshes[Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_L];
        Lara.mesh_ptrs[LM_FOOT_R] =
            Meshes[Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_R];
        break;

    case 70:
        Lara.mesh_effects |= (1 << LM_CALF_L);
        Lara.mesh_ptrs[LM_CALF_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_L];
        break;

    case 90:
        Lara.mesh_effects |= (1 << LM_THIGH_L);
        Lara.mesh_ptrs[LM_THIGH_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_L];
        break;

    case 100:
        Lara.mesh_effects |= (1 << LM_CALF_R);
        Lara.mesh_ptrs[LM_CALF_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_R];
        break;

    case 120:
        Lara.mesh_effects |= (1 << LM_HIPS);
        Lara.mesh_effects |= (1 << LM_THIGH_R);
        Lara.mesh_ptrs[LM_HIPS] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HIPS];
        Lara.mesh_ptrs[LM_THIGH_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_R];
        break;

    case 135:
        Lara.mesh_effects |= (1 << LM_TORSO);
        Lara.mesh_ptrs[LM_TORSO] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_TORSO];
        break;

    case 150:
        Lara.mesh_effects |= (1 << LM_UARM_L);
        Lara.mesh_ptrs[LM_UARM_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_L];
        break;

    case 163:
        Lara.mesh_effects |= (1 << LM_LARM_L);
        Lara.mesh_ptrs[LM_LARM_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_L];
        break;

    case 174:
        Lara.mesh_effects |= (1 << LM_HAND_L);
        Lara.mesh_ptrs[LM_HAND_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_L];
        break;

    case 186:
        Lara.mesh_effects |= (1 << LM_UARM_R);
        Lara.mesh_ptrs[LM_UARM_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_R];
        break;

    case 195:
        Lara.mesh_effects |= (1 << LM_LARM_R);
        Lara.mesh_ptrs[LM_LARM_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_R];
        break;

    case 218:
        Lara.mesh_effects |= (1 << LM_HAND_R);
        Lara.mesh_ptrs[LM_HAND_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_R];
        break;

    case 225:
        Lara.mesh_effects |= (1 << LM_HEAD);
        Lara.mesh_ptrs[LM_HEAD] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HEAD];
        break;
    }

    ItemSparkle(item, Lara.mesh_effects);
}

void LaraAsSwanDive(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTDIVE;
    }
}

void LaraAsFastDive(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    item->speed = (item->speed * 95) / 100;
    lara_speed_F *= 0.95;
    LaraSpeedError = 0;
}

void LaraAsNull(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void LaraAsGymnast(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void LaraAsWaterOut(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
}

void LaraColWalk(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->lava_is_pit = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }
    if (TestLaraVault(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        if (item->frame_number >= (29*ANIM_SCALE) && item->frame_number <= (47*ANIM_SCALE)) {
            item->anim_number = AA_STOP_RIGHT;
            item->frame_number = AF_STOP_RIGHT * ANIM_SCALE;
        } else if (
            (item->frame_number >= (22*ANIM_SCALE) && item->frame_number <= (28*ANIM_SCALE))
            || (item->frame_number >= (48*ANIM_SCALE) && item->frame_number <= (57*ANIM_SCALE))) {
            item->anim_number = AA_STOP_LEFT;
            item->frame_number = AF_STOP_LEFT * ANIM_SCALE;
        } else {
            item->anim_number = AA_STOP;
            item->frame_number = AF_STOP * ANIM_SCALE;
        }
    }

    if (coll->mid_floor > STEPUP_HEIGHT) {
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    if (coll->mid_floor > STEP_L / 2) {
        if (item->frame_number >= (28*ANIM_SCALE) && item->frame_number <= (45*ANIM_SCALE)) {
            item->anim_number = AA_WALKSTEPD_RIGHT;
            item->frame_number = AF_WALKSTEPD_RIGHT * ANIM_SCALE;
        } else {
            item->anim_number = AA_WALKSTEPD_LEFT;
            item->frame_number = AF_WALKSTEPD_LEFT * ANIM_SCALE;
        }
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_number >= (27*ANIM_SCALE) && item->frame_number <= (44*ANIM_SCALE)) {
            item->anim_number = AA_WALKSTEPUP_RIGHT;
            item->frame_number = AF_WALKSTEPUP_RIGHT * ANIM_SCALE;
        } else {
            item->anim_number = AA_WALKSTEPUP_LEFT;
            item->frame_number = AF_WALKSTEPUP_LEFT * ANIM_SCALE;
        }
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColRun(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }
    if (TestLaraVault(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->pos.z_rot = 0;

        if (coll->front_type == HT_WALL
            && coll->front_floor < -(STEP_L * 5) / 2) {
            item->current_anim_state = AS_SPLAT;
            if (item->frame_number >= 0 && item->frame_number <= (9*ANIM_SCALE)) {
                item->anim_number = AA_HITWALLLEFT;
                item->frame_number = AF_HITWALLLEFT * ANIM_SCALE;
                return;
            }
            if (item->frame_number >= (10*ANIM_SCALE) && item->frame_number <= (21*ANIM_SCALE)) {
                item->anim_number = AA_HITWALLRIGHT;
                item->frame_number = AF_HITWALLRIGHT * ANIM_SCALE;
                return;
            }
        }
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
    }

    if (coll->mid_floor > STEPUP_HEIGHT) {
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_number >= (3*ANIM_SCALE) && item->frame_number <= (14*ANIM_SCALE)) {
            item->anim_number = AA_RUNSTEPUP_LEFT;
            item->frame_number = AF_RUNSTEPUP_LEFT * ANIM_SCALE;
        } else {
            item->anim_number = AA_RUNSTEPUP_RIGHT;
            item->frame_number = AF_RUNSTEPUP_RIGHT * ANIM_SCALE;
        }
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor >= 50) {
        item->pos.y += 50;
        lara_float_pos.y +=  50.0f;
    } else {
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
    }
}

void LaraColStop(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 100) {
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    ShiftItemLara(item, coll);
    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColForwardJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    LaraDeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else if (Input & IN_FORWARD && !(Input & IN_SLOW)) {
            item->goal_anim_state = AS_RUN;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        AnimateLara(item);
    }
}

void LaraColPose(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColStop(item, coll);
}

void LaraColFastBack(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = AS_FALLBACK;
        item->goal_anim_state = AS_FALLBACK;
        item->anim_number = AA_FALLBACK;
        item->frame_number = AF_FALLBACK * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColTurnR(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (coll->mid_floor > 100) {
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColTurnL(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColTurnR(item, coll);
}

void LaraColDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RAD * 4;
    GetLaraCollisionInfo(item, coll);

    ShiftItemLara(item, coll);
    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
    item->hit_points = -1;
    Lara.air = -1;
}

void LaraColFastFall(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    LaraSlideEdgeJump(item, coll);
    if (coll->mid_floor <= 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else {
            item->goal_anim_state = AS_STOP;
            item->current_anim_state = AS_STOP;
            item->anim_number = AA_LANDFAR;
            item->frame_number = AF_LANDFAR * ANIM_SCALE;
        }
        StopSoundEffect(SFX_LARA_FALL, NULL);
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
    }
}

void LaraColHang(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraHangTest(item, coll);
    if (item->goal_anim_state == AS_HANG && (Input & IN_FORWARD)) {
        if (coll->front_floor > -850 && coll->front_floor < -650
            && coll->front_floor - coll->front_ceiling >= 0
            && coll->left_floor - coll->left_ceiling >= 0
            && coll->right_floor - coll->right_ceiling >= 0
            && !coll->hit_static) {
            if (Input & IN_SLOW) {
                item->goal_anim_state = AS_GYMNAST;
            } else {
                item->goal_anim_state = AS_NULL;
            }
        }
    }
}

void LaraColReach(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 1;
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = 0;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    if (LaraTestHangJump(item, coll)) {
        return;
    }
    LaraSlideEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;        
        lara_fall_speed_f = 0.0;
    }
}

void LaraColSplat(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    GetLaraCollisionInfo(item, coll);
    ShiftItemLara(item, coll);
}

void LaraColLand(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColStop(item, coll);
}

void LaraColCompress(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    GetLaraCollisionInfo(item, coll);

    if (coll->mid_ceiling > -100) {
        item->goal_anim_state = AS_STOP;
        item->current_anim_state = AS_STOP;
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        lara_float_pos.x = coll->old.x;
        lara_float_pos.y = coll->old.y;
        lara_float_pos.z = coll->old.z;
    }
}

void LaraColBack(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
    }

    if (coll->mid_floor > STEP_L / 2 && coll->mid_floor < (STEP_L * 3) / 2) {
        if (item->frame_number >= (964*ANIM_SCALE) && item->frame_number <= (993*ANIM_SCALE)) {
            item->anim_number = AA_BACKSTEPD_RIGHT;
            item->frame_number = AF_BACKSTEPD_RIGHT * ANIM_SCALE;
        } else {
            item->anim_number = AA_BACKSTEPD_LEFT;
            item->frame_number = AF_BACKSTEPD_LEFT * ANIM_SCALE;
        }
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColNull(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColFastTurn(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColStop(item, coll);
}

void LaraColStepRight(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot + PHD_90;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = STEP_L / 2;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColStepLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_90;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = 128;
    coll->bad_neg = -128;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColSlide(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    LaraSlideSlope(item, coll);
}

void LaraColBackJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    LaraColJumper(item, coll);
}

void LaraColRightJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot + PHD_90;
    LaraColJumper(item, coll);
}

void LaraColLeftJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_90;
    LaraColJumper(item, coll);
}

void LaraColUpJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->facing = Lara.move_angle;
    
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number, 870);
	
    if (LaraTestHangJumpUp(item, coll)) {
        return;
    }

    LaraSlideEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
    }
}

void LaraColFallBack(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    LaraDeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->pos.y += coll->mid_floor;
        lara_float_pos.y = item->pos.y;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
    }
}

void LaraColHangLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_90;
    LaraHangTest(item, coll);
    Lara.move_angle = item->pos.y_rot - PHD_90;
}

void LaraColHangRight(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot + PHD_90;
    LaraHangTest(item, coll);
    Lara.move_angle = item->pos.y_rot + PHD_90;
}

void LaraColSlideBack(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    LaraSlideSlope(item, coll);
}

void LaraColDefault(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);
}

void LaraColPushBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColPullBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColPPReady(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColPickup(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColSwitchOn(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColSwitchOff(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColUseKey(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColUsePuzzle(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColRoll(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    ShiftItemLara(item, coll);
    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColRoll2(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = AS_FALLBACK;
        item->goal_anim_state = AS_FALLBACK;
        item->anim_number = AA_FALLBACK;
        item->frame_number = AF_FALLBACK * ANIM_SCALE;
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    ShiftItemLara(item, coll);
    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;
}

void LaraColSpecial(ITEM_INFO *item, COLL_INFO *coll)
{
}

void LaraColUseMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColDieMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColSwanDive(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    LaraDeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        item->goal_anim_state = AS_STOP;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
    }
}

void LaraColFastDive(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);

    LaraDeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (item->fall_speed > 133) {
            item->goal_anim_state = AS_DEATH;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        item->pos.y += coll->mid_floor;
        lara_float_pos.y += coll->mid_floor;
    }
}

void LaraColGymnast(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColWaterOut(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraColDefault(item, coll);
}

void LaraColJumper(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);
    int collide = 1;
    //if(ANIM_SCALE == 2 && ((item->frame_number & 1) == 1)) {
	//	collide = 0; //make jump collisions be 30fps still
	//}
	if(collide == 1) {
		LaraDeflectEdgeJump(item, coll);

		//if (item->fall_speed > 0 && coll->mid_floor <= 0) {
		if (lara_fall_speed_f > 0.0 && coll->mid_floor <= 0) {
			if (LaraLandedBad(item, coll)) {
				item->goal_anim_state = AS_DEATH;
			} else {
				item->goal_anim_state = AS_STOP;
			}
			item->pos.y += coll->mid_floor;
			lara_float_pos.y += coll->mid_floor;
			item->gravity_status = 0;
			item->fall_speed = 0;
			lara_fall_speed_f = 0.0;
		}
	}
}

void GetLaraCollisionInfo(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->facing = Lara.move_angle;
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number,
        LARA_HITE);
}

void LaraSlideSlope(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -512;
    coll->bad_ceiling = 0;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    LaraDeflectEdge(item, coll);

    if (coll->mid_floor > (200)) {
        if (item->current_anim_state == AS_SLIDE) {
            item->current_anim_state = AS_FORWARDJUMP;
            item->goal_anim_state = AS_FORWARDJUMP;
            item->anim_number = AA_FALLDOWN;
            item->frame_number = AF_FALLDOWN * ANIM_SCALE;
        } else {
            item->current_anim_state = AS_FALLBACK;
            item->goal_anim_state = AS_FALLBACK;
            item->anim_number = AA_FALLBACK;
            item->frame_number = AF_FALLBACK * ANIM_SCALE;
        }
        item->gravity_status = 1;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        return;
    }

    TestLaraSlide(item, coll);
    item->pos.y += coll->mid_floor;
    lara_float_pos.y += coll->mid_floor;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        item->goal_anim_state = AS_STOP;
    }
}

int32_t LaraHitCeiling(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_TOP || coll->coll_type == COLL_CLAMP) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        lara_float_pos.x += coll->old.x;
        lara_float_pos.y += coll->old.y;
        lara_float_pos.z += coll->old.z;
        item->goal_anim_state = AS_STOP;
        item->current_anim_state = AS_STOP;
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
        item->gravity_status = 0;
        item->fall_speed = 0;
        lara_fall_speed_f = 0.0;
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        return 1;
    }
    return 0;
}

void LaraHangTest(ITEM_INFO *item, COLL_INFO *coll)
{
    int flag = 0;
    int16_t *bounds;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    GetLaraCollisionInfo(item, coll);
    if (coll->front_floor < 200) {
        flag = 1;
    }

    Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;

    PHD_ANGLE angle = ((uint16_t)(item->pos.y_rot + PHD_45)) / PHD_90;
    //LOG_DEBUG("angel %d, y_rot %d +PHD_45 %d", angle, item->pos.y_rot, (uint16_t)(item->pos.y_rot+PHD_45));
    switch (angle) {
    case DIR_NORTH:
        item->pos.z += 2;///ANIM_SCALE;
        lara_float_pos.z += 2;//*ANIM_SCALE;
        break;

    case DIR_WEST:
        item->pos.x -= 2;//*ANIM_SCALE;
        lara_float_pos.x -= 2;//*ANIM_SCALE;
        break;

    case DIR_SOUTH:
        item->pos.z -= 2;//*ANIM_SCALE;
        lara_float_pos.z -= 2;//*ANIM_SCALE;
        break;

    case DIR_EAST:
        item->pos.x += 2;//*ANIM_SCALE;
        lara_float_pos.x += 2;//*ANIM_SCALE;
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    GetLaraCollisionInfo(item, coll);

    if (!(Input & IN_ACTION) || item->hit_points <= 0) {
        item->goal_anim_state = AS_UPJUMP;
        item->current_anim_state = AS_UPJUMP;
        item->anim_number = AA_STOPHANG;
        item->frame_number = AF_STOPHANG * ANIM_SCALE;
        bounds = GetBoundsAccurate(item);
        
        lara_float_pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y] + 2;
        lara_float_pos.x += coll->shift.x;
        lara_float_pos.z += coll->shift.z;
        
        item->pos.y = lara_float_pos.y;
        item->pos.x = lara_float_pos.x;
        item->pos.z = lara_float_pos.z;
        
        item->gravity_status = 1;
        item->fall_speed = 1;
        lara_fall_speed_f = 1.0;
        item->speed = 2/ANIM_SCALE;
        lara_speed_F = 2.0/ANIM_SCALE; 
        LaraSpeedError = 0;
        Lara.gun_status = LGS_ARMLESS;
        return;
    }

    if (ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF
        || coll->mid_ceiling >= 0 || coll->coll_type != COLL_FRONT || flag) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        lara_float_pos.x = coll->old.x;
        lara_float_pos.y = coll->old.y;
        lara_float_pos.z = coll->old.z;
        if (item->current_anim_state == AS_HANGLEFT
            || item->current_anim_state == AS_HANGRIGHT) {
            item->goal_anim_state = AS_HANG;
            item->current_anim_state = AS_HANG;
            item->anim_number = AA_HANG;
            item->frame_number = AF_HANG * ANIM_SCALE;
        }
        return;
    }

    switch (angle) {
    case DIR_NORTH:
    case DIR_SOUTH:
        lara_float_pos.z += coll->shift.z;
        item->pos.z = lara_float_pos.z;
        break;

    case DIR_WEST:
    case DIR_EAST:
        lara_float_pos.x += coll->shift.x;
        item->pos.x = lara_float_pos.x;
        break;
    }

    bounds = GetBoundsAccurate(item);
    int hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif >= -STEP_L && hdif <= STEP_L) {
		lara_float_pos.y += hdif;
        item->pos.y = lara_float_pos.y;
        
    }
}

int32_t LaraDeflectEdge(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOPFRONT) {
        ShiftItemLara(item, coll);
        item->goal_anim_state = AS_STOP;
        item->current_anim_state = AS_STOP;
        item->gravity_status = 0;
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        return 1;
    }

    if (coll->coll_type == COLL_LEFT) {
        ShiftItemLara(item, coll);
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
    } else if (coll->coll_type == COLL_RIGHT) {
        ShiftItemLara(item, coll);
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
    }
    return 0;
}

void LaraDeflectEdgeJump(ITEM_INFO *item, COLL_INFO *coll)
{
    ShiftItemLara(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->pos.y_rot += LARA_DEF_ADD_EDGE; //DAN
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_FRONT:
    case COLL_TOPFRONT:
        item->goal_anim_state = AS_FASTFALL;
        item->current_anim_state = AS_FASTFALL;
        item->anim_number = AA_FASTFALL;
        item->frame_number = AF_FASTFALL * ANIM_SCALE;
        
        lara_speed_F /= 4.0;
        item->speed = lara_fall_speed_f;
        
        LaraSpeedError = 0;
        Lara.move_angle -= PHD_180;
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
            lara_fall_speed_f = 1.0;
        }
        break;

    case COLL_TOP:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
            lara_fall_speed_f = 1.0;
        }
        break;

    case COLL_CLAMP:
        //item->pos.z -= (phd_cos(coll->facing) * 100) >> W2V_SHIFT;
        //item->pos.x -= (phd_sin(coll->facing) * 100) >> W2V_SHIFT;
        lara_float_pos.z -= (phd_cos_f(coll->facing) * 100.0) / View2World;
        lara_float_pos.x -= (phd_sin_f(coll->facing) * 100.0) / View2World;
        item->pos.z = lara_float_pos.z;
        item->pos.x = lara_float_pos.x;
        
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        coll->mid_floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
            lara_fall_speed_f = 16.0;
        }
        break;
    }
}

void LaraSlideEdgeJump(ITEM_INFO *item, COLL_INFO *coll)
{
    ShiftItemLara(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_TOP:
    case COLL_TOPFRONT:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
            lara_fall_speed_f = 1.0;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (phd_cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (phd_sin(coll->facing) * 100) >> W2V_SHIFT;
        lara_float_pos.z -= (phd_cos_f(coll->facing) * 100.0) / View2World;
        lara_float_pos.x -= (phd_sin_f(coll->facing) * 100.0) / View2World;
        item->speed = 0;
        lara_speed_F = 0.0;
        LaraSpeedError = 0;
        coll->mid_floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
            lara_fall_speed_f = 16.0;
        }
        break;
    }
}

int32_t TestLaraVault(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !(Input & IN_ACTION)
        || Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return 0;
    }

    PHD_ANGLE angle = item->pos.y_rot;
    if (angle >= 0 - VAULT_ANGLE && angle <= 0 + VAULT_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - VAULT_ANGLE && angle <= PHD_90 + VAULT_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - VAULT_ANGLE
        || angle <= -(PHD_180 - 1) + VAULT_ANGLE) {
        angle = -PHD_180;
    } else if (
        angle >= -PHD_90 - VAULT_ANGLE && angle <= -PHD_90 + VAULT_ANGLE) {
        angle = -PHD_90;
    }

    if (angle & (PHD_90 - 1)) {
        return 0;
    }

    int32_t hdif = coll->front_floor;
    if (hdif >= -STEP_L * 2 - STEP_L / 2 && hdif <= -STEP_L * 2 + STEP_L / 2) {
        if (hdif - coll->front_ceiling < 0
            || coll->left_floor - coll->left_ceiling < 0
            || coll->right_floor - coll->right_ceiling < 0) {
            return 0;
        }
        item->current_anim_state = AS_NULL;
        item->goal_anim_state = AS_STOP;
        item->anim_number = AA_VAULT12;
        item->frame_number = AF_VAULT12 * ANIM_SCALE;
        item->pos.y += STEP_L * 2 + hdif;
        lara_float_pos.y += STEP_L * 2 + hdif;
        Lara.gun_status = LGS_HANDSBUSY;
        item->pos.y_rot = angle;
        ShiftItemLara(item, coll);
        return 1;
    } else if (
        hdif >= -STEP_L * 3 - STEP_L / 2 && hdif <= -STEP_L * 3 + STEP_L / 2) {
        if (hdif - coll->front_ceiling < 0
            || coll->left_floor - coll->left_ceiling < 0
            || coll->right_floor - coll->right_ceiling < 0) {
            return 0;
        }
        item->current_anim_state = AS_NULL;
        item->goal_anim_state = AS_STOP;
        item->anim_number = AA_VAULT34;
        item->frame_number = AF_VAULT34 * ANIM_SCALE;
        item->pos.y += STEP_L * 3 + hdif;
        lara_float_pos.y += STEP_L * 3 + hdif;
        Lara.gun_status = LGS_HANDSBUSY;
        item->pos.y_rot = angle;
        ShiftItemLara(item, coll);
        return 1;
    } else if (
        hdif >= -STEP_L * 7 - STEP_L / 2 && hdif <= -STEP_L * 4 + STEP_L / 2) {
        item->goal_anim_state = AS_UPJUMP;
        item->current_anim_state = AS_STOP;
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP * ANIM_SCALE;
        Lara.calc_fall_speed =
            -(int16_t)(phd_sqrt((int)(-2 * GRAVITY * (hdif + 800))) + 3);
        AnimateLara(item);
        item->pos.y_rot = angle;
        ShiftItemLara(item, coll);
        return 1;
    }

    return 0;
}

int32_t LaraTestHangJump(ITEM_INFO *item, COLL_INFO *coll)
{
    int hdif;
    int16_t *bounds;

    if (coll->coll_type != COLL_FRONT || !(Input & IN_ACTION)
        || Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return 0;
    }

    if (coll->front_ceiling > 0 || coll->mid_ceiling > -384
        || coll->mid_floor < 200) {
        return 0;
    }

    bounds = GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return 0;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return 0;
    }

    PHD_ANGLE angle = item->pos.y_rot;
    if (angle >= -HANG_ANGLE && angle <= HANG_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - HANG_ANGLE && angle <= PHD_90 + HANG_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - HANG_ANGLE
        || angle <= -(PHD_180 - 1) + HANG_ANGLE) {
        angle = -PHD_180;
    } else if (angle >= -PHD_90 - HANG_ANGLE && angle <= -PHD_90 + HANG_ANGLE) {
        angle = -PHD_90;
    }

    if (angle & (PHD_90 - 1)) {
        return 0;
    }

    if (TestHangSwingIn(item, angle)) {
        item->anim_number = AA_GRABLEDGEIN;
        item->frame_number = AF_GRABLEDGEIN * ANIM_SCALE;
    } else {
        item->anim_number = AA_GRABLEDGE;
        item->frame_number = AF_GRABLEDGE * ANIM_SCALE;
    }
    item->current_anim_state = AS_HANG;
    item->goal_anim_state = AS_HANG;

    // bounds = GetBoundsAccurate(item);
    lara_float_pos.y += hdif;
    lara_float_pos.x += coll->shift.x;
    lara_float_pos.z += coll->shift.z;    
    
    item->pos.y = lara_float_pos.y;
    item->pos.x = lara_float_pos.x;
    item->pos.z = lara_float_pos.z;
    
    item->pos.y_rot = angle;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    item->speed = 0;
    lara_speed_F = 0.0;
    LaraSpeedError = 0;
    Lara.gun_status = LGS_HANDSBUSY;
    return 1;
}

int32_t TestHangSwingIn(ITEM_INFO *item, PHD_ANGLE angle)
{
    int x = item->pos.x;
    int y = item->pos.y;
    int z = item->pos.z;
    int16_t room_num = item->room_number;
    switch (angle) {
    case 0:
        z += 256;
        break;
    case PHD_90:
        x += 256;
        break;
    case -PHD_90:
        x -= 256;
        break;
    case -PHD_180:
        z -= 256;
        break;
    }

    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    int h = GetHeight(floor, x, y, z);
    int c = GetCeiling(floor, x, y, z);

    if (h != NO_HEIGHT) {
        if ((h - y) > 0 && (c - y) < -400) {
            return 1;
        }
    }
    return 0;
}

int32_t LaraTestHangJumpUp(ITEM_INFO *item, COLL_INFO *coll)
{
    int hdif;
    int16_t *bounds;
    
    if (coll->coll_type != COLL_FRONT || !(Input & IN_ACTION)
        || Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return 0;
    }
    
    if (coll->front_ceiling > 0 || coll->mid_ceiling > -384) {
        return 0;
    }
    
    bounds = GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    
    if (hdif < 0 && hdif + (item->fall_speed) < 0) {
        return 0;
    }
   
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return 0;
    }
	
	int angle = item->pos.y_rot;
    if (angle >= 0 - HANG_ANGLE && angle <= 0 + HANG_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - HANG_ANGLE && angle <= PHD_90 + HANG_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - HANG_ANGLE
        || angle <= -(PHD_180 - 1) + HANG_ANGLE) {
        angle = -PHD_180;
    } else if (angle >= -PHD_90 - HANG_ANGLE && angle <= -PHD_90 + HANG_ANGLE) {
        angle = -PHD_90;
    }
	
	if (angle & (PHD_90 - 1)) {
        return 0;
    }
	
	item->goal_anim_state = AS_HANG;
    item->current_anim_state = AS_HANG;
    item->anim_number = AA_HANG;
    item->frame_number = AF_STARTHANG * ANIM_SCALE;
    bounds = GetBoundsAccurate(item);
    
    lara_float_pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    lara_float_pos.x += coll->shift.x;
    lara_float_pos.z += coll->shift.z; 
    
    item->pos.y = lara_float_pos.y;
    item->pos.x = lara_float_pos.x;
    item->pos.z = lara_float_pos.z;
       
    item->pos.y_rot = angle;
    item->gravity_status = 0;
    item->fall_speed = 0;
    lara_fall_speed_f = 0.0;
    item->speed = 0;
    lara_speed_F = 0.0;
    LaraSpeedError = 0;
    Lara.gun_status = LGS_HANDSBUSY;
    return 1;
}

int32_t TestLaraSlide(ITEM_INFO *item, COLL_INFO *coll)
{
    static PHD_ANGLE old_angle = 1;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        return 0;
    }

    PHD_ANGLE ang = 0;
    if (coll->tilt_x > 2) {
        ang = -PHD_90;
    } else if (coll->tilt_x < -2) {
        ang = PHD_90;
    }
    if (coll->tilt_z > 2 && coll->tilt_z > ABS(coll->tilt_x)) {
        ang = -PHD_180;
    } else if (coll->tilt_z < -2 && -coll->tilt_z > ABS(coll->tilt_x)) {
        ang = 0;
    }

    PHD_ANGLE adif = ang - item->pos.y_rot;
    ShiftItemLara(item, coll);
    if (adif >= -PHD_90 && adif <= PHD_90) {
        if (item->current_anim_state != AS_SLIDE || old_angle != ang) {
            item->goal_anim_state = AS_SLIDE;
            item->current_anim_state = AS_SLIDE;
            item->anim_number = AA_SLIDE;
            item->frame_number = AF_SLIDE * ANIM_SCALE;
            item->pos.y_rot = ang;
            Lara.move_angle = ang;
            old_angle = ang;
        }
    } else {
        if (item->current_anim_state != AS_SLIDEBACK || old_angle != ang) {
            item->goal_anim_state = AS_SLIDEBACK;
            item->current_anim_state = AS_SLIDEBACK;
            item->anim_number = AA_SLIDEBACK;
            item->frame_number = AF_SLIDEBACK * ANIM_SCALE;
            item->pos.y_rot = ang - PHD_180;
            Lara.move_angle = ang;
            old_angle = ang;
        }
    }
    return 1;
}

int16_t LaraFloorFront(ITEM_INFO *item, PHD_ANGLE ang, int32_t dist)
{
    int32_t x = item->pos.x + ((phd_sin(ang) * dist) >> W2V_SHIFT);
    int32_t y = item->pos.y - LARA_HITE;
    int32_t z = item->pos.z + ((phd_cos(ang) * dist) >> W2V_SHIFT);
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    int32_t height = GetHeight(floor, x, y, z);
    if (height != NO_HEIGHT)
        height -= item->pos.y;
    return height;
}

int32_t LaraLandedBad(ITEM_INFO *item, COLL_INFO *coll)
{
    int16_t room_num = item->room_number;

    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);

    int oy = item->pos.y;
    int height =
        GetHeight(floor, item->pos.x, item->pos.y - LARA_HITE, item->pos.z);

    item->floor = height;
    item->pos.y = height;
    TestTriggers(TriggerIndex, 0);
    item->pos.y = oy; //restores Y so don't touch double y
    
    //LOG_DEBUG("This is a test %d %g", item->fall_speed, lara_fall_speed_f);

    int landspeed = (item->fall_speed + 3 - DAMAGE_START); //this 3 is from measuring the different, why 3 no idea.
    if (landspeed <= 0) {
        return 0;
    } else if (landspeed > DAMAGE_LENGTH) {
        item->hit_points = -1;
    } else {
        item->hit_points -= ((LARA_HITPOINTS * landspeed * landspeed)
            / ((DAMAGE_LENGTH * DAMAGE_LENGTH)));
    }

    if (item->hit_points < 0) {
        return 1;
    }
    return 0;
}

void T1MInjectGameLara()
{
    INJECT(0x00422480, LaraAboveWater);

    INJECT(0x004225F0, LaraAsWalk);
    INJECT(0x00422670, LaraAsRun);
    INJECT(0x00422760, LaraAsStop);
    INJECT(0x00422970, LaraAsForwardJump);
    INJECT(0x00422A30, LaraAsFastBack);
    INJECT(0x00422A90, LaraAsTurnR);
    INJECT(0x00422B10, LaraAsTurnL);
    INJECT(0x00422B90, LaraAsFastFall);
    INJECT(0x00422BD0, LaraAsHang);
    INJECT(0x00422C20, LaraAsReach);
    INJECT(0x00422C40, LaraAsCompress);
    INJECT(0x00422EB0, LaraAsBack);
    INJECT(0x00422F30, LaraAsFastTurn);
    INJECT(0x00422F80, LaraAsStepRight);
    INJECT(0x00423000, LaraAsStepLeft);
    INJECT(0x00423080, LaraAsSlide);
    INJECT(0x004230B0, LaraAsBackJump);
    INJECT(0x004230D0, LaraAsRightJump);
    INJECT(0x004230F0, LaraAsFallBack);
    INJECT(0x00423120, LaraAsHangLeft);
    INJECT(0x00423160, LaraAsHangRight);
    INJECT(0x004231A0, LaraAsSlideBack);
    INJECT(0x004231C0, LaraAsPushBlock);
    INJECT(0x004231F0, LaraAsPPReady);
    INJECT(0x00423220, LaraAsPickup);
    INJECT(0x00423250, LaraAsSwitchOn);
    INJECT(0x00423280, LaraAsUseKey);
    INJECT(0x004232B0, LaraAsSpecial);
    INJECT(0x004232D0, LaraAsUseMidas);
    INJECT(0x004232F0, LaraAsDieMidas);
    INJECT(0x00423720, LaraAsSwanDive);
    INJECT(0x00423750, LaraAsFastDive);
    INJECT(0x00423790, LaraAsGymnast);
    INJECT(0x004237A0, LaraAsWaterOut);

    INJECT(0x004237C0, LaraColWalk);
    INJECT(0x004239F0, LaraColRun);
    INJECT(0x00423C00, LaraColStop);
    INJECT(0x00423D00, LaraColForwardJump);
    INJECT(0x00423DD0, LaraColFastBack);
    INJECT(0x00423F40, LaraColTurnR);
    INJECT(0x00423FF0, LaraColDeath);
    INJECT(0x00424070, LaraColFastFall);
    INJECT(0x004241F0, LaraColHang);
    INJECT(0x00424260, LaraColReach);
    INJECT(0x004243F0, LaraColSplat);
    INJECT(0x00424460, LaraColLand);
    INJECT(0x00424480, LaraColCompress);
    INJECT(0x00424520, LaraColBack);
    INJECT(0x00424690, LaraColStepRight);
    INJECT(0x004247D0, LaraColStepLeft);
    INJECT(0x00424910, LaraColSlide);
    INJECT(0x00424930, LaraColBackJump);
    INJECT(0x004249E0, LaraColRightJump);
    INJECT(0x00424A90, LaraColLeftJump);
    INJECT(0x00424B40, LaraColUpJump);
    INJECT(0x00424CD0, LaraColFallBack);
    INJECT(0x00424D80, LaraColHangLeft);
    INJECT(0x00424DC0, LaraColHangRight);
    INJECT(0x00424E00, LaraColSlideBack);
    INJECT(0x00424E30, LaraColDefault);
    INJECT(0x00424E90, LaraColRoll);
    INJECT(0x00424F90, LaraColRoll2);
    INJECT(0x004250A0, LaraColSwanDive);
    INJECT(0x00425130, LaraColFastDive);

    INJECT(0x004251D0, LaraSlideSlope);
    INJECT(0x00425350, LaraHangTest);
    INJECT(0x004255A0, LaraDeflectEdgeJump);
    INJECT(0x004256C0, TestLaraVault);
    INJECT(0x00425890, LaraTestHangJump);
    INJECT(0x00425AE0, LaraTestHangJumpUp);
    INJECT(0x00425C50, TestLaraSlide);
    INJECT(0x00425D70, LaraLandedBad);
}
