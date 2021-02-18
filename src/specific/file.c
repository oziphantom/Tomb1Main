#include "game/data.h"
#include "game/items.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/shed.h"
#include "mod.h"
#include "util.h"
#include <windows.h>

int32_t __cdecl LoadRooms(FILE* fp)
{
    TRACE("");
    uint16_t count2;
    uint32_t count4;

    _fread(&RoomCount, sizeof(uint16_t), 1, fp);
    if (RoomCount > MAX_ROOMS) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_RoomInfos);
    if (!RoomInfo) {
        strcpy(StringToShow, "LoadRoom(): Could not allocate memory for rooms");
        return 0;
    }

    int i = 0;
    for (ROOM_INFO* current_room_info = RoomInfo; i < RoomCount;
         ++i, ++current_room_info) {
        // Room position
        _fread(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        _fread(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        _fread(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        _fread(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        _fread(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data =
            game_malloc(sizeof(uint16_t) * count4, GBUF_RoomMesh);
        _fread(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        _fread(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_RoomDoor);
            current_room_info->doors->count = count2;
            _fread(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        _fread(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            game_malloc(sizeof(FLOOR_INFO) * count4, GBUF_RoomFloor);
        _fread(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        _fread(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_RoomLights);
            _fread(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        _fread(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_RoomStaticMeshInfos);
            _fread(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        _fread(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        _fread(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->bound_left = PhdWinMaxX;
        current_room_info->bound_top = PhdWinMaxY;
        current_room_info->bound_bottom = 0;
        current_room_info->bound_right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    _fread(&count4, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * count4, GBUF_FloorData);
    _fread(FloorData, sizeof(uint16_t), count4, fp);

    return 1;
}

int32_t __cdecl LoadItems(FILE* handle)
{
    int32_t item_count = 0;
    _fread(&item_count, sizeof(int32_t), 1, handle);

    TRACE("Item count: %d", item_count);

    if (item_count) {
        if (item_count > NUMBER_ITEMS) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
        }

        Items = game_malloc(sizeof(ITEM_INFO) * NUMBER_ITEMS, GBUF_Items);
        if (!Items) {
            strcpy(
                StringToShow,
                "LoadItems(): Unable to allocate memory for 'items'");
            return 0;
        }

        LevelItemCount = item_count;
        InitialiseItemArray(NUMBER_ITEMS);

        for (int i = 0; i < item_count; ++i) {
            ITEM_INFO* item = &Items[i];
            _fread(&item->object_number, 2, 1, handle);
            _fread(&item->room_number, 2, 1, handle);
            _fread(&item->pos.x, 4, 1, handle);
            _fread(&item->pos.y, 4, 1, handle);
            _fread(&item->pos.z, 4, 1, handle);
            _fread(&item->pos.y_rot, 2, 1, handle);
            _fread(&item->shade, 2, 1, handle);
            _fread(&item->flags, 2, 1, handle);

            if (item->object_number < 0
                || item->object_number >= NUMBER_OBJECTS) {
                sprintf(
                    StringToShow,
                    "LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
                S_ExitSystem(StringToShow);
            }

            InitialiseItem(i);
        }
    }

    return 1;
}
int32_t __cdecl S_LoadLevel(int level_id)
{
    TRACE("%d (%s)", level_id, LevelNames[level_id]);
    int ret = LoadLevel(LevelNames[level_id], level_id);

    if (Tomb1MConfig.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int lara_found = 0;
        int in_cutscene = 0;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].object_number == O_LARA) {
                lara_found = 1;
            }
            if (Items[i].object_number == O_PLAYER_1
                || Items[i].object_number == O_PLAYER_2
                || Items[i].object_number == O_PLAYER_3
                || Items[i].object_number == O_PLAYER_4) {
                in_cutscene = 1;
            }
        }
        if (!lara_found && !in_cutscene) {
            Tomb1MData.stored_lara_health = LARA_HITPOINTS;
        }
    }

    return ret;
}

const char* __cdecl GetFullPath(const char* filename)
{
    TRACE(filename);
#if defined FEATURE_NOCD_DATA
    sprintf(newpath, ".\\%s", filename);
#else
    if (DEMO) {
        sprintf(newpath, "%c:\\tomb\\%s", cd_drive, filename);
    } else {
        sprintf(newpath, "%c:\\%s", cd_drive, filename);
    }
#endif
    return newpath;
}

int32_t __cdecl FindCdDrive()
{
    TRACE("");
    FILE* fp;
    char root[5] = "C:\\";
    char tmp_path[MAX_PATH];

    for (cd_drive = 'C'; cd_drive <= 'Z'; cd_drive++) {
        root[0] = cd_drive;
        if (GetDriveType(root) == DRIVE_CDROM) {
            sprintf(tmp_path, "%c:\\tomb\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 1;
                return fclose(fp);
            }
            sprintf(tmp_path, "%c:\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 0;
                return fclose(fp);
            }
        }
    }
    ShowFatalError("ERROR: Please insert TombRaider CD");
    return 0;
}

void Tomb1MInjectSpecificFile()
{
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041BC60, LoadItems);
    INJECT(0x0041AF90, S_LoadLevel);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x0041C020, FindCdDrive);
}