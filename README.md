# Yocto v2 — maintained firmware

A continuation of the firmware for the **e-licktronic Yocto v2**, the TR-808 clone
(ATmega1284P). It starts from `Yocto_v2_003beta`, the last version e-licktronic
published in [e-licktronic/Yocto-V2.0](https://github.com/e-licktronic/Yocto-V2.0),
before the product was withdrawn in 2017 and the project was abandoned.

This is an independent, unofficial project: it is not affiliated with e-licktronic.
The original code, schematics and manuals are theirs. Upstream carries no license, so
this repository adds none of its own.

## What changes

Everything that changes how the machine behaves is listed in [CHANGELOG.md](CHANGELOG.md)
(in French). In short:

- **Fixes**: the encoder counts one step per detent instead of four. Edits no longer leak
  into fields you are not looking at. Power-on freezes and I2C lockups are cured, and
  failed saves now say so on screen. Trigger and clock timing is tighter, and slave sync
  no longer doubles the clock on MIDI OUT.
- **New per-step features**: micro-timing (0–23 ticks of delay), retrig (2, 3, 4, 6 or 8
  hits in the step) and trig conditions (play on the Xth of Y bars, or with a
  probability).
- **The pattern memory format is unchanged.** Your existing patterns keep working.

The development notebook, [TODO.md](TODO.md), records every diagnosis and decision. It
is in French too.

## Updating over MIDI — no programmer needed

Every Yocto v2 has e-licktronic's SysEx bootloader in its chip. You only need a MIDI
interface.

1. Download the `.syx` from the [Releases](../../releases). Its notes give the page count.
2. Power the Yocto on **while holding step buttons 1, 3 and 5**. All the LEDs blink twice,
   which means the bootloader is waiting.
3. Send the file to the Yocto's MIDI IN, **with a pause of at least 50 ms after each
   SysEx message** (after each `F7`). See below for why it matters.
4. The step LEDs count the accepted pages in binary. When the count reaches the page
   number in the release notes, the machine restarts and the boot screen shows the new
   version.

**If the count ends anywhere else, send the file again.** A rejected page is skipped
without any warning, and every following page then lands one page too low. The
bootloader never writes over itself, though, so a failed transfer can always be redone.
If it keeps failing, suspect the MIDI interface first: cheap USB-MIDI cables are known
to drop SysEx bytes.

### Senders

- **Windows**: `python tools/send_syx.py` lists the MIDI outputs. Then run
  `python tools/send_syx.py yocto2.syx "<part of the port name>"`. It sends one message
  at a time and waits as the bootloader needs. It uses only the standard library. MIDI-OX
  tries to buffer the whole 88 KB file up front, and we could not get it to work.
- **macOS**: SysEx Librarian, with a delay between messages of at least 50 ms.

### Why the pause

The bootloader writes each 256-byte page with interrupts disabled for about 9 ms. The
UART only holds three bytes in the meantime, so a message that arrives during that time
is lost.

### The protocol

It was never documented, so it was read back from the bootloader itself. Each page is
one message:

```
F0 7D 08 08 02  7E 00  <256 bytes as 512 nibbles, high nibble first>  <sum mod 256, 2 nibbles>  F7
```

The pages carry no address: they are written in order from address 0. The message
`F0 7D 08 08 02 7F 00 F7` starts the application. `tools/hex2syx.py` builds the file.
Before writing it, the script replays it through a transcription of the bootloader's
state machine.

## Power-on freezes: the fuses

Many units left the factory with fuses that make the start-up unreliable:

| Fuse | Factory value | Effect |
| --- | --- | --- |
| lfuse | `0xAC` | Low-power crystal mode, short start-up delay, and a 16 MHz clock output on the encoder's channel B |
| efuse | `0xFF` | Brown-out detection disabled |

The fix needs an ISP programmer. Write only these two bytes:

```
avrdude -c <programmer> -p m1284p -U lfuse:w:0xF7:m -U efuse:w:0xFC:m
```

The corrected values are full-swing crystal with the longest start-up delay, and
brown-out detection at 4.3 V. **Leave hfuse alone (`0xDC`)**, and **never use the
Arduino IDE's "Burn Bootloader"**. It rewrites all three fuses and erases the SysEx
bootloader, which exists nowhere else.

## Flashing over ISP

An ISP upload performs a chip erase, which takes the bootloader with it. So:

1. Back up your own chip first: `avrdude ... -U flash:r:yocto-backup.hex:i`.
2. Merge the application with the bootloader read from that backup:
   `python tools/merge_bootloader.py yocto2.ino.hex yocto-backup.hex yocto2-with-bootloader.hex`.
3. Flash the merged file. Nothing is flashed until avrdude prints `bytes of flash verified`.

## Building

- Arduino with **MightyCore 3.1.0**, board `MightyCore:avr:1284:pinout=standard,variant=modelP,clock=16MHz_external,LTO=Os`.
- Libraries: MIDI Library 5.0.2, LiquidCrystal, and **WireN**. WireN is the core's
  `Wire` with `BUFFER_LENGTH` and `TWI_BUFFER_LENGTH` raised to 132: a 128-byte EEPROM
  page plus its 2 address bytes.
- The build must produce no warnings on this repository's files.

```
arduino-cli compile --warnings more --fqbn "MightyCore:avr:1284:pinout=standard,variant=modelP,clock=16MHz_external,LTO=Os" .
python tools/hex2syx.py build/yocto2.ino.hex yocto2.syx
```

## Credits

- Original design and firmware: **e-licktronic**.
- Several fixes were cross-checked against the NAVA firmware maintained by **zabox**, and
  against the [MSS NAVA firmware](https://github.com/Modularsoundsystems/mss-nava-firmware).
  Ideas were rewritten here, not copied.
