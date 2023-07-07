@echo off

for /r %%i in (..\Bin\Debug, ..\Bin\SDebug, .vs, temp, obj) do rd %%i /s/q

for /r ..\Bin\Release %%i in (*.exp, *.ilk, *.lib, *.pdb, *.iobj, *.ipdb) do del %%i /f/s/q

for /r ..\Bin\SRelease %%i in (*.exp, *.ilk, *.lib, *.pdb, *.iobj, *.ipdb) do del %%i /f/s/q
