
installconf:

bin_PROGRAMS = beacony

man_MANS = beacony.1

EXTRA_DIST = beacony.man
CLEANFILES = beacony.1 beacony.1.tmp

beacony.1 : beacony.man
	name_call=$$(echo call | sed -e '$(transform)')			\
	name_Call=$$(echo $$name_call | sed -r 's@^(.)@\U\1\E@') &&	\
	name_CALL=$$(echo $$name_call | sed -r 's@^(.*)@\U\1\E@') &&	\
	name_listen=$$(echo listen | sed -e '$(transform)')		\
	name_Listen=$$(echo $$name_listen | sed -r 's@^(.)@\U\1\E@') &&	\
	name_LISTEN=$$(echo $$name_listen | sed -r 's@^(.*)@\U\1\E@') &&\
	sed -e "s/@@@call@@@/$$name_call/g"				\
	    -e "s/@@@Call@@@/$$name_Call/g"				\
	    -e "s/@@@CALL@@@/$$name_CALL/g"				\
	    -e "s/@@@listen@@@/$$name_listen/g"				\
	    -e "s/@@@Listen@@@/$$name_Listen/g"				\
	    -e "s/@@@LISTEN@@@/$$name_LISTEN/g"				\
	    $(srcdir)/beacony.man > beacony.1.tmp &&			\
	mv beacony.1.tmp beacony.1;

LDADD = $(AX25_LIB) $(NCURSES_LIB)

beacony_SOURCES = 	\
	beacony.c	\
	beacony.h	\
	ax25dump.c	\
	kissdump.c      \
	utils.c
