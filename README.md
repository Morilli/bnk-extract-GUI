<h1  align="center">

bnk-extract GUI 👾

</h1>

<p  align="center">
    <a  href="#tecnologias">Technologies</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
    <a  href="#projeto">Project</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
    <a  href="#rodando">Usage</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
    <a  href="#Building">Building</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
    <a  href="#como-contribuir">How to contribute</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
    <a  href="#license">License</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
</p>

<a  id="tecnologias"></a>

## Technologies 🖥️

<div  align="center">

<img  src="https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white"  />
<img  src="https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white"  />
<img  src="https://img.shields.io/badge/html5-%23E34F26.svg?style=for-the-badge&logo=html5&logoColor=white"  />
<img  src="https://img.shields.io/badge/shell_script-%23121011.svg?style=for-the-badge&logo=gnu-bash&logoColor=white"  />
<img  src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white"  />

</div>

<a  id="projeto"></a>

## Project 📕

An attempt at making a somewhat competently looking gui for my bnk-extract program (see https://github.com/Morilli/bnk-extract).

Written in plain C without any libraries so if something looks dumb or it crashes randomly you know why.

The most current release can be found here: https://github.com/Morilli/bnk-extract-GUI/releases/latest.

<a  id="rodando"></a>

## Usage 🚀

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

<a  id="Building"></a>

## Building

To build, I recommend using the msys/mingw toolchain. A simple `make` call in the root directory should be enough to build the project.


# Building

To build, I recommend using the msys/mingw toolchain. Build dependencies for the makefile-based build are `make`, `cmake` and `ninja`.

A simple `make` call in the root directory should be enough to build the project.

a49f29e6da79b6a8df4123b120a4791f3ee785c2

Alternatively, `cmake` can be used as normal as well.

As this project uses the winapi, it can only be compiled on/for windows platforms.

For releases, I'm using an additional `-mwindows` linker command to make the program run console-less (just the window).

<a  id="como-contribuir"></a>

## 🤔 How to contribute

- Fork this repository;

- Create a branch with your feature: `git checkout -b my-feature`;

- Commit your changes: `git commit -m 'feat: My new feature'`;

- Push to your branch: `git push origin my-feature`.

After your pull request is merged, you can delete your branch.

<a  id="license"></a>

### 🔖 license

If you want to check the project's license, just look at this file [LICENSE](./LICENSE.txt)
