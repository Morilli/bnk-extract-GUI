# bnk-extract GUI

An attempt at making a somewhat competently looking gui for my bnk-extract program (see https://github.com/Morilli/bnk-extract).
Written in plain C without any libraries so if something looks dumb or it crashes randomly you know why.

The most current release can be found here: https://github.com/Morilli/bnk-extract-GUI/releases/latest.

# Usage
This program is intended to allow easy visualization and editing of sounds contained in .bnk or .wpk files.
Just select any .bnk or .wpk file that contains audio using the `select audio file` button and hit `Parse files`.
A treeview should be displayed showing all contained audio files with their respective name.
To get a more structured view (including grouping by sound event name) you will need to provide an additional `events` .bnk file
and a .bin file that contains sound event names.

For League of Legends, the audio file's name is usually suffixed with `_audio.wpk` or `_audio.bnk`, and the event file with `_events.bnk`.
The bin files that contain sound effect names are usually tied to a specific skin or map name, for example `skin0.bin`, `skin12.bin` or `map11.bin`.

Clicking on an audio file in the treeview should play its audio, and there is also a `Play sound` button.
Whether audio should play on click can be configured in the settings (`Settings` > `Settings` > `Settings`).

There are several ways to replace a .wem file's audio data:
- by drag-and-dropping a file with the same name as an existing item to the treeview
- by right-clicking and item and choosing `Replace wem data`
- by selecting one or multiple items and using the `Replace wem data` button on the side

To save a modified .bnk or .wpk file, select a root node in the treeview (the file path) and select the `Save as bnk/wpk` button.

A full .bnk/.wpk file's content or parts of it can also be extracted to disk by selecting all desired files (or one entire root node)
and using the `Extract selection` button.
Whether .wem files are extracted as is or converted to .ogg format beforehand can be configured in the settings.


# Building
To build, I recommend using the msys/mingw toolchain. A simple `make` call in the root directory should be enough to build the project.
Alternatively, `cmake` can be used as normal as well.
As this project uses the winapi, it can only be compiled on/for windows platforms.
For releases, I'm using an additional `-mwindows` linker command to make the program run console-less (just the window).
