APPNAME = RealtimeSync

prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin

FFS_CPPFLAGS=-Wall -pipe -DNDEBUG -DwxUSE_UNICODE `wx-config --cxxflags --debug=no --unicode=yes` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -DZSTRING_CHAR -O3 -pthread -c -I../shared/boost_1_x
LINKFLAGS=`wx-config --libs --debug=no --unicode=yes` -O3 -pthread

FILE_LIST=              #internal list of all *.cpp files needed for compilation
FILE_LIST+=application.cpp
FILE_LIST+=functions.cpp
FILE_LIST+=guiGenerated.cpp
FILE_LIST+=mainDialog.cpp
FILE_LIST+=resources.cpp
FILE_LIST+=trayMenu.cpp
FILE_LIST+=watcher.cpp
FILE_LIST+=xmlProcessing.cpp
FILE_LIST+=xmlFreeFileSync.cpp
FILE_LIST+=../library/processXml.cpp
FILE_LIST+=../structures.cpp
FILE_LIST+=../shared/localization_no_BOM.cpp
FILE_LIST+=../shared/inotify/inotify-cxx.cpp
FILE_LIST+=../shared/tinyxml/tinyxml.cpp
FILE_LIST+=../shared/tinyxml/tinystr.cpp
FILE_LIST+=../shared/tinyxml/tinyxmlerror.cpp
FILE_LIST+=../shared/tinyxml/tinyxmlparser.cpp
FILE_LIST+=../shared/globalFunctions.cpp
FILE_LIST+=../shared/systemFunctions.cpp
FILE_LIST+=../shared/dragAndDrop.cpp
FILE_LIST+=../shared/zstring.cpp
FILE_LIST+=../shared/xmlBase.cpp
FILE_LIST+=../shared/customButton.cpp
FILE_LIST+=../shared/fileHandling.cpp
FILE_LIST+=../shared/fileTraverser.cpp
FILE_LIST+=../shared/standardPaths.cpp
FILE_LIST+=../shared/helpProvider.cpp
FILE_LIST+=../shared/fileIO.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), OBJ/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))


all: RealtimeSync

init: 
	if [ ! -d OBJ ]; then mkdir OBJ; fi

#remove byte ordering mark: needed by Visual C++ but an error with GCC
removeBOM: ../tools/removeBOM.cpp
	g++ -o OBJ/removeBOM ../tools/removeBOM.cpp 
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
