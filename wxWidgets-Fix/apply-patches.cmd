@if [%1]==[] echo Please pass wxWidgets installation directory as %%1 parameter, e.g.: C:\Programme\C++\wxWidgets  && pause && exit

::Segoe UI font with Vista
patch "%1\src\msw\settings.cpp" settings.cpp.patch
pause