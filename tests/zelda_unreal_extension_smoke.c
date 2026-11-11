/* Compiles the exact source snippet inserted into upstream cyc_extras.c. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t g_ram[65536];
static uint8_t g_sram[8192];
static uint64_t g_frame_count = 0;
static char response[8192];
static int has_error = 0;
void cyc_tcp_ok(int id, const char *fields) {
    snprintf(response, sizeof(response), "{\"id\":%d,\"ok\":true,%s}", id, fields);
}
void cyc_tcp_err(int id, const char *error) {
    (void)id;
    has_error = 1;
    snprintf(response, sizeof(response), "%s", error);
}
#include <stdbool.h>
#define ZELDAUNREAL_EXTENSION_TEST 1
/* Mock upstream world decoder for host-only extension unit tests. */
typedef struct { uint8_t tiles[704], attrs[48]; bool valid, simulation_ready; } TestWorldRoom;
static TestWorldRoom zelda_world_rooms[128];
static bool stock_rom_supported = true;
static int decoder_calls = 0;
static bool zelda_world_supported(void) { return stock_rom_supported; }
static bool zelda_world_decode_room(unsigned room) {
    ++decoder_calls;
    if (room >= 128) return false;
    zelda_world_rooms[room].tiles[0] = 0xCD;
    zelda_world_rooms[room].attrs[0] = 0x5A;
    zelda_world_rooms[room].valid = true;
    return true;
}
static bool cyc_tcp_long(const char *line, const char *field, long *out) {
    (void)field;
    return line && sscanf(line, "room=%ld", out) == 1;
}
#include "../src/zelda_unreal_tcp.inc"
int main(void) {
    g_ram[0x12] = 5;
    g_ram[0x10] = 0;
    g_ram[0xeb] = 0x77;
    g_ram[0x70] = 128;
    g_ram[0x84] = 176;
    g_ram[0x34a] = 0x80;
    g_ram[0x34f + 1] = 9;
    g_ram[0x70 + 1] = 42;
    g_ram[0x84 + 1] = 120;
    g_frame_count = 212;
    g_sram[0x530 + 22] = 0xCD;
    zelda_unreal_frame(7, NULL);
    assert(!has_error);
    assert(strstr(response, "\"frame\":212"));
    assert(strstr(response, "\"room\":119"));
    assert(strstr(response, "\"schema\":2"));
    assert(strstr(response, "\"profile_slot\":0"));
    assert(strstr(response, "\"quest_id\":0"));
    assert(strstr(response, "\"first_unwalkable\":128"));
    assert(strstr(response, "\"tiles_hex\":\"") != NULL);
    assert(strstr(response, "\"attrs_hex\":\"") != NULL);
    assert(strlen(response) < 8192);
    g_ram[0x12] = 2;
    zelda_unreal_frame(7, NULL);
    assert(has_error);
    g_ram[0x12] = 5;
    g_ram[0xeb] = 0x77;
    has_error = 0;
    zelda_unreal_room(8, "room=120");
    assert(!has_error && strstr(response, "\"room\":120"));
    assert(strstr(response, "\"isolated-decoder\""));
    assert(strstr(response, "\"tiles_hex\":\"CD"));
    assert(strstr(response, "\"attrs_hex\":\"5A"));
    assert(decoder_calls == 1);
    zelda_unreal_room(8, "room=127"); /* same row but far away */
    assert(has_error && decoder_calls == 1);
    has_error = 0;
    zelda_unreal_room(8, "room=103"); /* northern neighbor */
    assert(!has_error && decoder_calls == 2);
    // Decoder must NOT overwrite a room with live seamless simulation.
    zelda_world_rooms[0x76].valid = true;
    zelda_world_rooms[0x76].simulation_ready = true;
    zelda_world_rooms[0x76].tiles[0] = 0xFE;
    has_error = 0;
    zelda_unreal_room(8, "room=118");
    assert(!has_error && decoder_calls == 2);
    assert(strstr(response, "\"tiles_hex\":\"FE") != NULL);
    stock_rom_supported = false;
    has_error = 0;
    zelda_unreal_room(8, "room=104");
    assert(has_error && decoder_calls == 2);
    puts("NESRecomp extension with neighbor decoder: PASS");
    return 0;
}
