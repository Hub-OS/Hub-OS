# About

- [Discord](https://discord.hubos.dev)
- [Website](https://hubos.dev)

Hub OS is an online battle experience that takes place on interconnected servers, or Hubs.

Highlights:

- Rollback Netcode
- Multi-Battles (3+ players in a battle)
- Cross Platform - Windows, MacOS, Linux
- Custom Content - including mod manager and documentation
- Resource Packs
- Many, many, QOL features

Hub OS has roots in [OpenNetBattle](https://github.com/TheMaverickProgrammer/OpenNetBattle)

You can find our credits list on our website's [About page](https://hubos.dev/about) or Config -> About.

# Building

## Prerequisite

Windows requires [MSVC++](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-studio-2015-2017-2019-and-2022) for building Lua.

Install [rust](https://www.rust-lang.org/tools/install)

Run `cargo install cargo-about`. [cargo-about](https://crates.io/crates/cargo-about) is used for creating a license list file for distribution.

## All

Run `cargo run --bin dist` in the same folder as this README.
The commands below will run and a file containing licenses will be created in `dist`.

## Client

Run `cargo run --bin dist-client` in the same folder as this README.
A folder with the executable and required resources will be created in `dist/client`.

NOTE: If you're distributing files you should also use `cargo run --bin dist-licenses`

## Server

Run `cargo run --bin dist-server` in the same folder as this README.
A folder with the executable and required resources will be created in `dist/server`.

NOTE: If you're distributing files you should also use `cargo run --bin dist-licenses`

# Source Tinkering

Same prerequisites as [Building](#building). There's two primary projects in this repo `hub_os` in the `client` folder and `hub_os_server` in the `server` folder.

These subprojects require files from their respective subfolders to run, such as `resources` for the client. So running the client requires you to use `cargo run` in the client folder where the `resources` folder is available, and running the server must occur within the server folder.

Each subproject may have their own README as well for further information.
