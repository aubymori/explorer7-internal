<p align=center>
  <img src="https://github.com/user-attachments/assets/10465c11-481a-4403-aeef-19149a776f17">
</p>

explorer7 (formerly known as ex7forw8) is a **wrapper dll** that allows Windows 7's explorer.exe to run properly on modern Windows versions. This brings back the original Windows 7 Start Menu/Taskbar experience.

<details>
  <summary>Screenshots</summary>

![image](https://github.com/user-attachments/assets/58eedfb5-f6ff-435d-97fb-8696eb39b3ad)
![image](https://github.com/user-attachments/assets/eaaf450b-f368-4dbe-a6a0-92dcf538d697)

![image](https://github.com/user-attachments/assets/12d71c15-6d8c-4c33-ac80-54517352f160)
![image](https://github.com/user-attachments/assets/1a3a90e4-5359-497d-b451-4d97d5edcb7e)

</details>

## Registry keys

These keys are located under `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced`.

| Name | Type | Description | Default Value |
| ---- | ---- | ----------- | ------------- |
| Theme | REG_SZ | Name of the theme to use. This is relative to the directory. For example, `"aero"` will use the theme at `"explorer7\theme\aero.msstyles"`, `"Aero\aero"` will use the theme at `"explorer7\theme\Aero\aero.msstyles"`. If this is not specified, `aero` will be used. | **aero** |
| OrbImage | REG_SZ | Name of the orb to use. This is relative to the directory. For example, `"aero"` will use the theme at `"explorer7\orbs\aero.bmp"`, `"Aero\aero"` will use the theme at `"explorer7\orbs\Aero\aero.bmp"`. If this is not specified, `default` will be used. | **default** |
| DisableComposition | REG_DWORD | When set to 1, Explorer7 will act as if the Desktop Window Manager is not running. | **0** |
| ClassicTheme | REG_DWORD | When set to 1, Explorer7 will use the Windows Classic theme. | **0** |
| EnableImmersive | REG_DWORD | Controls the ability to run UWP apps in the system. When set to 0, UWP apps won't run. | **1** |
| EnableUWPAppsInStart | REG_DWORD | When set to 0, UWP apps will be hidden from the All Programs list. | **1** |

## Theme support

explorer7 allows any theme from Windows Vista - Windows 8.0 to be used for the start menu/taskbar. If applicable, you **must** include the "en-US" folder that comes along with your .msstyles file, otherwise the theme won't be applied. Windows 8.1+ themes do work, but do not have the proper classes for the Start Menu, and as of today, they cannot be restored.

<details>
  <summary>Here are valid file structures for the theme folder:</summary>

`Theme` registry key set to `theme1`
```
explorer7/
├─ theme/
│  ├─ en-US/
│  ├─ theme1.msstyles
```

`Theme` registry key set to `Themefolder\theme1`
```
explorer7/
├─ theme/
│  ├─ Themefolder/
│  │  ├─ en-US/
│  │  ├─ theme1.msstyles

```
  
</details>


## Manual Patch

If you wish to patch your explorer.exe to use the wrapper dll, you need something like [CFF Explorer](https://ntcore.com/files/CFF_Explorer.zip) to change out the imports for `SHLWAPI.DLL`, `OLE32.DLL` and (if applicable) `EXPLORERFRAME.DLL` to `WRP64.DLL` (wrapper dll). This is what the ex7forw8 installer does to the files you provide.

![image](https://github.com/user-attachments/assets/3122093d-8068-49c1-80a5-161468a65dfe)

## Development plan

We're working based on a milestone stage. Here's the planned stages of development:

|   Stage   | Goal | Status |
| -------- | --------- | ------ |
| Milestone 1 | Project start, stability on Windows 8.1 and a solid base for Windows 10 support. | ✅ Completed |
| Milestone 2 | Ironing out any last Windows 8.1-specific bugs, stability on Windows 10, UWP support and a solid base for Windows 11 | ⏳ Work in progress |
| Milestone 3 | Working out any last bugs on Windows 10, finishing up what's left for Windows 11, 1.0 | ⛔ Not in works |

While this project is aimed at restoring Windows 7 explorer.exe functionality, older explorers have been proven to work with the wrapper. In the future, we plan to support them directly.  Here's the chart
for support:

| Version | Status |
| ------- | ------ |
| Windows 7 | ⏳ Work in progress |
| Windows Vista | ❌ Not in works |
| Windows XP x64 | ❌ Not in works |

## Minhook Linker errors

If you're having linker errors because of the prebuilt minhook, do the following:

- In the root folder, open a cmd and grab the minhook repo: `git clone https://github.com/TsudaKageyu/minhook.git minhook`
- Once it's done, compile the solution in `minhook\build\VC17\MinHookVC17.sln`, specifying x64 Platform. You can do this using Visual Studio.
- Either copy it to the explorerwrapper project folder or just don't do anything and compile. A pre-build task will copy the new version over for you to use.

Contributors: DON'T COMMIT YOUR MODIFIED `libMinHook.x64.lib` UNLESS SPECIFIED!

## Note - Private Repo
Tihiy did not put a license on this so we're not releasing this source. This is for source control only.
