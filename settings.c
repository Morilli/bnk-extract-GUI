#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "resource.h"
#include "settings.h"

bool settings[SETTINGS_AMOUNT];
static bool oldSettings[SETTINGS_AMOUNT];


void LoadSettings()
{
    settings[0] = true; settings[1] = true; settings[2] = false; settings[3] = true; // default settings

    char* appdata = getenv("APPDATA");
    if (!appdata) return;
    const char pathPart[] = PROGRAM_NAME"/config.txt";
    char path[strlen(appdata) + strlen(pathPart) + 2];
    sprintf(path, "%s/%s", appdata, pathPart);
    FILE* settingsFile = fopen(path, "rb");
    if (!settingsFile) return;

    _lock_file(settingsFile);
    char byte;
    int index = 0;
    while ( (byte = getc(settingsFile)) != EOF) {
        if (byte == '\n') continue;
        settings[index++] = byte - '0';
        while (true) {
            byte = getc(settingsFile);
            if (byte == EOF) goto break_outer;
            if (byte == '\n') break;
        }
        if (index >= SETTINGS_AMOUNT) break;
    }
    break_outer: fclose(settingsFile);
}

void SaveSettings()
{
    char* appdata = getenv("APPDATA");
    if (!appdata) return;
    const char pathPart[] = PROGRAM_NAME"\0config.txt";
    char path[strlen(appdata) + strlen(pathPart) + 2];
    sprintf(path, "%s/%s", appdata, pathPart);
    mkdir(path);
    strcat(path, "/config.txt");
    FILE* settingsFile = fopen(path, "wb");
    if (!settingsFile) return;

    putc(settings[0] + '0', settingsFile); fputs(" // "__CRT_STRINGIZE(ID_AUTOPLAY_AUDIO)"\n", settingsFile);
    putc(settings[1] + '0', settingsFile); fputs(" // "__CRT_STRINGIZE(ID_EXTRACT_AS_WEM)"\n", settingsFile);
    putc(settings[2] + '0', settingsFile); fputs(" // "__CRT_STRINGIZE(ID_EXTRACT_AS_OGG)"\n", settingsFile);
    putc(settings[3] + '0', settingsFile); fputs(" // "__CRT_STRINGIZE(ID_MULTISELECT_ENABLED)"\n", settingsFile);
    fclose(settingsFile);
}

void BackupSettings()
{
    for (int i = 0; i < SETTINGS_AMOUNT; i++) {
        oldSettings[i] = settings[i];
    }
}

void RestoreSettings()
{
    for (int i = 0; i < SETTINGS_AMOUNT; i++) {
        settings[i] = oldSettings[i];
    }
}
