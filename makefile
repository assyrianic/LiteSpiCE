# ----------------------------
# Makefile Options
# ----------------------------

NAME = NODEVOLT
ICON = icon.png
DESCRIPTION = "Node-Voltage Method Analyzer"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
