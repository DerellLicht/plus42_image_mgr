USE_DEBUG = NO
USE_UNICODE = YES
USE_64BIT = NO

ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
else
#TOOLS=c:\mingw\bin
#TOOLS=c:\TDM-GCC-64\bin
TOOLS=c:\tdm32\bin
endif

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
CFLAGS=-Wall -O -g -mwindows -Weffc++ 
LFLAGS=
else
CFLAGS=-Wall -O2 -mwindows -Weffc++ 
LFLAGS=-s
endif
# CFLAGS += -m32
CFLAGS += -Wno-write-strings
CFLAGS += -Wno-literal-suffix
CFLAGS += -Wno-unused-function

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LFLAGS += -dUNICODE -d_UNICODE
LiFLAGS += -DUNICODE -D_UNICODE
endif

# link library files
LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
CSRC=pimage_mgr.cpp show_ref_image.cpp parse_layout_file.cpp show_image_list.cpp \
hyperlinks.cpp about.cpp \
der_libs/gdi_plus.cpp \
der_libs/gdiplus_setup.cpp \
der_libs/common_funcs.cpp \
der_libs/common_win.cpp \
der_libs/winmsgs.cpp \
der_libs/vlistview.cpp \
der_libs/cterminal.cpp \
der_libs/terminal.cpp \
der_libs/wthread.cpp \
der_libs/tooltips.cpp

# iface_lib.cpp 

OBJS = $(CSRC:.cpp=.o) rc.o

BASE=pimage_mgr
BIN=$(BASE).exe

#************************************************************
#%.o: %.cpp
#	g++ $(CFLAGS) -Weffc++ -c $< -o $@

%.o: %.cpp
	$(TOOLS)\g++ $(CFLAGS) -c $< -o $@

#************************************************************
all: $(BIN)

clean:
	rm -vf *.exe $(OBJS) *.zip *.bak *~

dist:
	rm -f *.zip
	zip $(BASE).zip $(BIN) $(BASE).chm README.md LICENSE \
   LandscapeRight.gif LandscapeRight.layout \
   Michaels-HP.gif Michaels-HP.layout \
   Plus42_Landscape_SGT.gif Plus42_Landscape_SGT.layout
	

wc:
	wc -l *.cpp *.rc

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CSRC)"

depend:
	makedepend $(CFLAGS) $(CSRC)

#************************************************************
#lodepng.o: lodepng.cpp
#	g++ $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(TOOLS)\g++ $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ -lgdi32 -lgdiplus -lcomctl32 -lhtmlhelp -lolepro32 -lole32 -luuid

rc.o: pimage_mgr.rc 
	$(TOOLS)\windres $< -O coff -o $@

# DO NOT DELETE

pimage_mgr.o: version.h resource.h der_libs/common.h der_libs/commonw.h
pimage_mgr.o: pimage_mgr.h der_libs/cterminal.h der_libs/vlistview.h
pimage_mgr.o: der_libs/terminal.h der_libs/winmsgs.h der_libs/gdiplus_setup.h
show_ref_image.o: resource.h der_libs/common.h der_libs/commonw.h
show_ref_image.o: pimage_mgr.h der_libs/gdi_plus.h der_libs/winmsgs.h
show_ref_image.o: der_libs/wthread.h
parse_layout_file.o: der_libs/common.h der_libs/commonw.h pimage_mgr.h
show_image_list.o: resource.h der_libs/common.h der_libs/commonw.h
show_image_list.o: pimage_mgr.h der_libs/winmsgs.h der_libs/wthread.h
hyperlinks.o: der_libs/iface_32_64.h hyperlinks.h
about.o: resource.h version.h der_libs/common.h pimage_mgr.h hyperlinks.h
der_libs/gdi_plus.o: der_libs/common.h der_libs/gdi_plus.h
der_libs/gdiplus_setup.o: der_libs/gdi_plus.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/common_win.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/vlistview.h
der_libs/cterminal.o: der_libs/common.h der_libs/commonw.h
der_libs/cterminal.o: der_libs/cterminal.h der_libs/vlistview.h
der_libs/terminal.o: der_libs/common.h der_libs/commonw.h
der_libs/terminal.o: der_libs/cterminal.h der_libs/vlistview.h
der_libs/terminal.o: der_libs/terminal.h der_libs/winmsgs.h
der_libs/wthread.o: der_libs/wthread.h
der_libs/tooltips.o: der_libs/iface_32_64.h der_libs/common.h
der_libs/tooltips.o: der_libs/tooltips.h
