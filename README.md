# FlatOut Online Play Support

A plugin to add online connectivity to FlatOut, without having to use any VPN or fake LAN services.

![lobby preview](https://i.imgur.com/Ds1GxZQ.png)

## Installation

- Make sure you have v1.1 of the game, as this is the only version this plugin is compatible with. (exe size of 2822144 bytes)
- Plop the files into your game folder, edit `FlatOutOnlinePlay_gcp.toml` to change the options to your liking.
- If you're planning to host a match, open the specified ports (23756 and 23757 by default) on your router.
- You should now be able to create games and have everyone with this mod installed simply see them in the server list.
- Enjoy, nya~ :3

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common) to a folder next to this one, so it can be found.

Required packages: `mingw-w64-gcc vcpkg`

To install all dependencies, use:
```console
vcpkg install curl:x86-mingw-static
```

Once installed, copy files from `~/.vcpkg/vcpkg/installed/x86-mingw-static/`:

- `include` dir to `nya-common/3rdparty`
- `lib` dir to `nya-common/lib32`

You should be able to build the project now in CLion.
