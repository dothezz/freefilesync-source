APPNAME = RealtimeSync
prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

COMMON_COMPILE_FLAGS = -Wall -pipe `pkg-config --cflags gtk+-2.0` -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I../shared
COMMON_LINK_FLAGS  = -O3 -pthread

#default build
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes` -lboost_thread

#static build used for precompiled release
ifeq ($(BUILD),release)
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes --static=yes` /usr/local/lib/libboost_thread.a
endif

FILE_LIST=              #internal list of all *.cpp files needed for compilation
FILE_LIST+=application.cpp
FILE_LIST+=gui_generated.cpp
FILE_LIST+=main_dlg.cpp
FILE_LIST+=resources.cpp
FILE_LIST+=tray_menu.cpp
FILE_LIST+=watcher.cpp
FILE_LIST+=xml_proc.cpp
FILE_LIST+=xml_ffs.cpp
FILE_LIST+=../library/process_xml.cpp
FILE_LIST+=../structures.cpp
FILE_LIST+=../shared/util.cpp
FILE_LIST+=../shared/check_exist.cpp
FILE_LIST+=../shared/i18n.cpp
FILE_LIST+=../shared/localization.cpp
FILE_LIST+=../shared/dir_watcher.cpp
FILE_LIST+=../shared/last_error.cpp
FILE_LIST+=../shared/dir_name.cpp
FILE_LIST+=../shared/zstring.cpp
FILE_LIST+=../shared/xml_base.cpp
FILE_LIST+=../shared/custom_button.cpp
FILE_LIST+=../shared/file_handling.cpp
FILE_LIST+=../shared/resolve_path.cpp
FILE_LIST+=../shared/file_traverser.cpp
FILE_LIST+=../shared/standard_paths.cpp
FILE_LIST+=../shared/help_provider.cpp
FILE_LIST+=../shared/file_io.cpp

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
