@if [%1]==[] echo Please pass wxWidgets installation directory as %%1 parameter, e.g.: C:\Programme\C++\wxWidgets  && pause && exit

::fix grid-label double-click to auto-size columns
patch %1\src\generic\grid.cpp grid.cpp.patch
patch %1\include\wx\generic\grid.h grid.h.patch

::fix translation of "Browse" text
patch %1\include\wx\filepicker.h filepicker.h.patch