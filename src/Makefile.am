AUTOMAKE_OPTIONS = foreign subdir-objects
bin_PROGRAMS = mcserverd
CFLAGS = -O2 -g -Wno-unused-result \
		 -Werror -Wall -Wunused-parameter
mcserverd_SOURCES = main.c \
						common/common.h \
						common/list.h \
						common/list.c \
						common/queue.h \
						config/config.h \
						config/config.c \
						config/ini.h \
						config/ini.c \
						log/log.h \
						log/log.c \
						network/ssh.h \
						network/netowtk.h \
						network/network.c \
						network/session.h \
						network/session.c \
						server/cmdline.h \
						server/cmdline.c \
						server/game-server.h \
						server/game-server.c \
						server/server.h \
						server/server.c


