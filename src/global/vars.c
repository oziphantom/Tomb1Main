#include "game/effects/bubble.h"
#include "game/effects/chain_block.h"
#include "game/effects/dino_stomp.h"
#include "game/effects/earthquake.h"
#include "game/effects/explosion.h"
#include "game/effects/finish_level.h"
#include "game/effects/flicker.h"
#include "game/effects/flipmap.h"
#include "game/effects/flood.h"
#include "game/effects/lara_effects.h"
#include "game/effects/powerup.h"
#include "game/effects/raising_block.h"
#include "game/effects/sand.h"
#include "game/effects/stairs2slope.h"
#include "game/effects/turn_180.h"
#include "global/vars.h"

char *ATIUserSettingsPath = "atiset.dat";
char *T1MUserSettingsPath = "cfg/Tomb1Main_runtime.json5";

int8_t IsGameWindowActive = 1;
double UITextScale;
double UIBarScale;
TEXTSTRING *AmmoText = NULL;
TEXTSTRING *FPSText = NULL;
int32_t FPSCounter = 0;

void (*EffectRoutines[])(ITEM_INFO *item) = {
    Turn180,    DinoStomp, LaraNormal,    LaraBubbles,  FinishLevel,
    EarthQuake, Flood,     RaisingBlock,  Stairs2Slope, DropSand,
    PowerUp,    Explosion, LaraHandsFree, FxFlipMap,    LaraDrawRightGun,
    ChainBlock, Flicker,
};

int32_t NoInputCount = 0;
int32_t IDelay;
int32_t IDCount;
int32_t Input;
int32_t InputDB;
int32_t KeyChange;

GAMEFLOW GF;
LARA_INFO Lara;
ITEM_INFO *LaraItem;
CAMERA_INFO Camera;
int32_t CameraUnderwater;
SAVEGAME_INFO SaveGame;
int32_t SavedGamesCount;
int32_t SaveCounter;
int32_t CurrentLevel;
int32_t DemoLevel;
uint32_t *DemoPtr;
int32_t DemoCount;
int32_t TitleLoaded;
int32_t LevelComplete;
int32_t ResetFlag;
int32_t OverlayFlag;
int32_t ChunkyFlag = 0;
int32_t HeightType = 0;

int32_t HealthBarTimer;
int16_t StoredLaraHealth = 0;

int16_t *FloorData;
int16_t *MeshBase;
int16_t **Meshes;
OBJECT_INFO Objects[O_NUMBER_OF];
STATIC_INFO StaticObjects[STATIC_NUMBER_OF];
int16_t RoomCount;
int32_t LevelItemCount;
ITEM_INFO *Items;
int16_t NextItemFree;
int16_t NextItemActive;
FX_INFO *Effects;
int16_t NextFxFree;
int16_t NextFxActive;
int32_t NumberBoxes;
BOX_INFO *Boxes;
uint16_t *Overlap;
int16_t *GroundZone[2];
int16_t *GroundZone2[2];
int16_t *FlyZone[2];
int32_t SlotsUsed;
CREATURE_INFO *BaddieSlots;
ANIM_STRUCT *Anims;
ANIM_CHANGE_STRUCT *AnimChanges;
ANIM_RANGE_STRUCT *AnimRanges;
int16_t *AnimCommands;
int32_t *AnimBones;
int16_t *AnimFrames;
int16_t *Cine;
int16_t NumCineFrames;
int16_t CineFrame;
PHD_3DPOS CinePosition;
int16_t RoomsToDraw[MAX_ROOMS_TO_DRAW];
int32_t RoomsToDrawNum;
int32_t NumberCameras;
int32_t NumberSoundEffects;
OBJECT_VECTOR *SoundEffectsTable;

int16_t *TriggerIndex;
int32_t FlipTimer = 0;
int32_t FlipEffect = -1;
int32_t FlipStatus;
int32_t FlipMapTable[MAX_FLIP_MAPS] = { 0 };

int16_t InvMode = INV_TITLE_MODE;
int32_t InvExtraData[8];
int16_t InvChosen = -1;

int16_t BarOffsetY[6];

int32_t LaraSpeedError = 0; //this is used to help with speed fix-ups when in 60fps mode
int16_t ANIM_SCALE = 1;
int GoldMode = 0;
