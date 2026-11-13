/* ZeldaUnreal opt-in diagnostic extension for upstream src/cyc_extras.c.
 * READ-ONLY guest observation; no input, collision or AI hooks.
 * The optional neighbor decoder uses upstream isolated guest execution.
 * It changes only the upstream display/decoder cache, not active guest state;
 * full original runtime regression remains a required validation gate.
 */
/* ZELDAUNREAL_TCP_EXTENSION_BEGIN */
#ifndef ZELDAUNREAL_EXTENSION_TEST
#include "zelda_world_internal.h"
#endif

static void zelda_unreal_frame(int id, const char *line)
{
    (void)line;
    if (g_ram[0x12] != 5 || g_ram[0x10] != 0) {
        cyc_tcp_err(id, "zelda_unreal_frame: requires active overworld mode 5");
        return;
    }
    static const char hex[] = "0123456789ABCDEF";
    unsigned slot = (unsigned)g_ram[0x16];
    unsigned quest = (unsigned)g_ram[0x62d + (slot < 3 ? slot : 0)];
    char fields[4096];
    int used = snprintf(fields, sizeof(fields),
        "\"schema\":2,\"frame\":%llu,\"mode\":%u,\"room\":%u,"
        "\"level\":%u,\"profile_slot\":%u,\"quest_id\":%u,"
        "\"link_x\":%u,\"link_y\":%u,"
        "\"direction\":%u,\"link_state\":%u,\"first_unwalkable\":%u,"
        "\"tiles_hex\":\"",
        (unsigned long long)g_frame_count,
        (unsigned)g_ram[0x12], (unsigned)g_ram[0xeb], (unsigned)g_ram[0x10],
        slot, quest, (unsigned)g_ram[0x70], (unsigned)g_ram[0x84],
        (unsigned)g_ram[0x98], (unsigned)g_ram[0xac], (unsigned)g_ram[0x34a]);
    if (used < 0 || (size_t)used + 2 * (704 + 48) + 2000 >= sizeof(fields)) {
        cyc_tcp_err(id, "zelda_unreal_frame: insufficient buffer");
        return;
    }
    for (int i = 0; i < 704; ++i) {
        unsigned char value = g_sram[0x530 + i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    used += snprintf(fields + used, sizeof(fields) - (size_t)used, "\",\"attrs_hex\":\"");
    for (int i = 0; i < 48; ++i) {
        unsigned char value = g_ram[0x530 + i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    used += snprintf(fields + used, sizeof(fields) - (size_t)used, "\",\"slots\":[");
    for (int i = 0; i < 12; ++i) {
        int count = snprintf(fields + used, sizeof(fields) - (size_t)used,
            "%s{\"i\":%d,\"type\":%u,\"x\":%u,\"y\":%u,\"dir\":%u,\"state\":%u}",
            i ? "," : "", i, (unsigned)g_ram[0x34f + i],
            (unsigned)g_ram[0x70 + i], (unsigned)g_ram[0x84 + i],
            (unsigned)g_ram[0x98 + i], (unsigned)g_ram[0xac + i]);
        if (count < 0 || (size_t)count >= sizeof(fields) - (size_t)used) {
            cyc_tcp_err(id, "zelda_unreal_frame: entity buffer overflow");
            return;
        }
        used += count;
    }
    if ((size_t)used + 2 >= sizeof(fields)) {
        cyc_tcp_err(id, "zelda_unreal_frame: buffer exhausted");
        return;
    }
    fields[used++] = ']';
    fields[used] = '\0';
    cyc_tcp_ok(id, fields);
}

/* One optional neighboring room per request. Never simulates actors, and
 * only runs the upstream isolated decoder for supported stock ROMs.
 * The decoder output is STATIC TERRAIN, never authoritative live changes.
 */
static void zelda_unreal_room(int id, const char *line)
{
    if (g_ram[0x12] != 5 || g_ram[0x10] != 0) {
        cyc_tcp_err(id, "zelda_unreal_room: overworld mode 5 required");
        return;
    }
    long requested = -1;
    if (!cyc_tcp_long(line, "room", &requested) || requested < 0 || requested >= 128) {
        cyc_tcp_err(id, "zelda_unreal_room: room 0..127 required");
        return;
    }
    unsigned active = (unsigned)g_ram[0xeb];
    unsigned room = (unsigned)requested;
    int dx = (int)(room & 15) - (int)(active & 15);
    int dy = (int)(room >> 4) - (int)(active >> 4);
    if ((!dx && !dy) || dx < -1 || dx > 1 || dy < -1 || dy > 1) {
        cyc_tcp_err(id, "zelda_unreal_room: only adjacent rooms, not active room");
        return;
    }
    // A neighbor running in seamless mode owns dynamic tiles. Re-decoding
    // that room would overwrite pushed/bombed terrain in its live cache.
    // Read the existing room instance instead; decode only STATIC rooms.
    if (!zelda_world_supported() ||
        (zelda_world_rooms[room].simulation_ready
            ? !zelda_world_rooms[room].valid
            : !zelda_world_decode_room(room)) ||
        !zelda_world_rooms[room].valid) {
        cyc_tcp_err(id, "zelda_unreal_room: isolated decode unavailable");
        return;
    }
    static const char hex[] = "0123456789ABCDEF";
    char fields[2000];
    int n = snprintf(fields, sizeof(fields),
        "\"schema\":2,\"room\":%u,\"source\":\"isolated-decoder\","
        "\"first_unwalkable\":%u,\"tiles_hex\":\"",
        room, (unsigned)g_ram[0x34a]);
    if (n < 0 || (size_t)n + 2 * (704 + 48) + 16 >= sizeof(fields)) {
        cyc_tcp_err(id, "zelda_unreal_room: buffer insufficient");
        return;
    }
    size_t used = (size_t)n;
    for (int i = 0; i < 704; ++i) {
        unsigned char value = zelda_world_rooms[room].tiles[i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    n = snprintf(fields + used, sizeof(fields) - used, "\",\"attrs_hex\":\"");
    if (n < 0 || (size_t)n >= sizeof(fields) - used) {
        cyc_tcp_err(id, "zelda_unreal_room: buffer formatting error");
        return;
    }
    used += (size_t)n;
    for (int i = 0; i < 48; ++i) {
        unsigned char value = zelda_world_rooms[room].attrs[i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    fields[used++] = '"';
    fields[used] = '\0';
    cyc_tcp_ok(id, fields);
}

/* Atlas v1: a DIFFERENT endpoint from adjacent live-room prefetch.
 * Uses a caller-owned temporary buffer from isolated original-ROM execution.
 * Never writes global zelda_world_rooms or starts enemy simulation.
 */
static void zelda_unreal_atlas_room(int id, const char *line)
{
    if (g_ram[0x12] != 5 || g_ram[0x10] != 0) {
        cyc_tcp_err(id, "zelda_unreal_atlas_room: overworld mode 5 required");
        return;
    }
    long requested = -1;
    if (!cyc_tcp_long(line, "room", &requested) || requested < 0 || requested >= 128) {
        cyc_tcp_err(id, "zelda_unreal_atlas_room: room 0..127 required");
        return;
    }
    unsigned char tiles[ZW_TILE_COUNT], attrs[ZW_ATTR_COUNT];
    if (!zelda_world_supported() ||
        !zelda_world_export_static_room((unsigned)requested, tiles, attrs)) {
        cyc_tcp_err(id, "zelda_unreal_atlas_room: isolated static ROM decode failed");
        return;
    }
    static const char hex[] = "0123456789ABCDEF";
    unsigned slot = (unsigned)g_ram[0x16];
    unsigned quest = (unsigned)g_ram[0x62d + (slot < 3 ? slot : 0)];
    char fields[2048];
    int n = snprintf(fields, sizeof(fields),
        "\"schema\":3,\"atlas_schema\":1,\"source\":\"isolated-static-decoder\","
        "\"rom_prg_crc32\":\"3FE272FB\",\"room\":%lu,"
        "\"quest_id\":%u,\"profile_slot\":%u,"
        "\"first_unwalkable\":%u,\"tiles_hex\":\"",
        (unsigned long)requested, quest, slot, (unsigned)g_ram[0x34a]);
    if (n < 0 || (size_t)n + 2 * (ZW_TILE_COUNT + ZW_ATTR_COUNT) + 20 >= sizeof(fields)) {
        cyc_tcp_err(id, "zelda_unreal_atlas_room: formatting buffer insufficient");
        return;
    }
    size_t used = (size_t)n;
    for (int i = 0; i < ZW_TILE_COUNT; ++i) {
        unsigned char value = tiles[i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    n = snprintf(fields + used, sizeof(fields) - used, "\",\"attrs_hex\":\"");
    if (n < 0 || (size_t)n >= sizeof(fields) - used) {
        cyc_tcp_err(id, "zelda_unreal_atlas_room: attrs buffer overflow");
        return;
    }
    used += (size_t)n;
    for (int i = 0; i < ZW_ATTR_COUNT; ++i) {
        unsigned char value = attrs[i];
        fields[used++] = hex[value >> 4];
        fields[used++] = hex[value & 15];
    }
    fields[used++] = '"';
    fields[used] = '\0';
    cyc_tcp_ok(id, fields);
}
/* ZELDAUNREAL_TCP_EXTENSION_END */
