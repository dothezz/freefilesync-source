APPNAME     = FreeFileSync
prefix      = /usr
BINDIR      = $(DESTDIR)$(prefix)/bin
SHAREDIR    = $(DESTDIR)$(prefix)/share
APPSHAREDIR = $(SHAREDIR)/$(APPNAME)
DOCSHAREDIR = $(SHAREDIR)/doc/$(APPNAME)

CPPFLAGS  = -Wall -pipe -O3 -pthread -std=gnu++0x -DNDEBUG -DwxUSE_UNICODE -DFFS_LINUX -DZEN_PLATFORM_OTHER -DWXINTL_NO_GETTEXT_MACRO -I. -include "zen/i18n.h" -include "zen/warn_static.h"
LINKFLAGS = -pthread -lrt -lz

ifeq ($(BUILD),release)
#static wxWidgets and boost library linkage for precompiled release
CPPFLAGS  += `wx-config --cxxflags --debug=no --unicode=yes --static=yes`
LINKFLAGS += `wx-config --libs std,aui --debug=no --unicode=yes --static=yes` -Wl,-Bstatic -lboost_thread -lboost_system -Wl,-Bdynamic
else
#default build
CPPFLAGS  += `wx-config --cxxflags --debug=no --unicode=yes`
LINKFLAGS += `wx-config --libs std,aui --debug=no --unicode=yes` -lboost_thread -lboost_system
endif
#####################################################################################################

#Gtk - recycler/icon loading
CPPFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs gtk+-2.0`

#support for SELinux (optional)
SELINUX_EXISTING=$(shell pkg-config --exists libselinux && echo YES)
ifeq ($(SELINUX_EXISTING),YES)
CPPFLAGS  += `pkg-config --cflags libselinux` -DHAVE_SELINUX
LINKFLAGS += `pkg-config --libs libselinux`
endif

#support for Ubuntu Unity (optional)
UNITY_EXISTING=$(shell pkg-config --exists unity && echo YES)
ifeq ($(UNITY_EXISTING),YES)
CPPFLAGS  += `pkg-config --cflags unity` -DHAVE_UBUNTU_UNITY
LINKFLAGS += `pkg-config --libs unity`
endif

CPP_LIST= #internal list of all *.cpp files needed for compilation
CPP_LIST+=algorithm.cpp
CPP_LIST+=application.cpp
CPP_LIST+=comparison.cpp
CPP_LIST+=structures.cpp
CPP_LIST+=synchronization.cpp
CPP_LIST+=file_hierarchy.cpp
CPP_LIST+=ui/custom_grid.cpp
CPP_LIST+=ui/folder_history_box.cpp
CPP_LIST+=ui/exec_finished_box.cpp
CPP_LIST+=ui/dir_name.cpp
CPP_LIST+=ui/batch_config.cpp
CPP_LIST+=ui/batch_status_handler.cpp
CPP_LIST+=ui/check_version.cpp
CPP_LIST+=ui/grid_view.cpp
CPP_LIST+=ui/tree_view.cpp
CPP_LIST+=ui/gui_generated.cpp
CPP_LIST+=ui/gui_status_handler.cpp
CPP_LIST+=ui/main_dlg.cpp
CPP_LIST+=ui/msg_popup.cpp
CPP_LIST+=ui/progress_indicator.cpp
CPP_LIST+=ui/search.cpp
CPP_LIST+=ui/small_dlgs.cpp
CPP_LIST+=ui/sync_cfg.cpp
CPP_LIST+=ui/taskbar.cpp
CPP_LIST+=ui/triple_splitter.cpp
CPP_LIST+=ui/tray_icon.cpp
CPP_LIST+=lib/binary.cpp
CPP_LIST+=lib/db_file.cpp
CPP_LIST+=lib/dir_lock.cpp
CPP_LIST+=lib/hard_filter.cpp
CPP_LIST+=lib/icon_buffer.cpp
CPP_LIST+=lib/localization.cpp
CPP_LIST+=lib/parallel_scan.cpp
CPP_LIST+=lib/process_xml.cpp
CPP_LIST+=lib/resolve_path.cpp
CPP_LIST+=lib/resources.cpp
CPP_LIST+=lib/perf_check.cpp
CPP_LIST+=lib/status_handler.cpp
CPP_LIST+=lib/versioning.cpp
CPP_LIST+=lib/xml_base.cpp
CPP_LIST+=zen/recycler.cpp
CPP_LIST+=zen/file_handling.cpp
CPP_LIST+=zen/file_id.cpp
CPP_LIST+=zen/file_io.cpp
CPP_LIST+=zen/file_traverser.cpp
CPP_LIST+=zen/zstring.cpp
CPP_LIST+=zen/format_unit.cpp
CPP_LIST+=wx+/grid.cpp
CPP_LIST+=wx+/button.cpp
CPP_LIST+=wx+/graph.cpp
CPP_LIST+=wx+/tooltip.cpp
CPP_LIST+=wx+/zlib_wrap.cpp

#list of all *.o files
OBJECT_LIST=$(CPP_LIST:%.cpp=OBJ/FFS_Release_GCC_Make/%.o)

all: FreeFileSync

OBJ/FFS_Release_GCC_Make/%.o : %.cpp
	mkdir -p $(dir $@)
	g++ $(CPPFLAGS) -c $< -o $@

FreeFileSync: $(OBJECT_LIST)
	g++ -o ./BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
#-f doesn't work when deleting directories
	if [ -d OBJ/FFS_Release_GCC_Make ]; then rm -rf OBJ/FFS_Release_GCC_Make; fi
	rm -f BUILD/$(APPNAME)

install:
	mkdir -p $(BINDIR)
	cp BUILD/$(APPNAME) $(BINDIR)

	mkdir -p $(APPSHAREDIR)
	cp -R BUILD/Languages/ \
	BUILD/Help/ \
	BUILD/Compare_Complete.wav \
	BUILD/Sync_Complete.wav \
	BUILD/Resources.zip \
	BUILD/styles.rc \
	$(APPSHAREDIR)

	mkdir -p $(DOCSHAREDIR)
	cp BUILD/Changelog.txt $(DOCSHAREDIR)/changelog
	gzip $(DOCSHAREDIR)/changelog
