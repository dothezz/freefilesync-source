BLAH_BLUBB_123=
#for some buggy reason the first row in the make file has no effect on Suse Linux! => make sure there's no important command
APPNAME     = FreeFileSync
prefix      = /usr
BINDIR      = $(DESTDIR)$(prefix)/bin
SHAREDIR    = $(DESTDIR)$(prefix)/share
APPSHAREDIR = $(SHAREDIR)/$(APPNAME)
DOCSHAREDIR = $(SHAREDIR)/doc/$(APPNAME)

CXXFLAGS  = -std=c++11 -Wall -pipe -O3 -DNDEBUG -DwxUSE_UNICODE -DWXINTL_NO_GETTEXT_MACRO -I. -include "zen/i18n.h" -include "zen/warn_static.h"
LINKFLAGS =

#distinguish Linux/OSX builds
OPERATING_SYSTEM_NAME := $(shell uname)

#################### Linux ############################
ifeq ($(OPERATING_SYSTEM_NAME), Linux)
COMPILER_BIN=g++ -pthread
CXXFLAGS += -DZEN_LINUX

#Gtk - support recycler/icon loading/no button border/grid scrolling
CXXFLAGS  += `pkg-config --cflags gtk+-2.0`
LINKFLAGS += `pkg-config --libs   gtk+-2.0`

#support for SELinux (optional)
SELINUX_EXISTING=$(shell pkg-config --exists libselinux && echo YES)
ifeq ($(SELINUX_EXISTING),YES)
CXXFLAGS  += `pkg-config --cflags libselinux` -DHAVE_SELINUX
LINKFLAGS += `pkg-config --libs libselinux`
endif

#support for Ubuntu Unity (optional)
UNITY_EXISTING=$(shell pkg-config --exists unity && echo YES)
ifeq ($(UNITY_EXISTING),YES)
CXXFLAGS  += `pkg-config --cflags unity` -DHAVE_UBUNTU_UNITY
LINKFLAGS += `pkg-config --libs unity`
endif

ifeq ($(BUILD),Launchpad)
#default build/Launchpad
CXXFLAGS  += `wx-config --cxxflags      --debug=no`
LINKFLAGS += `wx-config --libs std, aui --debug=no` -lboost_thread -lboost_system -lz
else
#static wxWidgets and boost library linkage for precompiled release
WX_CONFIG_BIN =$(HOME)/Desktop/wxGTK-2.8.12/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost_1_54_0
BOOST_LIB_DIR =$(HOME)/Desktop/boost_1_54_0/stage/lib

CXXFLAGS  += `$(WX_CONFIG_BIN) --cxxflags      --debug=no --static=yes`
LINKFLAGS += `$(WX_CONFIG_BIN) --libs std, aui --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a -lX11
endif

endif
#################### OS X ############################
ifeq ($(OPERATING_SYSTEM_NAME), Darwin)
COMPILER_BIN=clang++ -stdlib=libc++
CXXFLAGS += -DZEN_MAC

WX_CONFIG_BIN =$(HOME)/Desktop/wxWidgets-2.9.4/lib/release/bin/wx-config
CXXFLAGS  += -I$(HOME)/Desktop/boost_1_54_0
BOOST_LIB_DIR =$(HOME)/Desktop/boost_1_54_0/stage/lib
MACOS_SDK     =-mmacosx-version-min=10.7 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk

#-Wl,-Bstatic not supported on OSX!

# link wxWidgets and boost statically -> check dependencies with: otool -L FreeFileSync
CXXFLAGS  += $(MACOS_SDK) `$(WX_CONFIG_BIN) --cxxflags      --debug=no --static=yes`
LINKFLAGS += $(MACOS_SDK) `$(WX_CONFIG_BIN) --libs std, aui --debug=no --static=yes` $(BOOST_LIB_DIR)/libboost_thread.a $(BOOST_LIB_DIR)/libboost_system.a

endif
#####################################################################################################

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
CPP_LIST+=lib/ffs_paths.cpp
CPP_LIST+=lib/xml_base.cpp
CPP_LIST+=zen/recycler.cpp
CPP_LIST+=zen/file_handling.cpp
CPP_LIST+=zen/file_id.cpp
CPP_LIST+=zen/file_io.cpp
CPP_LIST+=zen/file_traverser.cpp
CPP_LIST+=zen/zstring.cpp
CPP_LIST+=zen/format_unit.cpp
CPP_LIST+=zen/process_priority.cpp
CPP_LIST+=wx+/grid.cpp
CPP_LIST+=wx+/button.cpp
CPP_LIST+=wx+/graph.cpp
CPP_LIST+=wx+/tooltip.cpp
CPP_LIST+=wx+/zlib_wrap.cpp

# OS X
ifeq ($(OPERATING_SYSTEM_NAME), Darwin)
MM_LIST= #objective C files
MM_LIST+=ui/osx_dock.mm
MM_LIST+=lib/osx_file_icon.mm
endif

#list of all *.o files
OBJECT_LIST =  $(CPP_LIST:%.cpp=OBJ/FFS_GCC_Make_Release/%.o)
OBJECT_LIST += $(MM_LIST:%.mm=OBJ/FFS_GCC_Make_Release/%.mm.o)

all: FreeFileSync

OBJ/FFS_GCC_Make_Release/%.mm.o : %.mm
	mkdir -p $(dir $@)
	$(COMPILER_BIN) $(CXXFLAGS) -c $< -o $@

OBJ/FFS_GCC_Make_Release/%.o : %.cpp
	mkdir -p $(dir $@)
	$(COMPILER_BIN) $(CXXFLAGS) -c $< -o $@

FreeFileSync: $(OBJECT_LIST)
	$(COMPILER_BIN) -o ./BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf OBJ/FFS_GCC_Make_Release
	rm -f BUILD/$(APPNAME)
	rm -f wx+/pch.h.gch

install:
	mkdir -p $(BINDIR)
	cp BUILD/$(APPNAME) $(BINDIR)

	mkdir -p $(APPSHAREDIR)
	cp -R BUILD/Languages/ \
	BUILD/Help/ \
	BUILD/Sync_Complete.wav \
	BUILD/Resources.zip \
	BUILD/styles.gtk_rc \
	$(APPSHAREDIR)

	mkdir -p $(DOCSHAREDIR)
	cp BUILD/Changelog.txt $(DOCSHAREDIR)/changelog
	gzip $(DOCSHAREDIR)/changelog
