::set these variables to the appropriate directories
set widgets=C:\Programme\C++\wxWidgets
set widgetslib=C:\Programme\C++\wxWidgets\lib\gcc_lib\mswu
set sources=.
set mingw=C:\Programme\C++\MinGW\bin

set parameters=-Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O3 -DNDEBUG -DTIXML_USE_STL
path=%path%;%mingw%
if not exist obj md obj
windres.exe -i %sources%\resource.rc -J rc -o obj\resource.res -O coff -I%widgets%\include -I%widgetslib%
mingw32-g++.exe -L%widgets%\lib\gcc_lib -o FreeFileSync.exe obj\application.o obj\algorithm.o obj\comparison.o obj\synchronization.o obj\globalFunctions.o obj\checkVersion.o obj\multithreading.o obj\statusHandler.o obj\fileHandling.o obj\customButton.o obj\misc.o obj\GUI_Generated.o obj\MainDialog.o obj\SyncDialog.o obj\CustomGrid.o obj\Resources.o obj\SmallDialogs.o obj\resource.res obj\tinyxml.o obj\tinystr.o obj\tinyxmlerror.o obj\tinyxmlparser.o obj\processXml.o obj\zstring.o -s -mthreads -lwxmsw28u_adv -lwxmsw28u_core -lwxbase28u_net -lwxbase28u -lwxpng -lwxzlib -lkernel32 -lws2_32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32 -lwsock32 -lodbc32 -mwindows
pause