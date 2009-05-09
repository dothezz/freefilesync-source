CPPFLAGS=-Wall -pipe -DNDEBUG `wx-config --cppflags` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -O3 -pthread -c
ENDFLAGS=`wx-config --libs` -O3 -pthread

all: FreeFileSync

init:
	if [ ! -d obj ]; then mkdir obj; fi

structures.o: structures.cpp
	g++ $(CPPFLAGS) structures.cpp -o obj/structures.o

algorithm.o: algorithm.cpp
	g++ $(CPPFLAGS) algorithm.cpp -o obj/algorithm.o

comparison.o: comparison.cpp
	g++ $(CPPFLAGS) comparison.cpp -o obj/comparison.o

synchronization.o: synchronization.cpp
	g++ $(CPPFLAGS) synchronization.cpp -o obj/synchronization.o

application.o: application.cpp
	g++ $(CPPFLAGS) application.cpp -o obj/application.o

globalFunctions.o: library/globalFunctions.cpp
	g++ $(CPPFLAGS) library/globalFunctions.cpp -o obj/globalFunctions.o

guiGenerated.o: ui/guiGenerated.cpp
	g++ $(CPPFLAGS) ui/guiGenerated.cpp -o obj/guiGenerated.o

gridView.o: ui/gridView.cpp
	g++ $(CPPFLAGS) ui/gridView.cpp -o obj/gridView.o

mainDialog.o: ui/mainDialog.cpp
	g++ $(CPPFLAGS) ui/mainDialog.cpp -o obj/mainDialog.o

syncDialog.o: ui/syncDialog.cpp
	g++ $(CPPFLAGS) ui/syncDialog.cpp -o obj/syncDialog.o

checkVersion.o: ui/checkVersion.cpp
	g++ $(CPPFLAGS) ui/checkVersion.cpp -o obj/checkVersion.o

batchStatusHandler.o: ui/batchStatusHandler.cpp
	g++ $(CPPFLAGS) ui/batchStatusHandler.cpp -o obj/batchStatusHandler.o

guiStatusHandler.o: ui/guiStatusHandler.cpp
	g++ $(CPPFLAGS) ui/guiStatusHandler.cpp -o obj/guiStatusHandler.o

customGrid.o: library/customGrid.cpp
	g++ $(CPPFLAGS) library/customGrid.cpp -o obj/customGrid.o

fileHandling.o: library/fileHandling.cpp
	g++ $(CPPFLAGS) library/fileHandling.cpp -o obj/fileHandling.o

multithreading.o: library/multithreading.cpp
	g++ $(CPPFLAGS) library/multithreading.cpp -o obj/multithreading.o

statusHandler.o: library/statusHandler.cpp
	g++ $(CPPFLAGS) library/statusHandler.cpp -o obj/statusHandler.o

resources.o: library/resources.cpp
	g++ $(CPPFLAGS) library/resources.cpp -o obj/resources.o

smallDialogs.o: ui/smallDialogs.cpp
	g++ $(CPPFLAGS) ui/smallDialogs.cpp -o obj/smallDialogs.o

dragAndDrop.o: ui/dragAndDrop.cpp
	g++ $(CPPFLAGS) ui/dragAndDrop.cpp -o obj/dragAndDrop.o

localization.o: library/localization.cpp
	g++ $(CPPFLAGS) library/localization.cpp -o obj/localization.o

tinyxml.o: library/tinyxml/tinyxml.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxml.cpp -o obj/tinyxml.o

tinystr.o: library/tinyxml/tinystr.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinystr.cpp -o obj/tinystr.o

tinyxmlerror.o: library/tinyxml/tinyxmlerror.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlerror.cpp -o obj/tinyxmlerror.o

tinyxmlparser.o: library/tinyxml/tinyxmlparser.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlparser.cpp -o obj/tinyxmlparser.o

processXml.o: library/processXml.cpp
	g++ $(CPPFLAGS) library/processXml.cpp -o obj/processXml.o

statistics.o: library/statistics.cpp
	g++ $(CPPFLAGS) library/statistics.cpp -o obj/statistics.o

zstring.o: library/zstring.cpp
	g++ $(CPPFLAGS) library/zstring.cpp -o obj/zstring.o

customButton.o: library/customButton.cpp
	g++ $(CPPFLAGS) library/customButton.cpp -o obj/customButton.o

filter.o: library/filter.cpp
	g++ $(CPPFLAGS) library/filter.cpp -o obj/filter.o

FreeFileSync: init application.o structures.o algorithm.o comparison.o customButton.o filter.o checkVersion.o batchStatusHandler.o guiStatusHandler.o synchronization.o globalFunctions.o guiGenerated.o gridView.o mainDialog.o syncDialog.o customGrid.o fileHandling.o resources.o smallDialogs.o dragAndDrop.o multithreading.o statusHandler.o localization.o tinyxml.o tinystr.o tinyxmlerror.o tinyxmlparser.o processXml.o statistics.o zstring.o
	g++ $(ENDFLAGS) -o FreeFileSync obj/application.o obj/structures.o obj/algorithm.o obj/comparison.o obj/customButton.o obj/filter.o obj/batchStatusHandler.o obj/guiStatusHandler.o obj/checkVersion.o obj/synchronization.o obj/globalFunctions.o obj/guiGenerated.o obj/gridView.o obj/mainDialog.o obj/syncDialog.o obj/customGrid.o obj/fileHandling.o obj/resources.o obj/smallDialogs.o obj/dragAndDrop.o obj/multithreading.o obj/statusHandler.o obj/localization.o obj/tinyxml.o obj/tinystr.o obj/tinyxmlerror.o obj/tinyxmlparser.o obj/processXml.o obj/statistics.o obj/zstring.o

clean:
	find obj -type f -exec rm {} \;
