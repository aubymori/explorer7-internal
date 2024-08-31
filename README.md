# Explorer7 - ex7forw8, modernized

**NOTE:** Tihiy did not put a license on this so we're not releasing this source. This is for source control only.

## Registry keys

These keys are located under `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced`.

| Name | Type | Description |
| ---- | ---- | ----------- |
| Theme | REG_SZ | Name of the theme to use. This is relative to the directory. For example, "aero" will use the theme at "explorer7\theme\aero.msstyles". If this is not specified, aero will be used. |
| DisableComposition | REG_DWORD | When set to 1, Explorer7 will act as if the Desktop Window Manager is not running. |
| ClassicTheme | REG_DWORD | When set to 1, Explorer7 will use the Windows Classic theme. |

## How 2 compile

In order for minhook to not complain about being in a different compiler version than the one you may have, you gotta compile it yourself. Luckily I (Erizur, who typed this small guide) made a small script to build minhook. **MAKE SURE TO CLONE THE SUBMODULES WHILE CLONING THE REPO!**

In Command Prompt (not PowerShell), go to the root folder and type `build_minhook.bat` to compile minhook, then build explorerwrapper.
