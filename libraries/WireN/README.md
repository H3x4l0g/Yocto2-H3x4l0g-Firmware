# WireN

The Arduino AVR core `Wire` library (1.8.8), unchanged except for two constants:
`BUFFER_LENGTH` (`WireN.h`) and `TWI_BUFFER_LENGTH` (`utility/twi.h`) go from 32 to
**132**. The Yocto firmware writes and reads a full 128-byte EEPROM page plus its two
address bytes in one transaction.

Licensed under the GNU Lesser General Public License 2.1 or later, like the original:
see `LICENSE.txt` and the file headers.

To build the firmware, copy this folder into your Arduino `libraries` folder, or pass
`--libraries libraries` to `arduino-cli`.
