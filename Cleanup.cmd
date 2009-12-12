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
del FreeFileSync.vcproj.*.user
del RealtimeSync\RealtimeSync.vcproj.*.user

del BUILD\FreeFileSync.pdb
del BUILD\FreeFileSync.ilk
del BUILD\RealtimeSync.pdb
del BUILD\RealtimeSync.ilk

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

::remove precompiled headers
del library\pch.h.gch
del RealtimeSync\pch.h.gch