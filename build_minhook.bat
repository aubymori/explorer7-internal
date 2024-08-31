cd .\minhook\build\VC17\
msbuild MinHookVC17.sln /p:Configuration=Release /p:Platform=x64
copy /Y .\lib\Release\libMinHook.x64.lib ..\..\..\explorerwrapper\
