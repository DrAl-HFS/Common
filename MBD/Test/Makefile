# Makefile : i2c util test
#ifndef GCC
ifdef CLANG
CC := clang-6.0
OPT := -Wall -Oz
else
CC := gcc
OPT := -Wall -Os -march=native
#OPT := -v -Wall -g -O0
# -std=c11 -D__USE_MISC -pedantic
endif

# ??? BLD_DIR := .
MAKEFILE := Makefile

# Everything hangs off development root (projects folder)
DVL_ROOT := $(shell echo ~/dvl)
COM_DIR := $(DVL_ROOT)/Common/src
SRC_DIR := $(DVL_ROOT)/Common/MBD
HDR_DIR := $(SRC_DIR)
OBJ_DIR := obj

# Each sub-project/component uses explicit module
# list from which file names are generated. Anything
# not fitting the pattern (header without body or
# vice versa) requires explicit addition...
I2C_MOD := lxI2C lxTiming
I2C_SRC := $(I2C_MOD:%=$(SRC_DIR)/%.c)
I2C_HDR := $(I2C_MOD:%=$(HDR_DIR)/%.h)
I2C_OBJ := $(I2C_MOD:%=$(OBJ_DIR)/%.o)

I2C_TST_MOD := ledMatrix ledMapRGB mbdUtil ubxUtil
I2C_TST_SRC := $(I2C_TST_MOD:%=$(SRC_DIR)/%.c)
I2C_TST_HDR := $(I2C_TST_MOD:%=$(HDR_DIR)/%.h)
I2C_TST_OBJ := $(I2C_TST_MOD:%=$(OBJ_DIR)/%.o)
I2C_TST_HDR+= $(HDR_DIR)/lumissil.h

ADS_MOD := ads1xDev ads1xUtil
ADS_SRC := $(ADS_MOD:%=$(SRC_DIR)/%.c)
ADS_HDR := $(ADS_MOD:%=$(HDR_DIR)/%.h)
ADS_OBJ := $(ADS_MOD:%=$(OBJ_DIR)/%.o)
ADS_HDR+= $(HDR_DIR)/ads1x.h

LSM_MOD := lsmUtil
LSM_SRC := $(LSM_MOD:%=$(SRC_DIR)/%.c)
LSM_HDR := $(LSM_MOD:%=$(HDR_DIR)/%.h)
LSM_OBJ := $(LSM_MOD:%=$(OBJ_DIR)/%.o)
LSM_HDR+= $(HDR_DIR)/lsm9ds1.h

# Support modules
SUPP_MOD := report util sciFmt
SUPP_SRC := $(SUPP_MOD:%=$(COM_DIR)/%.c)
SUPP_HDR := $(SUPP_MOD:%=$(COM_DIR)/%.h)
SUPP_OBJ := $(SUPP_MOD:%=$(OBJ_DIR)/%.o)


LIBS := -lm
# -lrt	Solaris "real-time" library, deprecated
I2C_INCDEF := -I$(COM_DIR) -DLX_I2C_MAIN -DLX_I2C_TEST
ADS_INCDEF := -I$(COM_DIR) -DADS1X_MAIN -DADS1X_TEST
LSM_INCDEF := -I$(COM_DIR) -DLSM_MAIN -DLSM_TEST


.PHONY : all clean run

all : ti2c ads1x lsm9ds1


# Move any object files to the expected location
$(OBJ_DIR)/%.o : %.o
	mv $< $@

# Conflicting conditional compilations break folder based rules
#%.o : $(SRC_DIR)/%.c $(HDR_DIR)/%.h
#	$(CC) $(OPT) $(ACC) $(INCDEF) $< -c

%.o : $(COM_DIR)/%.c $(COM_DIR)/%.h
	$(CC) $(OPT) $(ACC) $(INCDEF) $< -c


# Full build from source
ti2c : $(I2C_SRC) $(I2C_HDR) $(I2C_TST_SRC) $(I2C_TST_HDR) $(SUPP_OBJ) $(MAKEFILE)
	$(CC) $(OPT) $(I2C_INCDEF) $(LIBS) $(I2C_SRC) $(I2C_TST_SRC) $(SUPP_OBJ) -o $@

ads1x : $(I2C_SRC) $(I2C_HDR) $(ADS_SRC) $(ADS_HDR) $(SUPP_OBJ)  $(MAKEFILE)
	$(CC) $(OPT) $(ADS_INCDEF) $(LIBS) $(I2C_SRC) $(ADS_SRC) $(SUPP_OBJ) -o $@

lsm9ds1 : $(I2C_SRC) $(I2C_HDR) $(LSM_SRC) $(LSM_HDR) $(SUPP_OBJ) $(MAKEFILE)
	$(CC) $(OPT) $(LSM_INCDEF) $(LIBS) $(I2C_SRC) $(LSM_SRC) $(SUPP_OBJ) -o $@


setup :
	mkdir obj

clean :
	rm -f ti2c ads1x lsm9ds1 $(OBJ_DIR)/*

run : ti2c
	./$< -Hv

