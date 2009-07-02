@if NOT [%1]==[] echo Don't pass a parameter! && pause && exit

@echo off

::clean codeblocks garbage
del FreeFileSync.layout
del FreeFileSync.depend

::clean Visual C++ garbage
del FreeFileSync.ncb
attrib FreeFileSync.suo -h
del FreeFileSync.suo
del FreeFileSync.vcproj.*.user
del FreeFileSync.sln
del BUILD\FreeFileSync.pdb
del BUILD\FreeFileSync.ilk

del library\ShadowCopy\ShadowCopy.ncb
attrib library\ShadowCopy\ShadowCopy.suo -h
del library\ShadowCopy\ShadowCopy.suo
del library\ShadowCopy\ShadowDll.vcproj.*.user
del library\ShadowCopy\ShadowTest.vcproj.*.user
del library\ShadowCopy\Shadow.pdb
del library\ShadowCopy\Shadow.ilk
del library\ShadowCopy\Shadow.exp
del library\ShadowCopy\Shadow.lib
del library\ShadowCopy\ShadowTest.ilk
del library\ShadowCopy\ShadowTest.pdb

