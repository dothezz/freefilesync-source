::set these variables to the appropriate directories
set widgets=C:\Programme\CodeBlocks\wxWidgets
set widgetslib=C:\Programme\CodeBlocks\wxWidgets\lib\gcc_lib\mswu
set sources=C:\Programme\CodeBlocks\Projects\FreeFileSync
set mingw=C:\Programme\CodeBlocks\MinGW\bin

set parameters=-Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -DTIXML_USE_STL
path=%path%;%mingw%
if not exist obj md obj
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\application.cpp -o obj\Application.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\FreeFileSync.cpp -o obj\FreeFileSync.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\globalFunctions.cpp -o obj\globalFunctions.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\multithreading.cpp -o obj\multithreading.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\guiGenerated.cpp -o obj\GUI_Generated.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\mainDialog.cpp -o obj\MainDialog.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\syncDialog.cpp -o obj\SyncDialog.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\customGrid.cpp -o obj\CustomGrid.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\resources.cpp -o obj\Resources.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\misc.cpp -o obj\misc.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\smallDialogs.cpp -o obj\SmallDialogs.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxml.cpp -o obj\tinyxml.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinystr.cpp -o obj\tinystr.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxmlerror.cpp -o obj\tinyxmlerror.o
mingw32-g++.exe %parameters% -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\tinyxml\tinyxmlparser.cpp -o obj\tinyxmlparser.o

windres.exe -i %sources%\resource.rc -J rc -o obj\resource.res -O coff -I%widgets%\include -I%widgetslib%
mingw32-g++.exe -L%widgets%\lib\gcc_lib -o FreeFileSync.exe obj\Application.o obj\FreeFileSync.o obj\globalFunctions.o obj\multithreading.o obj\misc.o obj\GUI_Generated.o obj\MainDialog.o obj\SyncDialog.o obj\CustomGrid.o obj\Resources.o obj\SmallDialogs.o obj\resource.res obj\tinyxml.o obj\tinystr.o obj\tinyxmlerror.o obj\tinyxmlparser.o -s -mthreads -lwxmsw28u_adv -lwxmsw28u_core -lwxbase28u -lwxpng -lwxzlib -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32 -lwsock32 -lodbc32 -mwindows
pause