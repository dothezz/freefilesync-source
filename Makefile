CPPFLAGS=-Wall -pipe -DNDEBUG `wx-config --cppflags` -DFFS_LINUX -DTIXML_USE_STL -O3 -pthread -c
ENDFLAGS=`wx-config --libs` -lwx_gtk2_aui-2.8 -O3 -pthread

all: FreeFileSync

obj/FreeFileSync.o: FreeFileSync.cpp
	g++ $(CPPFLAGS) FreeFileSync.cpp -o obj/FreeFileSync.o

obj/application.o: application.cpp
	g++ $(CPPFLAGS) application.cpp -o obj/application.o

obj/globalFunctions.o: library/globalFunctions.cpp
	g++ $(CPPFLAGS) library/globalFunctions.cpp -o obj/globalFunctions.o

obj/guiGenerated.o: ui/guiGenerated.cpp
	g++ $(CPPFLAGS) ui/guiGenerated.cpp -o obj/guiGenerated.o

obj/mainDialog.o: ui/mainDialog.cpp
	g++ $(CPPFLAGS) ui/mainDialog.cpp -o obj/mainDialog.o

obj/syncDialog.o: ui/syncDialog.cpp
	g++ $(CPPFLAGS) ui/syncDialog.cpp -o obj/syncDialog.o

obj/customGrid.o: library/customGrid.cpp
	g++ $(CPPFLAGS) library/customGrid.cpp -o obj/customGrid.o

obj/multithreading.o: library/multithreading.cpp
	g++ $(CPPFLAGS) library/multithreading.cpp -o obj/multithreading.o

obj/resources.o: library/resources.cpp
	g++ $(CPPFLAGS) library/resources.cpp -o obj/resources.o

obj/smallDialogs.o: ui/smallDialogs.cpp
	g++ $(CPPFLAGS) ui/smallDialogs.cpp -o obj/smallDialogs.o

obj/misc.o: library/misc.cpp
	g++ $(CPPFLAGS) library/misc.cpp -o obj/misc.o

obj/tinyxml.o: library/tinyxml/tinyxml.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxml.cpp -o obj/tinyxml.o

obj/tinystr.o: library/tinyxml/tinystr.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinystr.cpp -o obj/tinystr.o

obj/tinyxmlerror.o: library/tinyxml/tinyxmlerror.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlerror.cpp -o obj/tinyxmlerror.o

obj/tinyxmlparser.o: library/tinyxml/tinyxmlparser.cpp
	g++ $(CPPFLAGS) library/tinyxml/tinyxmlparser.cpp -o obj/tinyxmlparser.o

obj/processXml.o: library/processXml.cpp
	g++ $(CPPFLAGS) library/processXml.cpp -o obj/processXml.o
	
FreeFileSync: obj/FreeFileSync.o obj/application.o obj/globalFunctions.o obj/guiGenerated.o obj/mainDialog.o obj/syncDialog.o obj/customGrid.o obj/resources.o obj/smallDialogs.o obj/multithreading.o obj/misc.o obj/tinyxml.o obj/tinystr.o obj/tinyxmlerror.o obj/tinyxmlparser.o obj/processXml.o
	g++ $(ENDFLAGS) -o FreeFileSync obj/application.o obj/FreeFileSync.o obj/globalFunctions.o obj/guiGenerated.o obj/mainDialog.o obj/syncDialog.o obj/customGrid.o obj/resources.o obj/smallDialogs.o obj/multithreading.o obj/misc.o obj/tinyxml.o obj/tinystr.o obj/tinyxmlerror.o obj/tinyxmlparser.o obj/processXml.o


clean:
	find obj -type f -exec rm {} \;
