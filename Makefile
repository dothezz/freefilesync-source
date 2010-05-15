APPNAME = FreeFileSync

prefix = /usr
BINDIR = $(DESTDIR)$(prefix)/bin
SHAREDIR = $(DESTDIR)$(prefix)/share
APPSHAREDIR = $(SHAREDIR)/$(APPNAME)

FFS_CPPFLAGS=-Wall -pipe -DNDEBUG -DwxUSE_UNICODE `wx-config --cxxflags --debug=no --unicode=yes` `pkg-config --cflags gtk+-2.0` -DFFS_LINUX -DTIXML_USE_STL -DZSTRING_CHAR -O3 -pthread -c -Ishared/boost_1_x
LINKFLAGS=`wx-config --libs --debug=no --unicode=yes` shared/ossp_uuid/.libs/libuuid++.a -O3 -pthread

FILE_LIST=              #internal list of all *.cpp files needed for compilation
FILE_LIST+=structures.cpp
FILE_LIST+=algorithm.cpp
FILE_LIST+=comparison.cpp
FILE_LIST+=synchronization.cpp
FILE_LIST+=fileHierarchy.cpp
FILE_LIST+=application.cpp
FILE_LIST+=ui/guiGenerated.cpp
FILE_LIST+=shared/util.cpp
FILE_LIST+=ui/gridView.cpp
FILE_LIST+=ui/mainDialog.cpp
FILE_LIST+=ui/batchConfig.cpp
FILE_LIST+=ui/syncConfig.cpp
FILE_LIST+=ui/checkVersion.cpp
FILE_LIST+=ui/batchStatusHandler.cpp
FILE_LIST+=ui/guiStatusHandler.cpp
FILE_LIST+=ui/trayIcon.cpp
FILE_LIST+=ui/search.cpp
FILE_LIST+=ui/switchToGui.cpp
FILE_LIST+=ui/messagePopup.cpp
FILE_LIST+=ui/progressIndicator.cpp
FILE_LIST+=library/customGrid.cpp
FILE_LIST+=library/errorLogging.cpp
FILE_LIST+=library/statusHandler.cpp
FILE_LIST+=library/resources.cpp
FILE_LIST+=ui/smallDialogs.cpp
FILE_LIST+=library/processXml.cpp
FILE_LIST+=library/statistics.cpp
FILE_LIST+=library/filter.cpp
FILE_LIST+=library/binary.cpp
FILE_LIST+=library/dbFile.cpp
FILE_LIST+=shared/localization_no_BOM.cpp
FILE_LIST+=shared/fileIO.cpp
FILE_LIST+=shared/dragAndDrop.cpp
FILE_LIST+=shared/guid.cpp
FILE_LIST+=shared/tinyxml/tinyxml.cpp
FILE_LIST+=shared/tinyxml/tinystr.cpp
FILE_LIST+=shared/tinyxml/tinyxmlerror.cpp
FILE_LIST+=shared/tinyxml/tinyxmlparser.cpp
FILE_LIST+=shared/globalFunctions.cpp
FILE_LIST+=shared/systemFunctions.cpp
FILE_LIST+=shared/customTooltip.cpp
FILE_LIST+=shared/fileHandling.cpp
FILE_LIST+=shared/fileTraverser.cpp
FILE_LIST+=shared/standardPaths.cpp
FILE_LIST+=shared/zstring.cpp
FILE_LIST+=shared/xmlBase.cpp
FILE_LIST+=shared/appMain.cpp
FILE_LIST+=shared/customButton.cpp
FILE_LIST+=shared/toggleButton.cpp
FILE_LIST+=shared/customComboBox.cpp
FILE_LIST+=shared/serialize.cpp
FILE_LIST+=shared/fileID.cpp
FILE_LIST+=shared/recycler.cpp
FILE_LIST+=shared/helpProvider.cpp

#list of all *.o files
OBJECT_LIST=$(foreach file, $(FILE_LIST), OBJ/$(subst .cpp,.o,$(notdir $(file))))

#build list of all dependencies
DEP_LIST=$(foreach file, $(FILE_LIST), $(subst .cpp,.dep,$(file)))

#support for Glib-IO/GIO recycler
#Recycle bin: check whether GLIB library is existing (and add relevant compiler and linker flags)
GIO_EXISTING=$(shell pkg-config --exists gio-2.0 && echo YES)
ifeq ($(GIO_EXISTING),YES)
FFS_CPPFLAGS+=-DRECYCLER_GIO `pkg-config --cflags gio-2.0`
LINKFLAGS+=`pkg-config --libs gio-2.0`
else
FFS_CPPFLAGS+=-DRECYCLER_NONE
$(warning )
$(warning -----------------------------------------------------------------------------------------)
$(warning | Warning: No gio-2.0 package found: Recycle Bin will NOT be available for this system! |)
$(warning -----------------------------------------------------------------------------------------)
$(warning )
endif


all: FreeFileSync

init: 
	if [ ! -d OBJ ]; then mkdir OBJ; fi

#remove byte ordering mark: needed by Visual C++ but an error with GCC
removeBOM: tools/removeBOM.cpp
	g++ -o OBJ/removeBOM tools/removeBOM.cpp 
	./OBJ/removeBOM shared/localization.cpp shared/localization_no_BOM.cpp

osspUUID:
#some files within ossp_uuid may need to have readonly attribute removed
	chmod -R 0755 shared/ossp_uuid && \
	cd shared/ossp_uuid && \
	chmod +x configure && \
	chmod +x shtool && \
	./configure --with-cxx --disable-shared && \
	make && \
	make check

%.dep : %.cpp 
#strip path information
	g++ $(FFS_CPPFLAGS) $< -o OBJ/$(subst .cpp,.o,$(notdir $<))

FreeFileSync: init removeBOM osspUUID $(DEP_LIST)
	g++ -o BUILD/$(APPNAME) $(OBJECT_LIST) $(LINKFLAGS)

clean:
	rm -rf OBJ
	rm -f BUILD/$(APPNAME)
	if [ -e shared/ossp_uuid/Makefile ]; then cd shared/ossp_uuid && make clean; fi
	rm -f shared/localization_no_BOM.cpp

install:
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR); fi
	if [ ! -d $(APPSHAREDIR) ] ; then mkdir -p $(APPSHAREDIR); fi

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
