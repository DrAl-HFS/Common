## Common
Common definitions, types and functions in C source files. Intended to be built into project using local 
configuration rather than as library.

General Modules (src/*.c) :-

* **platform** : general system library header includes, macros, tersely named rudimentary types.
* **util** : dumping ground for miscellaneous types & functions.
* **report** : filtered reporting (stdout/stderr).

Embedded Modules (MBD/*.c) :-

* **lxI2C** : I2C bus utilities.
