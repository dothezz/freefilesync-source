APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

COMMON_COMPILE_FLAGS = -Wall -pipe -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I.. -include "../zen/i18n.h"
COMMON_LINK_FLAGS    = -pthread -lrt

#default build
CPPFLAGS  = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes` -lboost_thread

#static std library linkage used for precompiled release
ifeq ($(BUILD),release)
CPPFLAGS  = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes --static=yes` /usr/local/lib/libboost_thread.a
endif
#####################################################################################################

#Gtk - no button border
CPPFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs gtk+-2.0`

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
CPP_LIST+=../zen/dir_watcher.cpp
CPP_LIST+=../zen/file_handling.cpp
CPP_LIST+=../zen/file_io.cpp
CPP_LIST+=../zen/file_traverser.cpp
CPP_LIST+=../zen/zstring.cpp
CPP_LIST+=../wx+/button.cpp
CPP_LIST+=../wx+/format_unit.cpp

#list of all *.o files
OBJECT_LIST=$(CPP_LIST:%.cpp=../OBJ/RTS_Release_GCC_Make/RTS/%.o)

all: RealtimeSync

../OBJ/RTS_Release_GCC_Make/RTS/%.o : %.cpp
	mkdir -p $(dir $@)
	g++ $(CPPFLAGS) -c $< -o $@

RealtimeSync: $(OBJECT_LIST)
	g++ -o ../BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
#-f doesn't work when deleting directories
	if [ -d ../OBJ/RTS_Release_GCC_Make ]; then rm -rf ../OBJ/RTS_Release_GCC_Make; fi
	rm -f ../BUILD/$(APPNAME)

install:
	mkdir -p $(BINDIR)
	cp ../BUILD/$(APPNAME) $(BINDIR)
