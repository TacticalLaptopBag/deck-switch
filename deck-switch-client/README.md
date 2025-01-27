# Deck Switch Client
This is a simple service written in Rust to send heartbeat messages to the Arduino.
It listens to DBus system messages for when the system is going to sleep and pauses heartbeats when sleep is detected.
This prevents the Deck from prematurely waking up.
If no Arduino is detected, it simply keeps checking for a connection to the Arduino.

## Build
There should be an already built binary of the Deck Switch Client in [releases](https://github.com/TacticalLaptopBag/deck-switch/releases/latest),
but in case you're a fan of building your own code, here's how to do it.

### Dependencies
On Debian/Ubuntu-based distros, you'll need `libudev-dev`:
- `sudo apt update && sudo apt install -y libudev-dev`

You'll also need to install rust:
- `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

### Build Application
Simply run `cargo build --release`.
The only file you need from the output is `./target/release/deck-switch-client`

## Manual Install
Currently, there isn't a nifty install script, though one is currently WIP.
For now, you'll have to install everything manually.

Place your `deck-switch-client` binary in `~/.local/bin/`.
If this folder doesn't exist, run this command: `mkdir -p ~/.local/bin/`

Place the `deck-switch-client.service` file in `~/.config/systemd/user/`.
Run `systemctl --user enable deck-switch-client.service`

Add your user to the `uucp` group, this will give the `deck` user permission to talk to serial devices,
such as an Arduino: `sudo usermod -aG uucp deck`

Finally, restart your Steam Deck. This will ensure the `deck` user is added to the `uucp` group.

