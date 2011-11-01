APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

COMMON_COMPILE_FLAGS = -Wall -pipe `pkg-config --cflags gtk+-2.0` -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I.. -include "../zen/i18n.h"
COMMON_LINK_FLAGS  = -O3 -pthread

#default build
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes` -lboost_thread

#static build used for precompiled release
ifeq ($(BUILD),release)
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes --static=yes` /usr/local/lib/libboost_thread.a
endif

FILE_LIST=    #internal list of all *.cpp files needed for compilation
FILE_LIST+=application.cpp
FILE_LIST+=gui_generated.cpp
FILE_LIST+=main_dlg.cpp
FILE_LIST+=resources.cpp
FILE_LIST+=tray_menu.cpp
FILE_LIST+=watcher.cpp
FILE_LIST+=xml_ffs.cpp
FILE_LIST+=xml_proc.cpp
FILE_LIST+=../lib/dir_name.cpp
FILE_LIST+=../lib/folder_history_box.cpp
FILE_LIST+=../lib/localization.cpp
FILE_LIST+=../lib/process_xml.cpp
FILE_LIST+=../lib/resolve_path.cpp
FILE_LIST+=../lib/xml_base.cpp
FILE_LIST+=../structures.cpp
FILE_LIST+=../wx+/button.cpp
FILE_LIST+=../wx+/format_unit.cpp
FILE_LIST+=../zen/dir_watcher.cpp
FILE_LIST+=../zen/file_handling.cpp
FILE_LIST+=../zen/file_io.cpp
FILE_LIST+=../zen/file_traverser.cpp
FILE_LIST+=../zen/zstring.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), ../OBJ/RTS_Release_GCC_Make/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))


all: RealtimeSync

init: 
	if [ ! -d ../OBJ ]; then mkdir ../OBJ; fi
	if [ ! -d ../OBJ/RTS_Release_GCC_Make ]; then mkdir ../OBJ/RTS_Release_GCC_Make; fi

%.dep : %.cpp 
 #strip path information
	g++ $(FFS_CPPFLAGS) -c $< -o ../OBJ/RTS_Release_GCC_Make/$(subst .cpp,.o,$(notdir $<))

RealtimeSync: init $(DEP_LIST)
#respect linker order: wxWidgets libraries last
	g++ -o ../BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf ../OBJ/RTS_Release_GCC_Make
	rm -f ../BUILD/$(APPNAME)

install:
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR); fi

	cp ../BUILD/$(APPNAME) $(BINDIR)
