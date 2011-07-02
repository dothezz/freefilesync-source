APPNAME     = FreeFileSync
prefix      = /usr
BINDIR      = $(DESTDIR)$(prefix)/bin
SHAREDIR    = $(DESTDIR)$(prefix)/share
APPSHAREDIR = $(SHAREDIR)/$(APPNAME)

COMMON_COMPILE_FLAGS = -Wall -pipe `pkg-config --cflags gtk+-2.0` -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I./shared
COMMON_LINK_FLAGS  = -O3 -pthread

#default build
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes` -lboost_thread

#static build used for precompiled release
ifeq ($(BUILD),release)
FFS_CPPFLAGS = $(COMMON_COMPILE_FLAGS) `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS    = $(COMMON_LINK_FLAGS) `wx-config --libs --debug=no --unicode=yes --static=yes` /usr/local/lib/libboost_thread.a
endif
#####################################################################################################


#support for GTKMM
FFS_CPPFLAGS+=`pkg-config --cflags gtkmm-2.4`
LINKFLAGS+=`pkg-config --libs gtkmm-2.4`

#support for SELinux (optional)
SELINUX_EXISTING=$(shell pkg-config --exists libselinux && echo YES)
ifeq ($(SELINUX_EXISTING),YES)
FFS_CPPFLAGS += -DHAVE_SELINUX
endif

FILE_LIST=              #internal list of all *.cpp files needed for compilation
FILE_LIST+=structures.cpp
FILE_LIST+=algorithm.cpp
FILE_LIST+=comparison.cpp
FILE_LIST+=synchronization.cpp
FILE_LIST+=file_hierarchy.cpp
FILE_LIST+=application.cpp
FILE_LIST+=ui/gui_generated.cpp
FILE_LIST+=shared/util.cpp
FILE_LIST+=ui/grid_view.cpp
FILE_LIST+=ui/main_dlg.cpp
FILE_LIST+=ui/batch_config.cpp
FILE_LIST+=ui/sync_cfg.cpp
FILE_LIST+=ui/check_version.cpp
FILE_LIST+=ui/batch_status_handler.cpp
FILE_LIST+=ui/gui_status_handler.cpp
FILE_LIST+=ui/tray_icon.cpp
FILE_LIST+=ui/search.cpp
FILE_LIST+=ui/switch_to_gui.cpp
FILE_LIST+=ui/msg_popup.cpp
FILE_LIST+=ui/progress_indicator.cpp
FILE_LIST+=library/custom_grid.cpp
FILE_LIST+=library/error_log.cpp
FILE_LIST+=library/status_handler.cpp
FILE_LIST+=library/resources.cpp
FILE_LIST+=ui/small_dlgs.cpp
FILE_LIST+=library/process_xml.cpp
FILE_LIST+=library/icon_buffer.cpp
FILE_LIST+=library/statistics.cpp
FILE_LIST+=library/hard_filter.cpp
FILE_LIST+=library/binary.cpp
FILE_LIST+=library/db_file.cpp
FILE_LIST+=library/dir_lock.cpp
FILE_LIST+=shared/i18n.cpp
FILE_LIST+=shared/localization.cpp
FILE_LIST+=shared/file_io.cpp
FILE_LIST+=shared/dir_name.cpp
FILE_LIST+=shared/guid.cpp
FILE_LIST+=shared/xml_base.cpp
FILE_LIST+=shared/check_exist.cpp
FILE_LIST+=shared/global_func.cpp
FILE_LIST+=shared/last_error.cpp
FILE_LIST+=shared/custom_tooltip.cpp
FILE_LIST+=shared/file_handling.cpp
FILE_LIST+=shared/resolve_path.cpp
FILE_LIST+=shared/file_traverser.cpp
FILE_LIST+=shared/standard_paths.cpp
FILE_LIST+=shared/zstring.cpp
FILE_LIST+=shared/app_main.cpp
FILE_LIST+=shared/custom_button.cpp
FILE_LIST+=shared/toggle_button.cpp
FILE_LIST+=shared/custom_combo_box.cpp
FILE_LIST+=shared/serialize.cpp
FILE_LIST+=shared/file_id.cpp
FILE_LIST+=shared/recycler.cpp
FILE_LIST+=shared/help_provider.cpp

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
	BUILD/Resources.dat \
	BUILD/Changelog.txt \
	BUILD/License.txt \
	BUILD/styles.rc \
	$(APPSHAREDIR)
