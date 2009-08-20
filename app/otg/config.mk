#
# This file is sourced by all makefiles. Make any system dependent 
# changes only here
#

# Location of APT repository
REPOSITORY = $(USER)@repository1
REPOSITORY_ROOT = /export/web/orbit/dists/testing/main

WITH_OML = 1

#
BUILD_DIR = $(TOP_DIR)/build
DIST_DIR = $(TOP_DIR)/dist


CPP = g++
CC = g++
CFLAGS = -g -Wall -O

ifdef WITH_OML
CFLAGS += -DWITH_OML 
endif
LDFLAGS = -L/home/gjourjon/oml2/oml2/build/oml_client/lib/ -L$(BUILD_LIB_DIR)
#CC	= gcc
#CPP	= g++

#CFLAGS = -g -Wall -O 

