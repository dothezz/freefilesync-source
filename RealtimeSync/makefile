APPNAME = RealtimeSync

prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

FFS_CPPFLAGS=-Wall -pipe -DNDEBUG -DwxUSE_UNICODE `wx-config --cxxflags --debug=no --unicode=yes` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -DZSTRING_CHAR -O3 -pthread -c
LINKFLAGS=`wx-config --libs --debug=no --unicode=yes` -lboost_thread -O3 -pthread

FILE_LIST=              #internal list of all *.cpp files needed for compilation
FILE_LIST+=application.cpp
FILE_LIST+=functions.cpp
FILE_LIST+=gui_generated.cpp
FILE_LIST+=main_dlg.cpp
FILE_LIST+=resources.cpp
FILE_LIST+=tray_menu.cpp
FILE_LIST+=watcher.cpp
FILE_LIST+=xml_proc.cpp
FILE_LIST+=xml_ffs.cpp
FILE_LIST+=../library/process_xml.cpp
FILE_LIST+=../structures.cpp
FILE_LIST+=../shared/check_exist.cpp
FILE_LIST+=../shared/localization_no_BOM.cpp
FILE_LIST+=../shared/inotify/inotify-cxx.cpp
FILE_LIST+=../shared/tinyxml/tinyxml.cpp
FILE_LIST+=../shared/tinyxml/tinyxmlerror.cpp
FILE_LIST+=../shared/tinyxml/tinyxmlparser.cpp
FILE_LIST+=../shared/global_func.cpp
FILE_LIST+=../shared/system_func.cpp
FILE_LIST+=../shared/drag_n_drop.cpp
FILE_LIST+=../shared/zstring.cpp
FILE_LIST+=../shared/xml_base.cpp
FILE_LIST+=../shared/custom_button.cpp
FILE_LIST+=../shared/file_handling.cpp
FILE_LIST+=../shared/file_traverser.cpp
FILE_LIST+=../shared/standard_paths.cpp
FILE_LIST+=../shared/help_provider.cpp
FILE_LIST+=../shared/file_io.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), OBJ/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))


all: RealtimeSync

init: 
	if [ ! -d OBJ ]; then mkdir OBJ; fi

#remove byte ordering mark: needed by Visual C++ but an error with GCC
removeBOM: ../tools/remove_BOM.cpp
	g++ -o OBJ/removeBOM ../tools/remove_BOM.cpp 
	./OBJ/removeBOM ../shared/localization.cpp ../shared/localization_no_BOM.cpp

%.dep : %.cpp 
 #strip path information
	g++ $(FFS_CPPFLAGS) $< -o OBJ/$(subst .cpp,.o,$(notdir $<))

RealtimeSync: init removeBOM $(DEP_LIST)
	g++ $(LINKFLAGS) -o ../BUILD/$(APPNAME) $(OBJECT_LIST)

clean:
	rm -rf OBJ
	rm -f ../BUILD/$(APPNAME)
	rm -f ../shared/localization_no_BOM.cpp

install:
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR); fi

	cp ../BUILD/$(APPNAME) $(BINDIR)
