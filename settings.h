#include <stdbool.h>

#include "resource.h"

#define SETTINGS_OFFSET ID_AUTOPLAY_AUDIO
#define SETTINGS_AMOUNT 4

extern bool settings[SETTINGS_AMOUNT];

void LoadSettings();
void SaveSettings();
void BackupSettings();
void RestoreSettings();
