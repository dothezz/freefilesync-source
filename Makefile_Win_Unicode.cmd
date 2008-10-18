set widgets=C:\Programme\CodeBlocks\wxWidgets
set widgetslib=C:\Programme\CodeBlocks\wxWidgets\lib\gcc_lib\mswu
set sources=C:\Programme\CodeBlocks\Projects\FreeFileSync

md obj

mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\application.cpp -o obj\Application.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\FreeFileSync.cpp -o obj\FreeFileSync.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\globalFunctions.cpp -o obj\globalFunctions.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\multithreading.cpp -o obj\multithreading.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\guiGenerated.cpp -o obj\GUI_Generated.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\mainDialog.cpp -o obj\MainDialog.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\syncDialog.cpp -o obj\SyncDialog.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\customGrid.cpp -o obj\CustomGrid.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\resources.cpp -o obj\Resources.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\library\misc.cpp -o obj\misc.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -DwxUSE_UNICODE -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgetslib% -c %sources%\ui\smallDialogs.cpp -o obj\SmallDialogs.o
windres.exe -i %sources%\resource.rc -J rc -o obj\resource.res -O coff -I%widgets%\include -I%widgetslib%
mingw32-g++.exe -L%widgets%\lib\gcc_lib -o FreeFileSync.exe obj\Application.o obj\FreeFileSync.o obj\globalFunctions.o obj\multithreading.o obj\misc.o obj\GUI_Generated.o obj\MainDialog.o obj\SyncDialog.o obj\CustomGrid.o obj\Resources.o obj\SmallDialogs.o obj\resource.res -s -mthreads -lwxmsw28u_adv -lwxmsw28u_core -lwxbase28u -lwxpng -lwxzlib -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32 -lwsock32 -lodbc32 -mwindows

pause