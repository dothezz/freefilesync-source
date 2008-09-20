set widgets=C:\Programme\CodeBlocks\wxWidgets
set sources=C:\Programme\CodeBlocks\Projects\FreeFileSync

md obj
md obj\Release
md obj\Release\library
md obj\Release\ui

mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\application.cpp -o obj\Release\Application.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\FreeFileSync.cpp -o obj\Release\FreeFileSync.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\library\globalFunctions.cpp -o obj\Release\library\globalFunctions.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\library\multithreading.cpp -o obj\Release\library\multithreading.cpp.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\ui\guiGenerated.cpp -o obj\Release\UI\GUI_Generated.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\ui\mainDialog.cpp -o obj\Release\ui\MainDialog.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\ui\syncDialog.cpp -o obj\Release\ui\SyncDialog.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\library\customGrid.cpp -o obj\Release\library\CustomGrid.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\ui\resources.cpp -o obj\Release\ui\Resources.o
mingw32-g++.exe -Wall -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DFFS_WIN -O2 -DNDEBUG -I%widgets%\include -I%widgets%\contrib\include -I%widgets%\lib\gcc_lib\msw -c %sources%\ui\smallDialogs.cpp -o obj\Release\ui\SmallDialogs.o
windres.exe -i %sources%\resource.rc -J rc -o obj\Release\resource.res -O coff -I%widgets%\include -I%widgets%\lib\gcc_lib\msw
mingw32-g++.exe -L%widgets%\lib\gcc_lib -o FreeFileSync.exe obj\Release\Application.o obj\Release\FreeFileSync.o obj\Release\library\globalFunctions.o obj\Release\library\multithreading.cpp.o obj\Release\ui\GUI_Generated.o obj\Release\ui\MainDialog.o obj\Release\ui\SyncDialog.o obj\Release\library\CustomGrid.o obj\Release\ui\Resources.o obj\Release\ui\SmallDialogs.o obj\Release\resource.res -s -mthreads -lwxmsw28_adv -lwxmsw28_core -lwxbase28 -lwxpng -lwxzlib -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32 -lwsock32 -lodbc32 -mwindows
pause