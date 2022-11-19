# Features:

- [x] Rollback
- [x] Unlimited players in Netplay/PVP
- [x] Official co-op support
- [x] P2P PVP and indirect fallback
- [x] Mod protection
- [ ] Automated GitHub builds
  - [x] Windows
  - [ ] Mac OS
  - [x] Linux / Steam Deck
  - [ ] Android

# Building

## Prerequisite

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

Same prerequisites as [Building](#building). There's two primary projects in this repo `real_pet` in the `client` folder and `real_pet_server` in the `server` folder.

These subprojects require files from their respective subfolders to run, such as `resources` for the client. So running the client requires you to use `cargo run` in the client folder where the `resources` folder is available, and running the server must occur within the server folder.

Each subproject may have their own README as well for further information.

# Acknowledgments

The project is a derivation of [OpenNetBattle](https://github.com/TheMaverickProgrammer/OpenNetBattle), splintered off from creative differences.

## Special Thanks

- Alison Allbright
- Alrysc
- CosmicNobab
- Dawn Elaine
- D3str0y3d255
- DJRezzed
- Dunstan
- FrozenLake
- Gemini
- Gray Nine
- JamesKing
- jontheburger
- Keristero
- Konstinople
- Kuri
- kyqurikan
- Loui
- Mars
- Mint
- Mithalan
- OuroYisus
- Pion
- Salad
- Shale
- theWildSushii
- Pheelbert
- PlayerZero
- Poly
- Weenie
- Zeek
