# Deck Switch

Deck Switch is a little project that gives your Steam Deck LCD an even more console-like experience
by turning on or waking up your Steam Deck LCD whenever you turn on a Bluetooth controller.

This nifty device requires a bit of setup, and has a couple of caveats.
Both of which are explained down below.

## Setup
There's a number of parts to this entire setup.
Here's a brief overview of how this entire system works:
Firstly, you need the hardware that will interact with the Deck and controllers.
This will continuously scan for Bluetooth devices and turn on the Deck once it detects a controller.
The Deck is turned on from shutdown by connecting it to the charger.
If the BIOS setting `Power on AC Attach` is enabled, this will power on the Deck.
If the Deck is sleeping, the Arduino will send a keypress for 500ms to wake it up.

Once the Deck is powered on, the Arduino needs to know when the Deck goes back to sleep or shuts down.
This is done through the [deck-switch-client](./deck-switch-client),
which keeps sending a heartbeat message to the Deck as long as it's awake.
Once it figures out that the Deck is off,
it disconnects power so that it can attempt to turn the Deck on again when a controller turns on.

### Hardware
Visual diagram is WIP, for now, you'll just have to manage with text
- Arduino Micro
- Relay
- HM-10 BLE Module
- USB-C male and female breakout boards

#### Relay
- Connect relay input power to female USB-C power.
- Connect relay COMMON to male USB-C ground.
- Connect relay NORMALL OPEN (NO) to male USB-C positive.
- Connect relay trigger to data pin 2 on the Arduino.

#### HM-10 BLE Module
- Connect VCC to Arduino VCC, 5V, or 3.3V.
- Connect GRD to Arduino GRD
- Connect TX to Arduino data pin 8
- Connect RX to Arduino data pin 7

#### Arduino Micro
- Plug USB-C port into a USB port on the Steam Deck's dock.
- Plug the USB-C male breakout board into the dock's charging port.

### BIOS
Enable `Power on AC Attach`

### Software
You'll need to install a service to your Steam Deck for Deck Switch to behave.
Theoretically, you don't need this, but Deck Switch will disconnect your charger after about 15 seconds.

For now, you'll have to follow the install instructions from the client's [README.md](./deck-switch-client/README.md).
An install script is currently WIP.

## Known Issues
- The Deck will not wake up if it is plugged into the dock while asleep.
- The Deck cannot charge while shut down and plugged into the dock.
  - It can still charge while shut down when plugged into a normal charger or a dock that is not connected to this device

