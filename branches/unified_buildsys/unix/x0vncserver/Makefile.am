bin_PROGRAMS = x0vncserver

man1_MANS = x0vncserver.man

HDRS = Geometry.h Image.h PollingManager.h PollingScheduler.h TimeMillis.h \
	XPixelBuffer.h

x0vncserver_SOURCES = $(HDRS) Image.cxx TimeMillis.cxx PollingScheduler.cxx \
	PollingManager.cxx Geometry.cxx XPixelBuffer.cxx x0vncserver.cxx \
	../vncconfig/QueryConnectDialog.cxx buildtime.c

# X_CFLAGS are really CPPFLAGS
x0vncserver_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/common/rfb \
	-I$(top_srcdir)/unix/tx -I$(top_srcdir)/unix/vncconfig \
	-I$(top_srcdir)/unix @XTEST_DEFINE@ @READDISPLAY_DEFINE@ \
	@MITSHM_DEFINE@ @X_CFLAGS@

x0vncserver_LDADD = $(top_srcdir)/common/rfb/librfb.la \
	$(top_srcdir)/common/network/libnetwork.la \
	$(top_srcdir)/common/rdr/librdr.la \
	$(top_srcdir)/unix/tx/libtx.la @INET_LIB@ @X_PRE_LIBS@ @X_LIBS@ \
	@XTEST_LIB@ -lXext -lX11 @X_EXTRA_LIBS@

EXTRA_DIST = x0vncserver.man
