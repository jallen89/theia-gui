SRC_DIR = ./src
APP_DIR = ./app
BIN_DIR = ./bin
INC_DIR = ./include
LIB_DIR = ./lib

CC = g++
CFLAGS_DEBUG = -DCONFIG_DEBUG
CFLAGS_DEBUG_MERGE =  $(CFLAGS_DEBUG)
CFLAGS = -g $(CFLAGS_DEBUG_MERGE) -I$(INC_DIR) -L$(LIB_DIR) 

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

SRCFILES := $(wildcard ${SRC_DIR}/*.c)
OBJFILES := $(patsubst %.c, %.o, ${SRCFILES})

all: test

test: $(OBJFILES)
	$(CC) -o $(BIN_DIR)/test $(APP_DIR)/test.c $(OBJFILES) $(CFLAGS)

clean:
	rm -f $(BIN_DIR)/* $(SRC_DIR)/*.o $(LIB_DIR)/$(STATIC_LIB)

$(OBJFILES): $(SRC_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)
