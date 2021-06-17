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
struct sound {
    uint32_t self_id;
    uint32_t file_id;
    uint32_t source_id;
    uint32_t sound_object_id;
    uint8_t is_streamed;
};

struct music_track {
    uint32_t self_id;
    uint32_t file_id;
    uint32_t music_container_id;
};

struct music_container {
    uint32_t self_id;
    uint32_t music_switch_id;
    uint32_t sound_object_id;
    uint32_t music_track_id_amount;
    uint32_t* music_track_ids;
};

struct event_action {
    uint32_t self_id;
    uint8_t scope;
    uint8_t type;
    union {
        uint32_t sound_object_id;
        uint32_t switch_group_id;
    };
};

struct event {
    uint32_t self_id;
    uint8_t event_amount;
    uint32_t* event_ids;
};

// this one is pure guessing
struct random_container {
    uint32_t self_id;
    uint32_t switch_container_id;
    uint32_t sound_id_amount;
    uint32_t* sound_ids;
};

typedef LIST(struct sound) SoundSection;
typedef LIST(struct music_track) MusicTrackSection;
typedef LIST(struct music_container) MusicContainerSection;
typedef LIST(struct event_action) EventActionSection;
typedef LIST(struct event) EventSection;
typedef LIST(struct random_container) RandomContainerSection;


void free_sound_section(SoundSection* section)
{
    free(section->objects);
}

void free_event_action_section(EventActionSection* section)
{
    free(section->objects);
}

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

void free_music_container_section(MusicContainerSection* section)
{
    for (uint32_t i = 0; i < section->length; i++) {
        free(section->objects[i].music_track_ids);
    }
    free(section->objects);
}

void free_music_track_section(MusicTrackSection* section)
{
    free(section->objects);
}

int read_random_container_object(FILE* bnk_file, uint32_t object_length, RandomContainerSection* random_containers)
{
    uint32_t initial_position = ftell(bnk_file);
    struct random_container new_random_container_object;
    assert(fread(&new_random_container_object.self_id, 4, 1, bnk_file) == 1);
    dprintf("at the beginning: %ld\n", ftell(bnk_file));
    fseek(bnk_file, 1, SEEK_CUR);
    uint8_t unk = getc(bnk_file);
    fseek(bnk_file, 5 + (unk != 0) + (unk * 7), SEEK_CUR);
    dprintf("reading in switch container id at position %ld\n", ftell(bnk_file));
    assert(fread(&new_random_container_object.switch_container_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 1, SEEK_CUR);
    uint8_t unk2 = getc(bnk_file);
    if (unk2 != 0) {
        dprintf("offset here: %ld, unk2: %d\n", ftell(bnk_file), unk2);
        fseek(bnk_file, 5 * unk2, SEEK_CUR);
    }
    uint8_t unk3 = getc(bnk_file);
    dprintf("unk3: %d\n", unk3);
    fseek(bnk_file, 9 * unk3, SEEK_CUR);
    fseek(bnk_file, 9 + (getc(bnk_file) > 1), SEEK_CUR);
    dprintf("where am i? %ld\n", ftello(bnk_file));
    uint8_t unk4 = getc(bnk_file);
    dprintf("unk4: %d\n", unk4);
    if (unk4 > 1) { // skip this object, because I don't understand the format
        // examples: ashe skin23 sfx, ivern skin1 sfx, rammus skin6 and 16 sfx, yasuo skin17 sfx with unk4 = 2, and gangplank 8+ with unk4 = 56???
        v_printf(2, "Info: Skipping object because I can't read it lol.\n");
        fseek(bnk_file, initial_position + object_length, SEEK_SET);
        return -1;
    }
    int to_seek = 25;
    if (unk4) {
        fseek(bnk_file, 13, SEEK_CUR);
        uint8_t unk5 = getc(bnk_file);
        dprintf("unk5: %d\n", unk5);
        to_seek += 12 * unk5;
    }
    dprintf("ftell before the seek: %ld\n", ftell(bnk_file));
    dprintf("gonna seek %d\n", to_seek);
    fseek(bnk_file, to_seek, SEEK_CUR);
    assert(fread(&new_random_container_object.sound_id_amount, 4, 1, bnk_file) == 1);
    dprintf("sound object id amount: %u\n", new_random_container_object.sound_id_amount);
    if (new_random_container_object.sound_id_amount > 100) {
        eprintf("Would have allocated %u bytes. That can't be right. (ERROR btw)\n", new_random_container_object.sound_id_amount * 4);
        return -1;
    }
    new_random_container_object.sound_ids = malloc(new_random_container_object.sound_id_amount * 4);
    assert(fread(new_random_container_object.sound_ids, 4, new_random_container_object.sound_id_amount, bnk_file) == new_random_container_object.sound_id_amount);

    fseek(bnk_file, initial_position + object_length, SEEK_SET);
    add_object(random_containers, &new_random_container_object);

    return 0;
}

int read_sound_object(FILE* bnk_file, uint32_t object_length, SoundSection* sounds)
{
    uint32_t initial_position = ftell(bnk_file);
    struct sound new_sound_object;
    assert(fread(&new_sound_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 4, SEEK_CUR);
    assert(fread(&new_sound_object.is_streamed, 1, 1, bnk_file) == 1);
    assert(fread(&new_sound_object.file_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_sound_object.source_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 8, SEEK_CUR);
    assert(fread(&new_sound_object.sound_object_id, 4, 1, bnk_file) == 1);

    fseek(bnk_file, initial_position + object_length, SEEK_SET);
    add_object(sounds, &new_sound_object);

    return 0;
}

int read_event_action_object(FILE* bnk_file, uint32_t object_length, EventActionSection* event_actions)
{
    uint32_t initial_position = ftell(bnk_file);
    struct event_action new_event_action_object;
    assert(fread(&new_event_action_object.self_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_event_action_object.scope, 1, 1, bnk_file) == 1);
    assert(fread(&new_event_action_object.type, 1, 1, bnk_file) == 1);
    if (new_event_action_object.type == 25) {
        fseek(bnk_file, 7, SEEK_CUR);
        assert(fread(&new_event_action_object.switch_group_id, 4, 1, bnk_file) == 1);
    } else {
        assert(fread(&new_event_action_object.sound_object_id, 4, 1, bnk_file) == 1);
    }

    fseek(bnk_file, initial_position + object_length, SEEK_SET);
    add_object(event_actions, &new_event_action_object);

    return 0;
}

int read_event_object(FILE* bnk_file, EventSection* events)
{
    struct event new_event_object;
    assert(fread(&new_event_object.self_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_event_object.event_amount, 1, 1, bnk_file) == 1);
    new_event_object.event_ids = malloc(new_event_object.event_amount * 4);
    assert(fread(new_event_object.event_ids, 4, new_event_object.event_amount, bnk_file) == new_event_object.event_amount);

    add_object(events, &new_event_object);

    return 0;
}

int read_music_container_object(FILE* bnk_file, uint32_t object_length, MusicContainerSection* music_containers)
{
    uint32_t initial_position = ftell(bnk_file);
    struct music_container new_music_container_object;
    assert(fread(&new_music_container_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 4, SEEK_CUR);
    assert(fread(&new_music_container_object.music_switch_id, 4, 1, bnk_file) == 1);
    assert(fread(&new_music_container_object.sound_object_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 1, SEEK_CUR);
    fseek(bnk_file, 5 * getc(bnk_file), SEEK_CUR);
    fseek(bnk_file, 9 * getc(bnk_file), SEEK_CUR);
    fseek(bnk_file, 9 + (getc(bnk_file) > 1), SEEK_CUR);
    uint8_t unk = getc(bnk_file);
    int to_seek = 1;
    if (unk == 2) {
        to_seek++;
        fseek(bnk_file, 16, SEEK_CUR);
        fseek(bnk_file, 8 * getc(bnk_file), SEEK_CUR);
    }
    fseek(bnk_file, to_seek, SEEK_CUR);
    assert(fread(&new_music_container_object.music_track_id_amount, 4, 1, bnk_file) == 1);
    new_music_container_object.music_track_ids = malloc(new_music_container_object.music_track_id_amount * 4);
    assert(fread(new_music_container_object.music_track_ids, 4, new_music_container_object.music_track_id_amount, bnk_file) == new_music_container_object.music_track_id_amount);

    fseek(bnk_file, initial_position + object_length, SEEK_SET);
    add_object(music_containers, &new_music_container_object);

    return 0;
}

int read_music_track_object(FILE* bnk_file, uint32_t object_length, MusicTrackSection* music_tracks)
{
    uint32_t initial_position = ftell(bnk_file);
    struct music_track new_music_track_object;
    assert(fread(&new_music_track_object.self_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 10, SEEK_CUR);
    assert(fread(&new_music_track_object.file_id, 4, 1, bnk_file) == 1);
    fseek(bnk_file, 64, SEEK_CUR);
    assert(fread(&new_music_track_object.music_container_id, 4, 1, bnk_file) == 1);

    fseek(bnk_file, initial_position + object_length, SEEK_SET);
    add_object(music_tracks, &new_music_track_object);

    return 0;
}


int parse_event_bnk_file(char* path, SoundSection* sounds, EventActionSection* event_actions, EventSection* events, RandomContainerSection* random_containers, MusicContainerSection* music_segments, MusicTrackSection* music_tracks, MusicContainerSection* music_playlists)
{
    FILE* bnk_file = fopen(path, "rb");
    if (!bnk_file) {
        eprintf("Error: Failed to open \"%s\".\n", path);
        return -1;
    }

    uint32_t section_length = skip_to_section(bnk_file, "HIRC", false);
    if (!section_length) {
        eprintf("Error: Failed to skip to section \"HIRC\" in file \"%s\".\nMake sure to provide the correct file.\n", path);
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
        switch (type)
        {
            case 2:
                read_sound_object(bnk_file, object_length, sounds);
                break;
            case 3:
                read_event_action_object(bnk_file, object_length, event_actions);
                break;
            case 4:
                read_event_object(bnk_file, events);
                break;
            case 5:
                read_random_container_object(bnk_file, object_length, random_containers);
                break;
            case 10:
                read_music_container_object(bnk_file, object_length, music_segments);
                break;
            case 11:
                read_music_track_object(bnk_file, object_length, music_tracks);
                break;
            case 13:
                read_music_container_object(bnk_file, object_length, music_playlists);
                break;
            default:
                dprintf("Skipping object, as it is irrelevant for me.\n");
                dprintf("gonna seek %u forward\n", object_length);
                fseek(bnk_file, object_length, SEEK_CUR);
        }

        objects_read++;
    }
    dprintf("objects read: %u, num of objects: %u\n", objects_read, num_of_objects);
    assert(objects_read == num_of_objects);
    dprintf("Current offset: %ld\n", ftell(bnk_file));

    for (uint32_t i = 0; i < sounds->length; i++) {
        dprintf("sound object id: %u, source id: %u, file id: %u\n", sounds->objects[i].sound_object_id, sounds->objects[i].source_id, sounds->objects[i].file_id);
    }

    for (uint32_t i = 0; i < events->length; i++) {
        dprintf("Self id of all event objects: %u\n", events->objects[i].self_id);
    }
    dprintf("amount of event ids: %u\n", events->length);
    dprintf("amount of event actions: %u, amount of sounds: %u\n", event_actions->length, sounds->length);
    for (uint32_t i = 0; i < event_actions->length; i++) {
        dprintf("event action sound object ids: %u\n", event_actions->objects[i].sound_object_id);
    }

    fclose(bnk_file);

    return 0;
}


#define VERSION "1.6"
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
    SoundSection sounds;
    EventActionSection event_actions;
    EventSection events;
    RandomContainerSection random_containers;
    MusicContainerSection music_segments;
    MusicTrackSection music_tracks;
    MusicContainerSection music_playlists;
    initialize_list(&sounds);
    initialize_list(&event_actions);
    initialize_list(&events);
    initialize_list(&random_containers);
    initialize_list(&music_segments);
    initialize_list(&music_tracks);
    initialize_list(&music_playlists);

    if (parse_event_bnk_file(events_path, &sounds, &event_actions, &events, &random_containers, &music_segments, &music_tracks, &music_playlists) != 0) {
        wem_information = NULL;
        goto free_and_return;
    }
    sort_list(&event_actions, self_id);
    sort_list(&events, self_id);
    sort_list(&music_segments, self_id);
    sort_list(&music_tracks, self_id);

    dprintf("amount: %u\n", read_strings->length);
    for (uint32_t i = 0; i < read_strings->length; i++) {
        uint32_t hash = read_strings->objects[i].hash;
        dprintf("hashes[%u]: %u, string: %s\n", i, read_strings->objects[i].hash, read_strings->objects[i].string);

        struct event* event = NULL;
        find_object_s(&events, event, self_id, hash);
        if (!event) continue;
        for (uint32_t j = 0; j < event->event_amount; j++) {
            struct event_action* event_action = NULL;
            find_object_s(&event_actions, event_action, self_id, event->event_ids[j]);
            if (event_action && event_action->type == 4 /* "play" */) {
                for (uint32_t k = 0; k < sounds.length; k++) {
                    if (sounds.objects[k].sound_object_id == event_action->sound_object_id || sounds.objects[k].self_id == event_action->sound_object_id) {
                        dprintf("Found one!\n");
                        v_printf(2, "Hash %u of string %s belongs to file \"%u.wem\".\n", hash, read_strings->objects[i].string, sounds.objects[k].file_id);
                        add_object(&string_files, (&(struct string_hash) {read_strings->objects[i].string, sounds.objects[k].file_id, 0}));
                    }
                }
                for (uint32_t k = 0; k < music_segments.length; k++) {
                    if (music_segments.objects[k].sound_object_id == event_action->sound_object_id) {
                        for (uint32_t l = 0; l < music_segments.objects[k].music_track_id_amount; l++) {
                            struct music_track* music_track = NULL;
                            find_object_s(&music_tracks, music_track, self_id, music_segments.objects[k].music_track_ids[l]);
                            if (!music_track) continue;
                            dprintf("Found one 1!\n");
                            v_printf(2, "Hash %u of string %s belongs to file \"%u.wem\".\n", hash, read_strings->objects[i].string, music_track->file_id);
                            add_object(&string_files, (&(struct string_hash) {read_strings->objects[i].string, music_track->file_id, music_segments.objects[k].self_id}));
                        }
                    }
                }
                for (uint32_t k = 0; k < music_playlists.length; k++) {
                    if (music_playlists.objects[k].sound_object_id == event_action->sound_object_id) {
                        for (uint32_t l = 0; l < music_playlists.objects[k].music_track_id_amount; l++) {
                            struct music_container* music_segment = NULL;
                            find_object_s(&music_segments, music_segment, self_id, music_playlists.objects[k].music_track_ids[l]);
                            if (!music_segment) continue;
                            for (uint32_t m = 0; m < music_segment->music_track_id_amount; m++) {
                                struct music_track* music_track = NULL;
                                find_object_s(&music_tracks, music_track, self_id, music_segment->music_track_ids[m]);
                                if (!music_track) continue;
                                dprintf("Found one 2!\n");
                                v_printf(2, "Hash %u of string %s belongs to file \"%u.wem\".\n", hash, read_strings->objects[i].string, music_track->file_id);
                                add_object(&string_files, (&(struct string_hash) {read_strings->objects[i].string, music_track->file_id, music_segment->self_id}));
                            }
                        }
                    }
                }
                for (uint32_t k = 0; k < random_containers.length; k++) {
                    if (random_containers.objects[k].switch_container_id == event_action->sound_object_id) {
                        for (uint32_t l = 0; l < random_containers.objects[k].sound_id_amount; l++) {
                            for (uint32_t m = 0; m < sounds.length; m++) {
                                if (random_containers.objects[k].sound_ids[l] == sounds.objects[m].self_id || random_containers.objects[k].sound_ids[l] == sounds.objects[m].sound_object_id) {
                                    dprintf("sound id amount? %u\n", random_containers.objects[k].sound_id_amount);
                                    dprintf("Found one precisely here.\n");
                                    v_printf(2, "Hash %u of string %s belongs to file \"%u.wem\".\n", hash, read_strings->objects[i].string, sounds.objects[m].file_id);
                                    add_object(&string_files, (&(struct string_hash) {read_strings->objects[i].string, sounds.objects[m].file_id, random_containers.objects[k].self_id}));
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
    free_sound_section(&sounds);
    free_event_action_section(&event_actions);
    free_event_section(&events);
    free_random_container_section(&random_containers);
    free_music_container_section(&music_segments);
    free_music_track_section(&music_tracks);
    free_music_container_section(&music_playlists);

    free(string_files.objects);
    for (uint32_t i = 0; i < read_strings->length; i++) {
        free(read_strings->objects[i].string);
    }
    free(read_strings->objects);
    free(read_strings);

    return wem_information;
}
