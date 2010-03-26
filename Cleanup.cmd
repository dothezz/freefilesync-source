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

del BUILD\FreeFileSync*.pdb
del BUILD\FreeFileSync*.ilk
del BUILD\FreeFileSync*.lib
del BUILD\FreeFileSync*.exp

del BUILD\RealtimeSync*.pdb
del BUILD\RealtimeSync*.ilk
del BUILD\RealtimeSync*.lib
del BUILD\RealtimeSync*.exp

del BUILD\FreeFileSync.exe
del BUILD\FreeFileSync_Win32.exe
del BUILD\FreeFileSync_x64.exe
del BUILD\RealtimeSync.exe
del BUILD\RealtimeSync_Win32.exe
del BUILD\RealtimeSync_x64.exe
del BUILD\gmon.out

del library\ShadowCopy\ShadowCopy.ncb
attrib library\ShadowCopy\ShadowCopy.suo -h
del library\ShadowCopy\ShadowCopy.suo
del library\ShadowCopy\Shadow_2003.vcproj.*.user
del library\ShadowCopy\Shadow_XP.vcproj.*.user
del library\ShadowCopy\ShadowTest.vcproj.*.user
del library\ShadowCopy\Shadow.pdb
del library\ShadowCopy\Shadow.ilk
del library\ShadowCopy\Shadow.exp
del library\ShadowCopy\Shadow.lib
del library\ShadowCopy\ShadowTest.ilk
del library\ShadowCopy\ShadowTest.pdb

del library\Recycler\Recycler_Vista.ncb
attrib library\Recycler\Recycler_Vista.suo -h
del library\Recycler\Recycler_Vista.suo
del library\Recycler\Recycler_Vista.vcproj.*.user
del library\Recycler\Test.vcproj.*.user

attrib library\Taskbar_Seven\Taskbar_Seven.suo -h
del library\Taskbar_Seven\Taskbar_Seven.suo
del library\Taskbar_Seven\Taskbar_Seven.vcproj.*.user
