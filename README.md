# FlatOut Online Play Support

A plugin to enable direct IP connections in FlatOut, without having to use any VPN or fake LAN services.

![lobby preview](https://i.imgur.com/Ds1GxZQ.png)

## Installation

- Make sure you have v1.1 of the game, as this is the only version this plugin is compatible with. (exe size of 2822144 bytes)
- Plop the files into your game folder, and for clients edit `FlatOutOnlinePlay_gcp.toml` to write in the server's IP address.
- For the server, open the specified ports (23756 and 23757 by default) on your router.
- You should now be able to start a server and have all clients with this mod installed simply see it in the server list.
- Enjoy, nya~ :3

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common) to a folder next to this one, so it can be found.

Required packages: `mingw-w64-gcc`

You should be able to build the project now in CLion.
