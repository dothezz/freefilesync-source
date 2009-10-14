CPPFLAGS=-Wall -pipe -DNDEBUG -DwxUSE_UNICODE `wx-config --cxxflags --debug=no --unicode=yes` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -DZSTRING_CHAR -O3 -pthread -c -I../shared/boost_1_40_0
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
FILE_LIST+=../shared/localization.cpp
FILE_LIST+=../shared/standardPaths.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), OBJ/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))


all: RealtimeSync

init: 
	if [ ! -d OBJ ]; then mkdir OBJ; fi

#remove byte ordering mark: needed by Visual C++ but an error with GCC
removeBOM: ../tools/removeBOM.cpp
	g++ -o removeBOM ../tools/removeBOM.cpp 
	./removeBOM ../shared/localization.cpp

%.dep : %.cpp 
 #strip path information
	g++ $(CPPFLAGS) $< -o OBJ/$(subst .cpp,.o,$(notdir $<))

RealtimeSync: init removeBOM $(DEP_LIST)
	g++ $(LINKFLAGS) -o ../BUILD/RealtimeSync $(OBJECT_LIST)

clean:
	find OBJ -type f -exec rm {} \;
