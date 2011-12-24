@if [%1]==[] echo Please pass wxWidgets installation directory as %%1 parameter, e.g.: C:\Programme\C++\wxWidgets  && pause && exit

::fix grid-label double-click to auto-size columns
patch "%1\src\generic\grid.cpp" grid.cpp.patch
pause
patch "%1\include\wx\generic\grid.h" grid.h.patch
pause
::Segoe UI font with Vista
patch "%1\src\msw\settings.cpp" settings.cpp.patch
echo Make sure to add "-luxtheme" to "Standard linker flags" in \wxWidgets\build\msw\config.gcc!!!
pause