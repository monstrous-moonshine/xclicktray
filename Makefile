CFLAGS = -Wall -Wextra -Os
LDLIBS = -lX11

all: xclicktray

clean:
	$(RM) xclicktray

.PHONY: all clean
