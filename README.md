# Screenlapse

Fun CPP application (which isn't well-made, can be optimized) that automatically make screenshots of your screen every few minutes and stores it into directory. (requested by [@themrlokopoff](https://github.com/TheMRLokopOff))

## Usage

1. Launch `screenlapse.exe`
2. Configure created `config.json` file (below)
3. Launch `screenlapse.exe` again
4. Add screenlapse.exe to startup (why not?)

## Configuration

- `directory` (string) - directory to save screenshots
- `timeout` (int) - sleep between new screenshots
- `format` (string) - format of screenshots (e.g. `png`, `jpeg`)
- `archive` (boolen) - should we archive screenshots of previous day

## Building

1. Install [vcpkg](https://vcpkg.io/en/getting-started.html)
2. Type `vcpkg install libzippp:x64-windows rapidjson:x64-windows` in `cmd.exe`
3. Type `vcpkg integrate install` in `cmd.exe`
4. Open .sln file in Visual Studio
5. Select **Release x64** version
6. Click **Build**

## Credits

- [rapidjson](https://github.com/Tencent/rapidjson) - parsing JSON configuration
- [libzippp](https://github.com/ctabin/libzippp) - archivation of screenshots
- [prashanthrajagopal](https://gist.github.com/prashanthrajagopal/) - how 2 screenshot [screenshot.cpp](https://gist.github.com/prashanthrajagopal/05f8ad157ece964d8c4d)
