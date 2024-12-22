# Explorer7 - ex7forw8, modernized

**NOTE:** Tihiy did not put a license on this so we're not releasing this source. This is for source control only.

## Registry keys

These keys are located under `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced`.

| Name | Type | Description |
| ---- | ---- | ----------- |
| Theme | REG_SZ | Name of the theme to use. This is relative to the directory. For example, "aero" will use the theme at "explorer7\theme\aero.msstyles". If this is not specified, aero will be used. |
| DisableComposition | REG_DWORD | When set to 1, Explorer7 will act as if the Desktop Window Manager is not running. |
| ClassicTheme | REG_DWORD | When set to 1, Explorer7 will use the Windows Classic theme. |

## Minhook Linker errors

If you're having linker errors because of the prebuilt minhook, do the following:

- In the root folder, open a cmd and grab the minhook repo: `git clone https://github.com/TsudaKageyu/minhook.git minhook`
- Once it's done, compile the solution in `minhook\build\VC17\MinHookVC17.sln`, specifying x64 Platform. You can do this using Visual Studio.
- Either copy it to the explorerwrapper project folder or just don't do anything and compile. A pre-build task will copy the new version over for you to use.

Contributors: DON'T COMMIT YOUR MODIFIED `libMinHook.x64.lib` UNLESS SPECIFIED!