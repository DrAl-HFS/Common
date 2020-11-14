## Common
Common definitions, types and functions in C source files.
Intended to run under Linux - modules prefixed lx* have a strong dependancy on OS interfaces.
Other modules such as *Dev or *Util depend on lx* modules and so are not readily portable outside the Linux world.
Lowest level modules & headers provide definitions, types and functions that are useful in a wider sense: see
e.g. Duino repository.
Intended to be built into project using local configuration rather than as library.

General Modules (src/*.c) :-

* **platform** : general system library header includes, macros, tersely named rudimentary types.
* **util**   : dumping ground for miscellaneous types & functions.
* **sciFmt** : Scientific multiplier (y through Y in 1k steps) number format read/write.
* **report** : filtered reporting (stdout/stderr).

Embedded Modules (MBD/*.c) :-

* **lxI2C** : I2C (2-wire bus) utilities.
* **lxSPI** : SPI (3/4-wire bus) utilities.
* **lxUART** : UART serial interface utilities.
