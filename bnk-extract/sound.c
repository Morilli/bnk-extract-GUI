#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "defs.h"
#include "bin.h"
#include "bnk.h"
#include "wpk.h"

int VERBOSE = 0;

// http://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)

// almost full documentation of all types and versions: https://github.com/bnnm/wwiser

struct sound {
    uint32_t self_id;
    uint32_t file_id;
    uint8_t is_streamed;
};

struct music_track {
    uint32_t self_id;
    uint32_t track_count;
    uint32_t* file_ids;
    bool has_switch_ids;
    uint32_t switch_group_id;
    uint32_t* switch_ids;
    uint32_t parent_id;
};

struct music_container {
    uint32_t self_id;
    uint32_t music_switch_id;
    uint32_t sound_object_id;
    uint32_t music_track_id_amount;
    uint32_t* music_track_ids;
};

struct music_switch {
    uint32_t self_id;
    uint32_t parent_id;
    uint32_t num_children;
    uint32_t* children;
    uint32_t num_arguments;
    struct {
        uint32_t group_id;
        uint8_t group_type;
    }* arguments;
    uint32_t num_nodes;
    struct {
        uint32_t key;
        uint32_t audio_id;
    }* nodes;
};

struct event_action {
    uint32_t self_id;
    uint8_t scope;
    uint8_t type;
    union {
        uint32_t sound_object_id;
        uint32_t switch_state_id;
        uint32_t target_state_id;
    };
    union {
        uint32_t switch_group_id;
        uint32_t state_group_id;
    };
};

struct event {
    uint32_t self_id;
    uint8_t event_amount;
    uint32_t* event_ids;
};

struct random_container {
    uint32_t self_id;
    uint32_t switch_container_id;
    uint32_t sound_id_amount;
    uint32_t* sound_ids;
};

struct switch_container {
    uint32_t self_id;
    uint32_t parent_id;
    uint8_t group_type;
    uint32_t group_id;
    uint32_t num_children;
    uint32_t* children;
};

typedef LIST(struct sound) SoundSection;
typedef LIST(struct event_action) EventActionSection;
typedef LIST(struct event) EventSection;
typedef LIST(struct random_container) RandomContainerSection;
typedef LIST(struct switch_container) SwitchContainerSection;
typedef LIST(struct music_container) MusicContainerSection;
typedef LIST(struct music_track) MusicTrackSection;
typedef LIST(struct music_switch) MusicSwitchSection;

struct BNKSections {
    SoundSection sounds;
    EventActionSection event_actions;
    EventSection events;
    RandomContainerSection random_containers;
    SwitchContainerSection switch_containers;
    MusicContainerSection music_segments;
    MusicTrackSection music_tracks;
    MusicSwitchSection music_switches;
    MusicContainerSection music_playlists;
};

struct __attribute__((packed)) track_source_info {
    uint32_t track_index;
    uint32_t file_id;
    uint32_t event_id;
    double play_at;
    double begin_trim_offset;
    double end_trim_offset;
    double source_duration;
};

void skip_initial_fx_params(FILE* bnk_file, uint32_t bnk_version)
{
    fseek(bnk_file, 1, SEEK_CUR);
    uint8_t num_fx = getc(bnk_file);
    if (num_fx) fseek(bnk_file, 1, SEEK_CUR);
    fseek(bnk_file, num_fx * (bnk_version <= 0x91 ? 7 : 6), SEEK_CUR);
}

void skip_initial_params(FILE* bnk_file)
{
    uint8_t prop_count = getc(bnk_file);
    fseek(bnk_file, 5 * prop_count, SEEK_CUR);
    prop_count = getc(bnk_file);
    fseek(bnk_file, 9 * prop_count, SEEK_CUR);
}

void skip_positioning_params(FILE* bnk_file, uint32_t bnk_version)
{
    uint8_t positioning_bits = getc(bnk_file);
    bool has_positioning = positioning_bits & 1, has_3d = false, has_automation = false;
    if (has_positioning) {
        if (bnk_version <= 0x59) {
            bool has_2d = getc(bnk_file);
            has_3d = getc(bnk_file);
            if (has_2d) getc(bnk_file);
        } else {
            has_3d = positioning_bits & 0x2;
        }
    }
    if (has_positioning && has_3d) {
        if (bnk_version <= 0x59) {
            has_automation = (getc(bnk_file) & 3) != 1;
            fseek(bnk_file, 8, SEEK_CUR);
        } else {
            has_automation = (positioning_bits >> 5) & 3;
            getc(bnk_file);
        }
    }
    if (has_automation) {
        fseek(bnk_file, (bnk_version <= 0x59 ? 9 : 5), SEEK_CUR);
        uint32_t num_vertices;
        assert(fread(&num_vertices, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 16 * num_vertices, SEEK_CUR);
        uint32_t num_playlist_items;
        assert(fread(&num_playlist_items, 4, 1, bnk_file) == 1);
        dprintf("num vertices: %d, num_playlist items: %d, ftell: %ld\n", num_vertices, num_playlist_items, ftell(bnk_file));
        fseek(bnk_file, (bnk_version <= 0x59 ? 16 : 20) * num_playlist_items, SEEK_CUR);
    } else if (bnk_version <= 0x59) {
        getc(bnk_file);
    }
}

void skip_aux_params(FILE* bnk_file, uint32_t bnk_version)
{
    bool has_aux = (getc(bnk_file) >> 3) & 1;
    if (has_aux) fseek(bnk_file, 4 * sizeof(uint32_t), SEEK_CUR);
    if (bnk_version > 0x87) fseek(bnk_file, 4, SEEK_CUR);
}

void skip_state_chunk(FILE* bnk_file)
{
    uint8_t state_props = getc(bnk_file);
    fseek(bnk_file, 3 * state_props, SEEK_CUR);
    uint8_t state_groups = getc(bnk_file);
    for (uint8_t i = 0; i < state_groups; i++) {
        fseek(bnk_file, 5, SEEK_CUR);
        uint8_t states = getc(bnk_file);
        fseek(bnk_file, 8 * states, SEEK_CUR);
    }
}

void skip_rtpc(FILE* bnk_file, uint32_t bnk_version)
{
    uint16_t num_rtpc;
    assert(fread(&num_rtpc, 2, 1, bnk_file) == 1);
    for (int i = 0; i < num_rtpc; i++) {
        fseek(bnk_file, bnk_version <= 0x59 ? 13 : 12, SEEK_CUR);
        uint16_t point_count;
        assert(fread(&point_count, 2, 1, bnk_file) == 1);
        fseek(bnk_file, 12 * point_count, SEEK_CUR);
    }
}

void skip_clip_automation(FILE* bnk_file)
{
    uint32_t num_clip_automation;
    assert(fread(&num_clip_automation, 4, 1, bnk_file) == 1);
    for (uint32_t i = 0; i < num_clip_automation; i++) {
        fseek(bnk_file, 8, SEEK_CUR);
        uint32_t point_count;
        assert(fread(&point_count, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 12 * point_count, SEEK_CUR);
    }
}

uint32_t skip_base_params(FILE* bnk_file, uint32_t bnk_version, uint32_t* out_bus_id)
{
    skip_initial_fx_params(bnk_file, bnk_version);
    if (bnk_version > 0x88) {
        fseek(bnk_file, 1, SEEK_CUR);
        uint8_t num_fx = getc(bnk_file);
        fseek(bnk_file, 6 * num_fx, SEEK_CUR);
    }

    if (bnk_version > 0x59 && bnk_version <= 0x91) fseek(bnk_file, 1, SEEK_CUR);
    if (!out_bus_id) out_bus_id = &(uint32_t) {0};
    assert(fread(out_bus_id, 4, 1, bnk_file) == 1);
    dprintf("reading in parent id at position %ld\n", ftell(bnk_file));
    uint32_t parent_id;
    assert(fread(&parent_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, bnk_version <= 0x59 ? 2 : 1, SEEK_CUR);

    skip_initial_params(bnk_file);
    skip_positioning_params(bnk_file, bnk_version);
    skip_aux_params(bnk_file, bnk_version);

    fseek(bnk_file, 6, SEEK_CUR);

    uint8_t state_props = getc(bnk_file);
    fseek(bnk_file, 3 * state_props, SEEK_CUR);
    uint8_t state_groups = getc(bnk_file);
    for (uint8_t i = 0; i < state_groups; i++) {
        fseek(bnk_file, 5, SEEK_CUR);
        uint8_t states = getc(bnk_file);
        fseek(bnk_file, 8 * states, SEEK_CUR);
    }

    dprintf("skipping rtpc at position %ld\n", ftell(bnk_file));
    skip_rtpc(bnk_file, bnk_version);

    return parent_id;
}


#define free_simple_section(section) do { \
    free((section)->objects); \
} while (0)

void free_event_section(EventSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].event_ids);
    }
    free(section->objects);
}

void free_random_container_section(RandomContainerSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].sound_ids);
    }
    free(section->objects);
}

void free_switch_container_section(SwitchContainerSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].children);
    }
    free(section->objects);
}

void free_music_container_section(MusicContainerSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].music_track_ids);
    }
    free(section->objects);
}

void free_music_track_section(MusicTrackSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].file_ids);
        if (section->objects[i].has_switch_ids) {
            free(section->objects[i].switch_ids);
        }
    }
    free(section->objects);
}

void free_music_switch_section(MusicSwitchSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].children);
        if (section->objects[i].num_arguments) {
            free(section->objects[i].arguments);
        }
        if (section->objects[i].num_nodes) {
            free(section->objects[i].nodes);
        }
    }
    free(section->objects);
}

int read_random_container_object(FILE* bnk_file, RandomContainerSection* random_containers, uint32_t bnk_version)
{
    struct random_container new_random_container_object;
    assert(fread(&new_random_container_object.self_id, 4, 1, bnk_file) == 1);
    dprintf("at the beginning: %ld\n", ftell(bnk_file));

    new_random_container_object.switch_container_id = skip_base_params(bnk_file, bnk_version, NULL);

    fseek(bnk_file, 24, SEEK_CUR);
    assert(fread(&new_random_container_object.sound_id_amount, 4, 1, bnk_file) == 1);
    dprintf("sound object id amount: %u\n", new_random_container_object.sound_id_amount);
    if (new_random_container_object.sound_id_amount > 100) {
        eprintf("Would have allocated %u bytes. That can't be right. (ERROR btw)\n", new_random_container_object.sound_id_amount * 4);
        return -1;
    }
    new_random_container_object.sound_ids = malloc(new_random_container_object.sound_id_amount * 4);
    assert(fread(new_random_container_object.sound_ids, 4, new_random_container_object.sound_id_amount, bnk_file) == new_random_container_object.sound_id_amount);

    add_object(random_containers, &new_random_container_object);

    return 0;
}

int read_switch_container_object(FILE *bnk_file, SwitchContainerSection* switch_containers, uint32_t bnk_version)
{
    struct switch_container new_switch_container_object;
    assert(fread(&new_switch_container_object.self_id, 4, 1, bnk_file) == 1);

    new_switch_container_object.parent_id = skip_base_params(bnk_file, bnk_version, NULL);
    assert(fread(&new_switch_container_object.group_type, 1, 1, bnk_file) == 1);
    if (bnk_version <= 0x59) fseek(bnk_file, 3, SEEK_CUR);
    assert(fread(&new_switch_container_object.group_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 5, SEEK_CUR);

    assert(fread(&new_switch_container_object.num_children, 4, 1, bnk_file) == 1);
    new_switch_container_object.children = malloc(new_switch_container_object.num_children * sizeof(uint32_t));
    assert(fread(new_switch_container_object.children, 4, new_switch_container_object.num_children, bnk_file) == new_switch_container_object.num_children);

    add_object(switch_containers, &new_switch_container_object);

    return 0;
}

int read_sound_object(FILE* bnk_file, SoundSection* sounds, uint32_t bnk_version)
{
    struct sound new_sound_object;
    assert(fread(&new_sound_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 4, SEEK_CUR);
    assert(fread(&new_sound_object.is_streamed, 1, 1, bnk_file) == 1);
    if (bnk_version <= 0x59) fseek(bnk_file, 3, SEEK_CUR); // was 4 byte field with 3 bytes zero
    if (bnk_version <= 0x70) fseek(bnk_file, 4, SEEK_CUR);
    assert(fread(&new_sound_object.file_id, 4, 1, bnk_file) == 1);

    add_object(sounds, &new_sound_object);

    return 0;
}

int read_event_action_object(FILE* bnk_file, EventActionSection* event_actions)
{
    struct event_action new_event_action_object;
    assert(fread(&new_event_action_object.self_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_event_action_object.scope, 1, 1, bnk_file) == 1);
    assert(fread(&new_event_action_object.type, 1, 1, bnk_file) == 1);
    if (new_event_action_object.type == 25 /* set switch */) {
        fseek(bnk_file, 5, SEEK_CUR);
        skip_initial_params(bnk_file);
        assert(fread(&new_event_action_object.switch_group_id, 4, 1, bnk_file) == 1);
        assert(fread(&new_event_action_object.switch_state_id, 4, 1, bnk_file) == 1);
    } else if (new_event_action_object.type == 18 /* "set state" */) {
        fseek(bnk_file, 5, SEEK_CUR);
        skip_initial_params(bnk_file);
        assert(fread(&new_event_action_object.state_group_id, 4, 1, bnk_file) == 1);
        assert(fread(&new_event_action_object.target_state_id, 4, 1, bnk_file) == 1);
    } else {
        assert(fread(&new_event_action_object.sound_object_id, 4, 1, bnk_file) == 1);
    }

    add_object(event_actions, &new_event_action_object);

    return 0;
}

int read_event_object(FILE* bnk_file, EventSection* events, uint32_t bnk_version)
{
    struct event new_event_object;
    assert(fread(&new_event_object.self_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_event_object.event_amount, 1, 1, bnk_file) == 1);
    if (bnk_version == 0x58) fseek(bnk_file, 3, SEEK_CUR); // presumably padding bytes or 4 byte int which was later deemed unnecessarily high
    new_event_object.event_ids = malloc(new_event_object.event_amount * 4);
    assert(fread(new_event_object.event_ids, 4, new_event_object.event_amount, bnk_file) == new_event_object.event_amount);

    add_object(events, &new_event_object);

    return 0;
}

int read_music_container_object(FILE* bnk_file, MusicContainerSection* music_containers, uint32_t bnk_version)
{
    struct music_container new_music_container_object;
    assert(fread(&new_music_container_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 1, SEEK_CUR);
    new_music_container_object.sound_object_id = skip_base_params(bnk_file, bnk_version, &new_music_container_object.music_switch_id);
    assert(fread(&new_music_container_object.music_track_id_amount, 4, 1, bnk_file) == 1);
    new_music_container_object.music_track_ids = malloc(new_music_container_object.music_track_id_amount * 4);
    assert(fread(new_music_container_object.music_track_ids, 4, new_music_container_object.music_track_id_amount, bnk_file) == new_music_container_object.music_track_id_amount);

    add_object(music_containers, &new_music_container_object);

    return 0;
}

int read_music_track_object(FILE* bnk_file, MusicTrackSection* music_tracks, uint32_t bnk_version)
{
    struct music_track new_music_track_object;
    assert(fread(&new_music_track_object.self_id, 4, 1, bnk_file) == 1);
    getc(bnk_file);
    uint32_t count;
    assert(fread(&count, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 14 * count, SEEK_CUR);

    // read playlist count
    assert(fread(&count, 4, 1, bnk_file) == 1);
    // this "playlist count" lists all tracks, but there may be a dummy "null" track which has a switch id associated with it
    // seek forward and read in the subtrack count for an accurate value
    // let's hope this is safe and the subtrack count can't be other values (:
    fseek(bnk_file, count * sizeof(struct track_source_info), SEEK_CUR);
    assert(fread(&new_music_track_object.track_count, 4, 1, bnk_file) == 1);
    // printf("track count: %u, count: %u, offset: %lX\n", new_music_track_object.track_count, count, ftell(bnk_file));
    fseek(bnk_file, - 4 - count * sizeof(struct track_source_info), SEEK_CUR);

    new_music_track_object.file_ids = calloc(new_music_track_object.track_count, sizeof(uint32_t));
    for (uint32_t i = 0; i < count; i++) {
        struct track_source_info source_info = {0};
        assert(fread(&source_info, sizeof(struct track_source_info), 1, bnk_file) == 1);
        new_music_track_object.file_ids[source_info.track_index] = source_info.file_id;
    }
    fseek(bnk_file, 4, SEEK_CUR);
    skip_clip_automation(bnk_file);
    new_music_track_object.parent_id = skip_base_params(bnk_file, bnk_version, NULL);
    uint8_t track_type = getc(bnk_file);
    new_music_track_object.has_switch_ids = track_type == 0x3;
    if (new_music_track_object.has_switch_ids) {
        getc(bnk_file);
        assert(fread(&new_music_track_object.switch_group_id, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 4, SEEK_CUR); // default switch id
        assert(fread(&count, 4, 1, bnk_file) == 1);

        if (count != new_music_track_object.track_count) {
            printf("Error: Switch id count does not match track count!\n");
            return -1;
        }
        new_music_track_object.switch_ids = malloc(new_music_track_object.track_count * sizeof(uint32_t));
        assert(fread(new_music_track_object.switch_ids, sizeof(uint32_t), new_music_track_object.track_count, bnk_file) == new_music_track_object.track_count);
    }

    add_object(music_tracks, &new_music_track_object);

    return 0;
}

int read_music_switch_object(FILE* bnk_file, MusicSwitchSection* music_switches, uint32_t bnk_version)
{
    struct music_switch new_music_switch_object;
    assert(fread(&new_music_switch_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 1, SEEK_CUR);
    new_music_switch_object.parent_id = skip_base_params(bnk_file, bnk_version, NULL);
    assert(fread(&new_music_switch_object.num_children, 4, 1, bnk_file) == 1);
    new_music_switch_object.children = malloc(new_music_switch_object.num_children * sizeof(uint32_t));
    assert(fread(new_music_switch_object.children, 4, new_music_switch_object.num_children, bnk_file) == new_music_switch_object.num_children);
    fseek(bnk_file, 23, SEEK_CUR);
    uint32_t num_stingers;
    assert(fread(&num_stingers, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 24 * num_stingers, SEEK_CUR);
    uint32_t num_rules;
    assert(fread(&num_rules, 4, 1, bnk_file) == 1);
    for (uint32_t i = 0; i < num_rules; i++) {
        uint32_t num_sources;
        assert(fread(&num_sources, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 4 * num_sources, SEEK_CUR);
        uint32_t num_destinations;
        assert(fread(&num_destinations, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 4 * num_destinations, SEEK_CUR);
        fseek(bnk_file, bnk_version <= 0x84 ? 45 : 47, SEEK_CUR);
        bool has_transobject = getc(bnk_file);
        if (has_transobject) fseek(bnk_file, 30, SEEK_CUR);
    }
    fseek(bnk_file, 1, SEEK_CUR);
    assert(fread(&new_music_switch_object.num_arguments, 4, 1, bnk_file) == 1);
    new_music_switch_object.arguments = malloc(new_music_switch_object.num_arguments * sizeof(*new_music_switch_object.arguments));
    for (uint32_t i = 0; i < new_music_switch_object.num_arguments; i++) {
        assert(fread(&new_music_switch_object.arguments[i].group_id, 4, 1, bnk_file) == 1);
    }
    for (uint32_t i = 0; i < new_music_switch_object.num_arguments; i++) {
        assert(fread(&new_music_switch_object.arguments[i].group_type, 1, 1, bnk_file) == 1);
    }
    uint32_t tree_size;
    assert(fread(&tree_size, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 1, SEEK_CUR);
    new_music_switch_object.num_nodes = tree_size / 12;
    new_music_switch_object.nodes = malloc(new_music_switch_object.num_nodes * sizeof(*new_music_switch_object.nodes));
    for (uint32_t i = 0; i < new_music_switch_object.num_nodes; i++) {
        assert(fread(&new_music_switch_object.nodes[i].key, 4, 1, bnk_file) == 1);
        assert(fread(&new_music_switch_object.nodes[i].audio_id, 4, 1, bnk_file) == 1);
        fseek(bnk_file, 4, SEEK_CUR);
    }

    add_object(music_switches, &new_music_switch_object);

    return 0;
}

int parse_event_bnk_file(char* path, struct BNKSections* sections)
{
    FILE* bnk_file = fopen(path, "rb");
    if (!bnk_file) {
        eprintf("Error: Failed to open \"%s\".\n", path);
        return -1;
    }
    char magic[4];
    assert(fread(magic, 1, 4, bnk_file) == 4);
    if (memcmp(magic, "BKHD", 4) != 0) {
        eprintf("Error: Not a bnk file!\n");
        fclose(bnk_file);
        return -1;
    }
    fseek(bnk_file, 4, SEEK_CUR);
    uint32_t bnk_version;
    assert(fread(&bnk_version, 4, 1, bnk_file) == 1);

    uint32_t section_length = skip_to_section(bnk_file, "HIRC", true);
    if (!section_length) {
        eprintf("Error: Failed to skip to section \"HIRC\" in file \"%s\".\nMake sure to provide the correct file.\n", path);
        fclose(bnk_file);
        return -1;
    }
    uint32_t initial_position = ftell(bnk_file);
    uint32_t num_of_objects;
    assert(fread(&num_of_objects, 4, 1, bnk_file) == 1);
    uint32_t objects_read = 0;
    while ((uint32_t) ftell(bnk_file) < initial_position + section_length) {
        uint8_t type;
        uint32_t object_length;
        assert(fread(&type, 1, 1, bnk_file) == 1);
        assert(fread(&object_length, 4, 1, bnk_file) == 1);

        dprintf("Am here with an object of type %u\n", type);
        int object_start = ftell(bnk_file);
        switch (type)
        {
            case 2:
                read_sound_object(bnk_file, &sections->sounds, bnk_version);
                break;
            case 3:
                read_event_action_object(bnk_file, &sections->event_actions);
                break;
            case 4:
                read_event_object(bnk_file, &sections->events, bnk_version);
                break;
            case 5:
                read_random_container_object(bnk_file, &sections->random_containers, bnk_version);
                break;
            case 6:
                read_switch_container_object(bnk_file, &sections->switch_containers, bnk_version);
                break;
            case 10:
                read_music_container_object(bnk_file, &sections->music_segments, bnk_version);
                break;
            case 11:
                read_music_track_object(bnk_file, &sections->music_tracks, bnk_version);
                break;
            case 12:
                read_music_switch_object(bnk_file, &sections->music_switches, bnk_version);
                break;
            case 13:
                read_music_container_object(bnk_file, &sections->music_playlists, bnk_version);
                break;
            default:
                dprintf("Skipping object, as it is irrelevant for me.\n");
                dprintf("gonna seek %u forward\n", object_length);
        }
        fseek(bnk_file, object_start + object_length, SEEK_SET);

        objects_read++;
    }
    dprintf("objects read: %u, num of objects: %u\n", objects_read, num_of_objects);
    assert(objects_read == num_of_objects);
    dprintf("Current offset: %ld\n", ftell(bnk_file));

    for (uint32_t i = 0; i < sections->sounds.length; i++) {
        dprintf("is streamed: %s, file id: %u\n", sections->sounds.objects[i].is_streamed ? "true" : "false", sections->sounds.objects[i].file_id);
    }

    for (uint32_t i = 0; i < sections->events.length; i++) {
        dprintf("Self id of all event objects: %u\n", sections->events.objects[i].self_id);
    }
    dprintf("amount of event ids: %u\n", sections->events.length);
    dprintf("amount of event actions: %u, amount of sounds: %u\n", sections->event_actions.length, sections->sounds.length);
    for (uint32_t i = 0; i < sections->event_actions.length; i++) {
        dprintf("event action sound object ids: %u\n", sections->event_actions.objects[i].sound_object_id);
    }

    fclose(bnk_file);

    return 0;
}

void add_connected_files(char* string, uint32_t id, uint32_t parent_id, StringHashes* stringHashes, struct BNKSections* sections)
{
    struct music_switch* music_switch = NULL;
    find_object_s(&sections->music_switches, music_switch, self_id, id);
    if (music_switch) {
        for (uint32_t i = 0; i < music_switch->num_children; i++) {
            add_connected_files(string, music_switch->children[i], id, stringHashes, sections);
        }
        return;
    }

    struct music_container* music_playlist = NULL;
    find_object_s(&sections->music_playlists, music_playlist, self_id, id);
    if (music_playlist) {
        for (uint32_t i = 0; i < music_playlist->music_track_id_amount; i++) {
            add_connected_files(string, music_playlist->music_track_ids[i], music_playlist->music_track_id_amount > 1 ? id : parent_id, stringHashes, sections);
        }
        return;
    }

    struct random_container* random_container = NULL;
    find_object_s(&sections->random_containers, random_container, self_id, id);
    if (random_container) {
        for (uint32_t i = 0; i < random_container->sound_id_amount; i++) {
            add_connected_files(string, random_container->sound_ids[i], random_container->sound_id_amount > 1 ? id : parent_id, stringHashes, sections);
        }
        return;
    }

    struct switch_container* switch_container = NULL;
    find_object_s(&sections->switch_containers, switch_container, self_id, id);
    if (switch_container) {
        for (uint32_t i = 0; i < switch_container->num_children; i++) {
            add_connected_files(string, switch_container->children[i], id, stringHashes, sections);
        }
        return;
    }

    struct music_container* music_segment = NULL;
    find_object_s(&sections->music_segments, music_segment, self_id, id);
    if (music_segment) {
        for (uint32_t i = 0; i < music_segment->music_track_id_amount; i++) {
            struct music_track* music_track = NULL;
            find_object_s(&sections->music_tracks, music_track, self_id, music_segment->music_track_ids[i]);
            for (uint32_t j = 0; j < music_track->track_count; j++) {
                dprintf("Found a matching id in a music track!\n");
                v_printf(2, "Event %s belongs to file \"%u.wem\".\n", string, music_track->file_ids[j]);
                add_object(stringHashes, (&(struct string_hash) {
                    .string = string,
                    .hash = music_track->file_ids[j],
                    .container_id = parent_id,
                    .music_segment_id = music_segment->music_track_id_amount > 1 ? music_segment->self_id : 0
                }));
            }
        }
        return;
    }

    for (uint32_t i = 0; i < sections->sounds.length; i++) {
        if (sections->sounds.objects[i].self_id == id) {
            dprintf("Found a matching sound id!\n");
            v_printf(2, "Event %s belongs to file \"%u.wem\".\n", string, sections->sounds.objects[i].file_id);
            add_object(stringHashes, (&(struct string_hash) {
                .string = string,
                .hash = sections->sounds.objects[i].file_id,
                .container_id = parent_id,
                .sound_index = i
            }));
            return;
        }
    }
}


#define VERSION "1.8"
void print_help()
{
    printf("bnk-extract "VERSION" - a tool to extract bnk and wpk files, optionally sorting them into named groups.\n\n");
    printf("Syntax: ./bnk-extract --audio path/to/audio.[bnk|wpk] [--bin path/to/skinX.bin --events path/to/events.bnk] [-o path/to/output] [--wems-only] [--oggs-only]\n\n");
    printf("Options: \n");
    printf("  [-a|--audio] path\n    Specify the path to the audio bnk/wpk file that is to be extracted (mandatory).\n    Specifying this option without -e and -b will only extract files without grouping them by event name.\n\n");
    printf("  [-e|--events] path\n    Specify the path to the events bnk file that contains information about the events that trigger certain audio files.\n\n");
    printf("  [-b|--bin] path\n    Specify the path to the bin file that lists the clear names of all events.\n\n    Must specify both -e and -b options (or neither).\n\n");
    printf("  [-o|--output] path\n    Specify output path. Default is \"output\".\n\n");
    printf("  [--wems-only]\n    Extract wem files only.\n\n");
    printf("  [--oggs-only]\n    Extract ogg files only.\n    By default, both .wem and converted .ogg files will be extracted.\n\n");
    printf("  [--alternate-filenames]\n    Files will be named by event name + unique id instead of by their audio id.\n\n");
    printf("  [-v [-v ...]]\n    Increases verbosity level by one per \"-v\".\n");
}

WemInformation* bnk_extract(int argc, char* argv[])
{
    if (argc < 2) {
        eprintf("Missing arguments! (type --help for more info).\n");
        return NULL;
    }
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return NULL;
    }

    char* bin_path = NULL;
    char* audio_path = NULL;
    char* events_path = NULL;
    for (char** arg = &argv[1]; *arg; arg++) {
        if (strcmp(*arg, "-a") == 0 || strcmp(*arg, "--audio") == 0) {
            if (*(arg + 1)) {
                arg++;
                audio_path = *arg;
            }
        } else if (strcmp(*arg, "-e") == 0 || strcmp(*arg, "--events") == 0) {
            if (*(arg + 1)) {
                arg++;
                events_path = *arg;
            }
        } else if (strcmp(*arg, "-b") == 0 || strcmp(*arg, "--bin") == 0) {
            if (*(arg + 1)) {
                arg++;
                bin_path = *arg;
            }
        } else if (strcmp(*arg, "-v") == 0) {
            VERBOSE++;
        }
    }
    if (!audio_path) {
        eprintf("Error: No audio file provided.\n");
        return NULL;
    } else if (!events_path != !bin_path) { // one given, but not both
        eprintf("Error: Provide both events and bin file.\n");
        return NULL;
    }

    StringHashes string_files;
    initialize_list(&string_files);
    WemInformation* wem_information;
    if (!bin_path) {
        if (strlen(audio_path) >= 4 && memcmp(&audio_path[strlen(audio_path) - 4], ".bnk", 4) == 0)
            wem_information = parse_audio_bnk_file(audio_path, &string_files);
        else
            wem_information = parse_wpk_file(audio_path, &string_files);
        free(string_files.objects);
        if (wem_information) wem_information->grouped_wems->string = strdup(audio_path);
        return wem_information;
    }

    StringHashes* read_strings = parse_bin_file(bin_path);
    if (!read_strings) {
        free(string_files.objects);
        return NULL;
    }

    struct BNKSections sections = {0};
    initialize_list(&sections.sounds);
    initialize_list(&sections.event_actions);
    initialize_list(&sections.events);
    initialize_list(&sections.random_containers);
    initialize_list(&sections.switch_containers);
    initialize_list(&sections.music_segments);
    initialize_list(&sections.music_tracks);
    initialize_list(&sections.music_switches);
    initialize_list(&sections.music_playlists);

    if (parse_event_bnk_file(events_path, &sections) != 0) {
        wem_information = NULL;
        goto free_and_return;
    }

    sort_list(&sections.sounds, self_id);
    sort_list(&sections.event_actions, self_id);
    sort_list(&sections.events, self_id);
    sort_list(&sections.random_containers, self_id);
    sort_list(&sections.switch_containers, self_id);
    sort_list(&sections.music_segments, self_id);
    sort_list(&sections.music_tracks, self_id);
    sort_list(&sections.music_switches, self_id);
    sort_list(&sections.music_playlists, self_id);

    dprintf("amount: %u\n", read_strings->length);
    for (uint32_t i = 0; i < read_strings->length; i++) {
        uint32_t hash = read_strings->objects[i].hash;
        dprintf("hashes[%u]: %u, string: %s\n", i, read_strings->objects[i].hash, read_strings->objects[i].string);

        struct event* event = NULL;
        find_object_s(&sections.events, event, self_id, hash);
        if (!event) continue;
        for (uint32_t j = 0; j < event->event_amount; j++) {
            struct event_action* event_action = NULL;
            find_object_s(&sections.event_actions, event_action, self_id, event->event_ids[j]);
            if (!event_action) continue;

            if (event_action->type == 4 /* "play" */) {
                add_connected_files(read_strings->objects[i].string, event_action->sound_object_id, 0, &string_files, &sections);
            } else if (event_action->type == 25 /* "set switch" */) {
                for (uint32_t k = 0; k < sections.music_tracks.length; k++) {
                    struct music_track* music_track = &sections.music_tracks.objects[k];
                    if (music_track->has_switch_ids && music_track->switch_group_id == event_action->switch_group_id) {
                        for (uint32_t l = 0; l < music_track->track_count; l++) {
                            if (music_track->switch_ids[l] == event_action->switch_state_id) {
                                dprintf("Found a music track matching the event switch id!\n");
                                v_printf(2, "Event %s belongs to file \"%u.wem\".\n", read_strings->objects[i].string, music_track->file_ids[l]);
                                add_object(&string_files, (&(struct string_hash) {
                                    .string = read_strings->objects[i].string,
                                    .hash = music_track->file_ids[l],
                                    .switch_id = music_track->switch_group_id
                                }));
                            }
                        }
                    }
                }
            } else if (event_action->type == 18 /* "set state" */) {
                for (uint32_t k = 0; k < sections.music_switches.length; k++) {
                    struct music_switch* music_switch = &sections.music_switches.objects[k];
                    for (uint32_t l = 0; l < music_switch->num_arguments; l++) {
                        if (music_switch->arguments[l].group_type == 1 /* state */ && music_switch->arguments[l].group_id == event_action->state_group_id) {
                            for (uint32_t m = 0; m < music_switch->num_nodes; m++) {
                                if (music_switch->nodes[m].key == event_action->target_state_id) {
                                    add_connected_files(read_strings->objects[i].string, music_switch->nodes[m].audio_id, music_switch->self_id, &string_files, &sections);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    sort_list(&string_files, hash);
    if (strlen(audio_path) >= 4 && memcmp(&audio_path[strlen(audio_path) - 4], ".bnk", 4) == 0)
        wem_information = parse_audio_bnk_file(audio_path, &string_files);
    else
        wem_information = parse_wpk_file(audio_path, &string_files);
    if (wem_information) wem_information->grouped_wems->string = strdup(audio_path);

    free_and_return:;
    free_simple_section(&sections.sounds);
    free_simple_section(&sections.event_actions);
    free_event_section(&sections.events);
    free_random_container_section(&sections.random_containers);
    free_switch_container_section(&sections.switch_containers);
    free_music_container_section(&sections.music_segments);
    free_music_track_section(&sections.music_tracks);
    free_music_switch_section(&sections.music_switches);
    free_music_container_section(&sections.music_playlists);

    free(string_files.objects);
    for (uint32_t i = 0; i < read_strings->length; i++) {
        free(read_strings->objects[i].string);
    }
    free(read_strings->objects);
    free(read_strings);

    return wem_information;
}
