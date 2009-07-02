CPPFLAGS=-Wall -pipe -DNDEBUG `wx-config --cppflags` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -DZSTRING_CHAR -O3 -pthread -c
ENDFLAGS=`wx-config --libs` -O3 -pthread

all: FreeFileSync

init:
	if [ ! -d OBJ ]; then mkdir OBJ; fi

structures.o: structures.cpp
	g++ $(CPPFLAGS) structures.cpp -o OBJ/structures.o

algorithm.o: algorithm.cpp
	g++ $(CPPFLAGS) algorithm.cpp -o OBJ/algorithm.o

comparison.o: comparison.cpp
	g++ $(CPPFLAGS) comparison.cpp -o OBJ/comparison.o

synchronization.o: synchronization.cpp
	g++ $(CPPFLAGS) synchronization.cpp -o OBJ/synchronization.o

application.o: application.cpp
	g++ $(CPPFLAGS) application.cpp -o OBJ/application.o

globalFunctions.o: library/globalFunctions.cpp
	g++ $(CPPFLAGS) library/globalFunctions.cpp -o OBJ/globalFunctions.o

guiGenerated.o: ui/guiGenerated.cpp
	g++ $(CPPFLAGS) ui/guiGenerated.cpp -o OBJ/guiGenerated.o

gridView.o: ui/gridView.cpp
	g++ $(CPPFLAGS) ui/gridView.cpp -o OBJ/gridView.o

mainDialog.o: ui/mainDialog.cpp
	g++ $(CPPFLAGS) ui/mainDialog.cpp -o OBJ/mainDialog.o

syncDialog.o: ui/syncDialog.cpp
	g++ $(CPPFLAGS) ui/syncDialog.cpp -o OBJ/syncDialog.o

checkVersion.o: ui/checkVersion.cpp
	g++ $(CPPFLAGS) ui/checkVersion.cpp -o OBJ/checkVersion.o

batchStatusHandler.o: ui/batchStatusHandler.cpp
	g++ $(CPPFLAGS) ui/batchStatusHandler.cpp -o OBJ/batchStatusHandler.o

guiStatusHandler.o: ui/guiStatusHandler.cpp
	g++ $(CPPFLAGS) ui/guiStatusHandler.cpp -o OBJ/guiStatusHandler.o

customGrid.o: library/customGrid.cpp
	g++ $(CPPFLAGS) library/customGrid.cpp -o OBJ/customGrid.o

fileHandling.o: library/fileHandling.cpp
	g++ $(CPPFLAGS) library/fileHandling.cpp -o OBJ/fileHandling.o

statusHandler.o: library/statusHandler.cpp
	g++ $(CPPFLAGS) library/statusHandler.cpp -o OBJ/statusHandler.o

resources.o: library/resources.cpp
	g++ $(CPPFLAGS) library/resources.cpp -o OBJ/resources.o

smallDialogs.o: ui/smallDialogs.cpp
	g++ $(CPPFLAGS) ui/smallDialogs.cpp -o OBJ/smallDialogs.o

dragAndDrop.o: ui/dragAndDrop.cpp
	g++ $(CPPFLAGS) ui/dragAndDrop.cpp -o OBJ/dragAndDrop.o

removeBOM: tools/removeBOM.cpp
	g++ -o removeBOM tools/removeBOM.cpp
	
localization.o: library/localization.cpp removeBOM
	./removeBOM library/localization.cpp
	g++ $(CPPFLAGS) library/localization.cpp -o OBJ/localization.o

tinyxml.o: library/tinyxml/tinyxml.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxml.cpp -o OBJ/tinyxml.o

tinystr.o: library/tinyxml/tinystr.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinystr.cpp -o OBJ/tinystr.o

tinyxmlerror.o: library/tinyxml/tinyxmlerror.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlerror.cpp -o OBJ/tinyxmlerror.o

tinyxmlparser.o: library/tinyxml/tinyxmlparser.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlparser.cpp -o OBJ/tinyxmlparser.o

processXml.o: library/processXml.cpp
	g++ $(CPPFLAGS) library/processXml.cpp -o OBJ/processXml.o

statistics.o: library/statistics.cpp
	g++ $(CPPFLAGS) library/statistics.cpp -o OBJ/statistics.o

zstring.o: library/zstring.cpp
	g++ $(CPPFLAGS) library/zstring.cpp -o OBJ/zstring.o

customButton.o: library/customButton.cpp
	g++ $(CPPFLAGS) library/customButton.cpp -o OBJ/customButton.o

filter.o: library/filter.cpp
	g++ $(CPPFLAGS) library/filter.cpp -o OBJ/filter.o

FreeFileSync: init application.o structures.o algorithm.o comparison.o customButton.o filter.o checkVersion.o batchStatusHandler.o guiStatusHandler.o synchronization.o globalFunctions.o guiGenerated.o gridView.o mainDialog.o syncDialog.o customGrid.o fileHandling.o resources.o smallDialogs.o dragAndDrop.o statusHandler.o localization.o tinyxml.o tinystr.o tinyxmlerror.o tinyxmlparser.o processXml.o statistics.o zstring.o
	g++ $(ENDFLAGS) -o BUILD/FreeFileSync OBJ/application.o OBJ/structures.o OBJ/algorithm.o OBJ/comparison.o OBJ/customButton.o OBJ/filter.o OBJ/batchStatusHandler.o OBJ/guiStatusHandler.o OBJ/checkVersion.o OBJ/synchronization.o OBJ/globalFunctions.o OBJ/guiGenerated.o OBJ/gridView.o OBJ/mainDialog.o OBJ/syncDialog.o OBJ/customGrid.o OBJ/fileHandling.o OBJ/resources.o OBJ/smallDialogs.o OBJ/dragAndDrop.o OBJ/statusHandler.o OBJ/localization.o OBJ/tinyxml.o OBJ/tinystr.o OBJ/tinyxmlerror.o OBJ/tinyxmlparser.o OBJ/processXml.o OBJ/statistics.o OBJ/zstring.o

clean:
	find obj -type f -exec rm {} \;
