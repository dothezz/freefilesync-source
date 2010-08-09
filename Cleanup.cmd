@if NOT [%1]==[] echo Don't pass a parameter! && pause && exit

@echo off

::clean codeblocks garbage
del FreeFileSync.layout
del FreeFileSync.depend
del RealtimeSync\RealtimeSync.layout
del RealtimeSync\RealtimeSync.depend

::clean Visual C++ garbage
del FreeFileSync.ncb
del RealtimeSync\RealtimeSync.ncb
attrib FreeFileSync.suo -h
del FreeFileSync.suo
attrib RealtimeSync\RealtimeSync.suo -h
del RealtimeSync\RealtimeSync.suo
del FreeFileSync.sdf
del RealtimeSync\RealtimeSync.sdf

del BUILD\FreeFileSync*.pdb
del BUILD\FreeFileSync*.ilk
del BUILD\FreeFileSync*.lib
del BUILD\FreeFileSync*.exp

del BUILD\RealtimeSync*.pdb
del BUILD\RealtimeSync*.ilk
del BUILD\RealtimeSync*.lib
del BUILD\RealtimeSync*.exp

del BUILD\FreeFileSync.exe
del BUILD\FreeFileSync_Debug.exe
del BUILD\FreeFileSync_Win32.exe
del BUILD\FreeFileSync_x64.exe
del BUILD\RealtimeSync.exe
del BUILD\RealtimeSync_Debug.exe
del BUILD\RealtimeSync_Win32.exe
del BUILD\RealtimeSync_x64.exe
del BUILD\gmon.out

del shared\ShadowCopy\ShadowCopy.ncb
attrib shared\ShadowCopy\ShadowCopy.suo -h
del shared\ShadowCopy\ShadowCopy.suo
del shared\ShadowCopy\Shadow_2003.vcproj.*.user
del shared\ShadowCopy\Shadow_XP.vcproj.*.user
del shared\ShadowCopy\ShadowTest.vcproj.*.user
del shared\ShadowCopy\Shadow.pdb
del shared\ShadowCopy\Shadow.ilk
del shared\ShadowCopy\Shadow.exp
del shared\ShadowCopy\Shadow.lib
del shared\ShadowCopy\ShadowTest.ilk
del shared\ShadowCopy\ShadowTest.pdb

del shared\IFileOperation\FileOperation_Vista.ncb
attrib shared\IFileOperation\FileOperation_Vista.suo -h
del shared\IFileOperation\FileOperation_Vista.suo
del shared\IFileOperation\FileOperation_Vista.vcproj.*.user
del shared\IFileOperation\Test.vcproj.*.user
del shared\IFileOperation\Test.ilk

attrib shared\Taskbar_Seven\Taskbar_Seven.suo -h
del shared\Taskbar_Seven\Taskbar_Seven.suo
del shared\Taskbar_Seven\Taskbar_Seven.vcproj.*.user
