BLAH_BLUBB_123=
#for some buggy reason the first row in the make file has no effect on Suse Linux! => make sure there's no important command
APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

CXXFLAGS  = -std=c++11 -pipe -DWXINTL_NO_GETTEXT_MACRO -I../../.. -I../../../zenXml -include "zen/i18n.h" -include "zen/warn_static.h" \
-Wall -Winit-self -Wredundant-decls -Wmissing-include-dirs -Wswitch-enum -Wmain

release launchpad: CXXFLAGS  += -O3 -DNDEBUG
debug:             CXXFLAGS  += -g -D__WXDEBUG__

LINKFLAGS =

#distinguish Linux/Mac builds
OPERATING_SYSTEM_NAME := $(shell uname)

#################### Linux ############################
ifeq ($(OPERATING_SYSTEM_NAME), Linux)
COMPILER_BIN = g++ -pthread
CXXFLAGS += -DZEN_LINUX -Wcast-align -Wshadow

#Gtk - support "no button border"
CXXFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs   gtk+-2.0`

release launchpad: LINKFLAGS += -s
#release debug: LINKFLAGS += -lX11
endif
#################### OS X ############################
ifeq ($(OPERATING_SYSTEM_NAME), Darwin)
COMPILER_BIN = clang++ -stdlib=libc++ -mmacosx-version-min=10.7 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk
CXXFLAGS += -DZEN_MAC -Wbind-to-temporary-copy -Wambiguous-member-template -Wextra-tokens

#-Wl,-Bstatic not supported on OSX!
# check dependencies with: otool -L FreeFileSync
endif
######################################################

WX_CONFIG_LIB =$(HOME)/Desktop/wxWidgets/lib
BOOST_LIB_DIR =$(HOME)/Desktop/boost/stage/lib
CXXFLAGS  += -I$(HOME)/Desktop/boost

release:   CXXFLAGS  += `$(WX_CONFIG_LIB)/release/bin/wx-config --cxxflags --debug=no  --static=yes`
debug:     CXXFLAGS  += `$(WX_CONFIG_LIB)/debug/bin/wx-config   --cxxflags --debug=yes --static=yes`
launchpad: CXXFLAGS  += `wx-config                              --cxxflags --debug=no`

release:   LINKFLAGS += `$(WX_CONFIG_LIB)/release/bin/wx-config --libs std, aui --debug=no  --static=yes`
debug:     LINKFLAGS += `$(WX_CONFIG_LIB)/debug/bin/wx-config   --libs std, aui --debug=yes --static=yes`
launchpad: LINKFLAGS += `wx-config                              --libs std, aui --debug=no`

release debug: LINKFLAGS += $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a
launchpad:     LINKFLAGS += -lboost_thread -lboost_system -lz

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

all: release

release:   RealtimeSync
debug:     RealtimeSync
launchpad: RealtimeSync

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
