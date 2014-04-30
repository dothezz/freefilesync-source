BLAH_BLUBB_123=
#for some buggy reason the first row in the make file has no effect on Suse Linux! => make sure there's no important command
APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

CXXFLAGS  = -std=c++11 -pipe -O3 -DNDEBUG -DWXINTL_NO_GETTEXT_MACRO -I../../.. -I../../../zenXml -include "zen/i18n.h" -include "zen/warn_static.h" \
-Wall -Winit-self -Wredundant-decls -Wmissing-include-dirs -Wswitch-enum -Wmain

LINKFLAGS =

#distinguish Linux/Mac builds
OPERATING_SYSTEM_NAME := $(shell uname)

#################### Linux ############################
ifeq ($(OPERATING_SYSTEM_NAME), Linux)
COMPILER_BIN=g++ -pthread
CXXFLAGS += -DZEN_LINUX -Wcast-align -Wshadow

#Gtk - support "no button border"
CXXFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs   gtk+-2.0`

ifeq ($(BUILD),Launchpad)
#default build/Launchpad
CXXFLAGS  += `wx-config --cxxflags --debug=no`
LINKFLAGS += `wx-config --libs     --debug=no` -lboost_thread -lboost_system -lz
else
#static wxWidgets and boost library linkage for precompiled release
WX_CONFIG_BIN =$(HOME)/Desktop/wxWidgets/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost
BOOST_LIB_DIR =$(HOME)/Desktop/boost/stage/lib

CXXFLAGS  += `$(WX_CONFIG_BIN) --cxxflags --debug=no --static=yes`
LINKFLAGS += `$(WX_CONFIG_BIN) --libs     --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a -lX11
endif

endif
#################### OS X ############################
ifeq ($(OPERATING_SYSTEM_NAME), Darwin)
COMPILER_BIN=clang++ -stdlib=libc++
CXXFLAGS += -DZEN_MAC -Wbind-to-temporary-copy -Wambiguous-member-template -Wextra-tokens

WX_CONFIG_BIN =$(HOME)/Desktop/wxWidgets/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost
BOOST_LIB_DIR =$(HOME)/Desktop/boost/stage/lib
MACOS_SDK     =-mmacosx-version-min=10.7 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk
#-Wl,-Bstatic not supported on OSX!

# link wxWidgets and boost statically -> check dependencies with: otool -L RealtimeSync
CXXFLAGS  += $(MACOS_SDK) `$(WX_CONFIG_BIN) --cxxflags --debug=no --static=yes`
LINKFLAGS += $(MACOS_SDK) `$(WX_CONFIG_BIN) --libs     --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a

endif
######################################################

CPP_LIST= #internal list of all *.cpp files needed for compilation
CPP_LIST+=application.cpp
CPP_LIST+=gui_generated.cpp
CPP_LIST+=main_dlg.cpp
CPP_LIST+=tray_menu.cpp
CPP_LIST+=monitor.cpp
CPP_LIST+=xml_proc.cpp
CPP_LIST+=../structures.cpp
CPP_LIST+=../ui/dir_name.cpp
CPP_LIST+=../ui/folder_history_box.cpp
CPP_LIST+=../lib/localization.cpp
CPP_LIST+=../lib/process_xml.cpp
CPP_LIST+=../lib/resolve_path.cpp
CPP_LIST+=../lib/ffs_paths.cpp
CPP_LIST+=../../../zen/xml_io.cpp
CPP_LIST+=../../../zen/dir_watcher.cpp
CPP_LIST+=../../../zen/file_handling.cpp
CPP_LIST+=../../../zen/file_io.cpp
CPP_LIST+=../../../zen/file_traverser.cpp
CPP_LIST+=../../../zen/zstring.cpp
CPP_LIST+=../../../zen/format_unit.cpp
CPP_LIST+=../../../wx+/image_tools.cpp
CPP_LIST+=../../../wx+/image_resources.cpp
CPP_LIST+=../../../wx+/popup_dlg.cpp
CPP_LIST+=../../../wx+/popup_dlg_generated.cpp

#list of all *.o files (we need the "RTS" subdirectory to handle "../*.cpp" files
OBJECT_LIST=$(CPP_LIST:%.cpp=../../Obj/RTS_GCC_Make_Release/ffs/src/rts/%.o)

all: RealtimeSync

../../Obj/RTS_GCC_Make_Release/ffs/src/rts/%.o : %.cpp
	mkdir -p $(dir $@)
	$(COMPILER_BIN) $(CXXFLAGS) -c $< -o $@

RealtimeSync: $(OBJECT_LIST)
	$(COMPILER_BIN) -o ../../Build/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf ../../Obj/RTS_GCC_Make_Release
	rm -f ../../Build/$(APPNAME)
	rm -f ../../../wx+/pch.h.gch

install:
	mkdir -p $(BINDIR)
	cp ../../Build/$(APPNAME) $(BINDIR)
