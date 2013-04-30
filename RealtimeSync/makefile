BLAH_BLUBB_123=
#for some buggy reason the first row in the make file has no effect on Suse Linux! => make sure there's no important command
APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

CXXFLAGS  = -std=c++11 -Wall -pipe -O3 -DNDEBUG -DwxUSE_UNICODE -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I.. -include "zen/i18n.h" -include "zen/warn_static.h"
LINKFLAGS =

#distinguish Linux/Mac builds
OPERATING_SYSTEM_NAME := $(shell uname)

#################### Linux ############################
ifeq ($(OPERATING_SYSTEM_NAME), Linux)
COMPILER_BIN=g++ -pthread
CXXFLAGS += -DFFS_LINUX

#Gtk - support "no button border"
CXXFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs   gtk+-2.0`

ifeq ($(BUILD),Launchpad)
#default build/Launchpad
CXXFLAGS  += `wx-config --cxxflags --debug=no`
LINKFLAGS += `wx-config --libs     --debug=no` -lboost_thread -lboost_system -lz
else
#static wxWidgets and boost library linkage for precompiled release
WX_CONFIG_BIN =$(HOME)/Desktop/wxGTK-2.8.12/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost_1_53_0
BOOST_LIB_DIR =$(HOME)/Desktop/boost_1_53_0/stage/lib

CXXFLAGS  += `$(WX_CONFIG_BIN) --cxxflags --debug=no --static=yes`
LINKFLAGS += `$(WX_CONFIG_BIN) --libs     --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a -lX11
endif

endif
#################### OS X ############################
ifeq ($(OPERATING_SYSTEM_NAME), Darwin)
COMPILER_BIN=clang++ -stdlib=libc++
CXXFLAGS += -DFFS_MAC

WX_CONFIG_BIN =$(HOME)/Desktop/wxWidgets-2.9.4/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost_1_53_0
BOOST_LIB_DIR =$(HOME)/Desktop/boost_1_53_0/stage/lib
MACOS_SDK     =-mmacosx-version-min=10.7 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk
#-Wl,-Bstatic not supported on OSX!

# link wxWidgets and boost statically -> check dependencies with: otool -L RealtimeSync
CXXFLAGS  += $(MACOS_SDK) `$(WX_CONFIG_BIN) --cxxflags --debug=no --static=yes`
LINKFLAGS += $(MACOS_SDK) `$(WX_CONFIG_BIN) --libs     --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a

endif
#####################################################################################################

CPP_LIST= #internal list of all *.cpp files needed for compilation
CPP_LIST+=application.cpp
CPP_LIST+=gui_generated.cpp
CPP_LIST+=main_dlg.cpp
CPP_LIST+=resources.cpp
CPP_LIST+=tray_menu.cpp
CPP_LIST+=watcher.cpp
CPP_LIST+=xml_ffs.cpp
CPP_LIST+=xml_proc.cpp
CPP_LIST+=../structures.cpp
CPP_LIST+=../ui/dir_name.cpp
CPP_LIST+=../ui/folder_history_box.cpp
CPP_LIST+=../lib/localization.cpp
CPP_LIST+=../lib/process_xml.cpp
CPP_LIST+=../lib/resolve_path.cpp
CPP_LIST+=../lib/xml_base.cpp
CPP_LIST+=../lib/ffs_paths.cpp
CPP_LIST+=../zen/dir_watcher.cpp
CPP_LIST+=../zen/file_handling.cpp
CPP_LIST+=../zen/file_io.cpp
CPP_LIST+=../zen/file_traverser.cpp
CPP_LIST+=../zen/zstring.cpp
CPP_LIST+=../wx+/button.cpp

#list of all *.o files (we need the "RTS" subdirectory to handle "../*.cpp" files
OBJECT_LIST=$(CPP_LIST:%.cpp=../OBJ/RTS_Release_GCC_Make/RTS/%.o)

all: RealtimeSync

../OBJ/RTS_Release_GCC_Make/RTS/%.o : %.cpp
	mkdir -p $(dir $@)
	$(COMPILER_BIN) $(CXXFLAGS) -c $< -o $@

RealtimeSync: $(OBJECT_LIST)
	$(COMPILER_BIN) -o ../BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf ../OBJ/RTS_Release_GCC_Make
	rm -f ../BUILD/$(APPNAME)
	rm -f ../wx+/pch.h.gch

install:
	mkdir -p $(BINDIR)
	cp ../BUILD/$(APPNAME) $(BINDIR)
