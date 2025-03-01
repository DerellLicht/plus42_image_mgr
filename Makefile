USE_DEBUG = NO
USE_BMP = YES
USE_PNG = YES

#*****************************************************************************
# notes on compiler quirks, using MinGW/G++ V4.3.3
# if I build with -O3, I get following warnings from g++ :
#   wfuncs.cpp: In function 'int light_a_flare(HWND__*)':
#   wfuncs.cpp:338: warning: array subscript is above array bounds
# where light_a_flare() starts at line 779 !!
# If I build with -O2, I get no such warnings.
# In either case, PcLint V9 is giving no warnings on this code.
#*****************************************************************************

ifeq ($(USE_DEBUG),YES)
CFLAGS=-Wall -O -g -mwindows 
LFLAGS=
else
CFLAGS=-Wall -O2 -mwindows 
LFLAGS=-s
endif
CFLAGS += -Wno-write-strings

# link library files
LiFLAGS = -Ider_libs
CFLAGS += -Ider_libs
CSRC=pimage_mgr.cpp terminal.cpp lodepng.cpp lode_png.cpp hyperlinks.cpp about.cpp \
show_ref_image.cpp \
der_libs/common_funcs.cpp \
der_libs/common_win.cpp \
der_libs/winmsgs.cpp \
der_libs/vlistview.cpp \
der_libs/cterminal.cpp \
der_libs/wthread.cpp \
der_libs/statbar.cpp \
der_libs/tooltips.cpp


# iface_lib.cpp 

OBJS = $(CSRC:.cpp=.o) rc.o

BASE=pimage_mgr
BIN=$(BASE).exe

#************************************************************
%.o: %.cpp
	g++ $(CFLAGS) -Weffc++ -c $< -o $@

#************************************************************
all: $(BIN)

clean:
	rm -vf *.exe $(OBJS) *.zip *.bak *~

dist:
	rm -f *.zip
	zip $(BASE).zip $(BIN) README.md

wc:
	wc -l *.cpp *.rc

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CSRC)"

lint8:
	cmd /C "c:\lint8\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint8 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CSRC)"

depend:
	makedepend $(CFLAGS) $(CSRC)

#************************************************************
lodepng.o: lodepng.cpp
	g++ $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	g++ $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ -lgdi32 -lcomctl32 -lhtmlhelp -lolepro32 -lole32 -luuid

rc.o: pimage_mgr.rc 
	windres $< -O coff -o $@

# DO NOT DELETE

pimage_mgr.o: version.h resource.h der_libs/common.h der_libs/commonw.h
pimage_mgr.o: pimage_mgr.h terminal.h der_libs/winmsgs.h
terminal.o: resource.h der_libs/common.h der_libs/commonw.h pimage_mgr.h
terminal.o: der_libs/cterminal.h der_libs/vlistview.h terminal.h
terminal.o: der_libs/winmsgs.h
lodepng.o: lodepng.h
lode_png.o: der_libs/common.h lodepng.h lode_png.h
hyperlinks.o: der_libs/iface_32_64.h hyperlinks.h
about.o: resource.h version.h der_libs/common.h pimage_mgr.h hyperlinks.h
show_ref_image.o: resource.h der_libs/common.h pimage_mgr.h
show_ref_image.o: der_libs/statbar.h der_libs/winmsgs.h der_libs/wthread.h
show_ref_image.o: lodepng.h lode_png.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/common_win.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/vlistview.h
der_libs/cterminal.o: der_libs/common.h der_libs/commonw.h
der_libs/cterminal.o: der_libs/cterminal.h der_libs/vlistview.h
der_libs/wthread.o: der_libs/wthread.h
der_libs/statbar.o: der_libs/common.h der_libs/commonw.h der_libs/statbar.h
der_libs/tooltips.o: der_libs/iface_32_64.h der_libs/common.h
der_libs/tooltips.o: der_libs/tooltips.h
