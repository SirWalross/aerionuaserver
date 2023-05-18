# Building from source (Windows and Linux)

## Server
- Install [vcpkg](https://vcpkg.io/en/getting-started.html) and remember the path you installed vcpkg in
- Install [cmake](https://cmake.org/) version 3.20 or higher
- Install a C++17 compliant compiler (on ubuntu-20.04 or higher the default g++ compiler should work, on windows install [Visual Studio](https://visualstudio.microsoft.com/vs/) with the `Desktop development with C++` workload and the optional `C++ CMake tools for Windows` component selected)
- Clone the repository with `git clone https://github.com/SirWalross/aerionuaserver.git` and run `cd aerionuaserver`
- On linux set the vcpkg path with `export VCPKG_ROOT="<path to vcpkg>"` with `<path to vcpkg>` being the path to the vcpkg directory
- Configure cmake with `cmake --preset win-x64-test` for windows or `cmake --preset unix-x64-test` for linux
- Build the server with `cmake --build --preset windows` for windows or `cmake --build --preset unix` for linux
- And run the server with `build\.\aerionuaserver.exe` on windows or `build/./aerionuaserver` on linux

## GUI
- Built with [tauri](https://tauri.app/) a rust framework for serving web-apps
- Uses [svelte-kit](https://kit.svelte.dev/) as the front-end framework
- To build the gui install [rust](https://www.rust-lang.org/tools/install) and [npm](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm)
- Navigate to the `gui` directory and run `npm install` and `cargo install tauri-cli`
- Run `cargo tauri dev` to run the gui or `cargo tauri build` to the build the gui
- Depending on the operating system additional packages may have to be installed before running

## Run tests
- To run the tests install [python 3.8](https://www.python.org/downloads/) or higher
- Install the necessary packages with `pip3 install pytest asyncua asyncio pytest-asyncio`
- Depending on the test run one or multiple robot and/or plc mockups with `python3 tests/mockup/robot.py` or `python3 tests/mockup/plc.py`
- Run the test with `python3 -m pytest <test-name>`

## TODO
- Add all predictive/preventive maintenance data from melfa smart plus card to server
- Fix `Task was destroyed but is pending` error in robot testing
- Fix having to recompile as release in ci on windows and add back icon of executable
- Access to cpu buffer memory access device from plc user nodes
- Add more datatypes to plc user nodes
- Add setting access control
- Add option to select client certificates to allow