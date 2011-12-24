APPNAME     = FreeFileSync
prefix      = /usr
BINDIR      = $(DESTDIR)$(prefix)/bin
SHAREDIR    = $(DESTDIR)$(prefix)/share
APPSHAREDIR = $(SHAREDIR)/$(APPNAME)

COMMON_COMPILE_FLAGS = -Wall -pipe `pkg-config --cflags gtk+-2.0` -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I. -include "zen/i18n.h"
COMMON_LINK_FLAGS  = -O3 -pthread

#default build
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs std,aui --debug=no --unicode=yes` -lboost_thread

#static build used for precompiled release
ifeq ($(BUILD),release)
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs std,aui --debug=no --unicode=yes --static=yes` /usr/local/lib/libboost_thread.a
endif
#####################################################################################################


#support for GTKMM
FFS_CPPFLAGS += `pkg-config --cflags gtkmm-2.4`
LINKFLAGS += `pkg-config --libs gtkmm-2.4`

#support for SELinux (optional)
SELINUX_EXISTING=$(shell pkg-config --exists libselinux && echo YES)
ifeq ($(SELINUX_EXISTING),YES)
FFS_CPPFLAGS += `pkg-config --cflags libselinux ` -DHAVE_SELINUX
LINKFLAGS    += `pkg-config --libs libselinux`
endif

#support for Ubuntu Unity (optional)
UNITY_EXISTING=$(shell pkg-config --exists unity && echo YES)
ifeq ($(UNITY_EXISTING),YES)
FFS_CPPFLAGS += `pkg-config --cflags unity` -DHAVE_UBUNTU_UNITY
LINKFLAGS    += `pkg-config --libs unity`
endif

FILE_LIST=   #internal list of all *.cpp files needed for compilation
FILE_LIST+=algorithm.cpp
FILE_LIST+=application.cpp
FILE_LIST+=comparison.cpp
FILE_LIST+=file_hierarchy.cpp
FILE_LIST+=lib/binary.cpp
FILE_LIST+=lib/custom_grid.cpp
FILE_LIST+=lib/db_file.cpp
FILE_LIST+=lib/dir_lock.cpp
FILE_LIST+=lib/error_log.cpp
FILE_LIST+=lib/hard_filter.cpp
FILE_LIST+=lib/icon_buffer.cpp
FILE_LIST+=lib/localization.cpp
FILE_LIST+=lib/parallel_scan.cpp
FILE_LIST+=lib/process_xml.cpp
FILE_LIST+=lib/recycler.cpp
FILE_LIST+=lib/resolve_path.cpp
FILE_LIST+=lib/resources.cpp
FILE_LIST+=lib/statistics.cpp
FILE_LIST+=lib/status_handler.cpp
FILE_LIST+=lib/xml_base.cpp
FILE_LIST+=structures.cpp
FILE_LIST+=synchronization.cpp
FILE_LIST+=ui/folder_history_box.cpp
FILE_LIST+=ui/exec_finished_box.cpp
FILE_LIST+=ui/dir_name.cpp
FILE_LIST+=ui/batch_config.cpp
FILE_LIST+=ui/batch_status_handler.cpp
FILE_LIST+=ui/check_version.cpp
FILE_LIST+=ui/grid_view.cpp
FILE_LIST+=ui/gui_generated.cpp
FILE_LIST+=ui/gui_status_handler.cpp
FILE_LIST+=ui/main_dlg.cpp
FILE_LIST+=ui/msg_popup.cpp
FILE_LIST+=ui/progress_indicator.cpp
FILE_LIST+=ui/search.cpp
FILE_LIST+=ui/small_dlgs.cpp
FILE_LIST+=ui/sync_cfg.cpp
FILE_LIST+=ui/taskbar.cpp
FILE_LIST+=ui/tray_icon.cpp
FILE_LIST+=wx+/button.cpp
FILE_LIST+=wx+/format_unit.cpp
FILE_LIST+=wx+/graph.cpp
FILE_LIST+=wx+/tooltip.cpp
FILE_LIST+=zen/file_handling.cpp
FILE_LIST+=zen/file_id.cpp
FILE_LIST+=zen/file_io.cpp
FILE_LIST+=zen/file_traverser.cpp
FILE_LIST+=zen/zstring.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), OBJ/FFS_Release_GCC_Make/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))


all: FreeFileSync

init: 
	if [ ! -d ./OBJ ]; then mkdir OBJ; fi
	if [ ! -d ./OBJ/FFS_Release_GCC_Make ]; then mkdir OBJ/FFS_Release_GCC_Make; fi

%.dep : %.cpp 
#strip path information
	g++ $(FFS_CPPFLAGS) -c $< -o OBJ/FFS_Release_GCC_Make/$(subst .cpp,.o,$(notdir $<))

FreeFileSync: init $(DEP_LIST)
#respect linker order: wxWidgets libraries last
	g++ -o ./BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf OBJ/FFS_Release_GCC_Make
	rm -f BUILD/$(APPNAME)

install:
	if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); fi
	if [ ! -d $(APPSHAREDIR) ]; then mkdir -p $(APPSHAREDIR); fi

	cp BUILD/$(APPNAME) $(BINDIR)
	cp -R BUILD/Languages/ \
	BUILD/Help/ \
	BUILD/Compare_Complete.wav \
	BUILD/Sync_Complete.wav \
	BUILD/Resources.zip \
	BUILD/Changelog.txt \
	BUILD/License.txt \
	BUILD/styles.rc \
	$(APPSHAREDIR)
