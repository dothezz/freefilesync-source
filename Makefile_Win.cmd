::-------------------------------------------------------
::set these variables to the appropriate directories
set widgets=C:\Programme\C++\wxWidgets
set widgetslib=C:\Programme\C++\wxWidgets\lib\gcc_lib\mswu
set sources=.
set mingw=C:\Programme\C++\MinGW\bin
::-------------------------------------------------------

set parameters=-Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O3 -DNDEBUG -DTIXML_USE_STL
path=%path%;%mingw%
if not exist obj md obj
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\structures.cpp -o obj\structures.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\application.cpp -o obj\application.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\algorithm.cpp -o obj\algorithm.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\comparison.cpp -o obj\comparison.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\synchronization.cpp -o obj\synchronization.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\globalFunctions.cpp -o obj\globalFunctions.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\multithreading.cpp -o obj\multithreading.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\statusHandler.cpp -o obj\statusHandler.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\fileHandling.cpp -o obj\fileHandling.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\guiGenerated.cpp -o obj\GUI_Generated.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\gridView.cpp -o obj\gridView.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\mainDialog.cpp -o obj\MainDialog.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\syncDialog.cpp -o obj\SyncDialog.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\checkVersion.cpp -o obj\checkVersion.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\batchStatusHandler.cpp -o obj\batchStatusHandler.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\guiStatusHandler.cpp -o obj\guiStatusHandler.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\customGrid.cpp -o obj\CustomGrid.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\resources.cpp -o obj\Resources.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\localization.cpp -o obj\localization.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\statistics.cpp -o obj\statistics.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\smallDialogs.cpp -o obj\SmallDialogs.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\dragAndDrop.cpp -o obj\dragAndDrop.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxml.cpp -o obj\tinyxml.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinystr.cpp -o obj\tinystr.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxmlerror.cpp -o obj\tinyxmlerror.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxmlparser.cpp -o obj\tinyxmlparser.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\processXml.cpp -o obj\processXml.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\zstring.cpp -o obj\zstring.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\customButton.cpp -o obj\customButton.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\filter.cpp -o obj\filter.o

windres.exe -i %sources%\resource.rc -J rc -o obj\resource.res -O coff -I%widgets%\include -I%widgetslib%
mingw32-g++.exe -L%widgets%\lib\gcc_lib -o FreeFileSync.exe obj\structures.o obj\application.o obj\algorithm.o obj\dragAndDrop.o obj\comparison.o obj\batchStatusHandler.o obj\statistics.o obj\guiStatusHandler.o obj\synchronization.o obj\globalFunctions.o obj\checkVersion.o obj\multithreading.o obj\statusHandler.o obj\fileHandling.o obj\customButton.o obj\filter.o obj\localization.o obj\GUI_Generated.o obj\gridView.o obj\MainDialog.o obj\SyncDialog.o obj\CustomGrid.o obj\Resources.o obj\SmallDialogs.o obj\resource.res obj\tinyxml.o obj\tinystr.o obj\tinyxmlerror.o obj\tinyxmlparser.o obj\processXml.o obj\zstring.o -s -mthreads -lwxmsw28u_adv -lwxmsw28u_core -lwxbase28u_net -lwxbase28u -lwxpng -lwxzlib -lkernel32 -lws2_32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32 -lwsock32 -lodbc32 -mwindows
pause