SUBDIRS = beacony

EXTRA_DIST = pathnames.h beacony.spec

installconf:
	@for app in $(SUBDIRS); do $(MAKE) -C $$app installconf; done

AX25_SYSCONFDIR=$(sysconfdir)/ax25
AX25_LOCALSTATEDIR=$(localstatedir)/ax25

AM_CPPFLAGS = -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\" 
