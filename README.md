<p align=center>
  <img src="https://github.com/user-attachments/assets/10465c11-481a-4403-aeef-19149a776f17">
</p>

explorer7 is a **wrapper dll** that allows Windows 7's explorer.exe to run properly on modern Windows versions. This brings back the original Windows 7 Start Menu/Taskbar experience.

<details>
  <summary>Screenshots</summary>

![image](https://github.com/user-attachments/assets/58eedfb5-f6ff-435d-97fb-8696eb39b3ad)
![image](https://github.com/user-attachments/assets/eaaf450b-f368-4dbe-a6a0-92dcf538d697)

![image](https://github.com/user-attachments/assets/12d71c15-6d8c-4c33-ac80-54517352f160)
![image](https://github.com/user-attachments/assets/1a3a90e4-5359-497d-b451-4d97d5edcb7e)

</details>

## Known issues (Milestone 1)

**MAKE SURE YOU READ THESE FIRST TO KNOW WHAT YOU'RE GETTING INTO!**

**Windows 8.1**
- No proper strings for "Customize Start Menu" dialog.

**Windows 10**
- Autoplay does not work.
- Metro "Open With" dialog opens on top left of screen (works with UWP enabled)
- System msstyles with name "aero.msstyles" messes with the Start Menu colorization.
- Wallpaper stops working/Desktop becomes buggy when plugging another monitor in/changing multimonitor configuration while explorer is running.
- Desktop area sticks to a specific resolution and requires going into the Wallpaper control panel page to fix (i.e going from 1920x1080 to 1024x768, the desktop area/wallpaper would be rendered as 1080p still, vice versa).
- Notification Area Icon settings in Control Panel are missing.
- Unless icon is blanked, the badge for compression (in case enabled) will appear on taskbar/start menu items.

**Windows 11**
- Taskbar/Start menu pins broken due to confirmation dialog introduction (fixed in Windows 10 22h2).
- Yet to be documented!

**Windows 7 limitations/bugs**

None of the following will be accounted for in explorer7:

- Multi-monitor taskbars are not supported. These would be introduced by Windows 8 build 7779.
- Startup items defined in modern Task Manager are not accounted for. You must use old msconfig.exe.
- The taskbar does not remember its size/position until a while after.

## Installation Guide

For most users, you'll want the **regular installation method**:

<details>
  <summary>Standard installation</summary>
  
**Pre-Requirements**
1. explorer7 package from releases
2. Valid Windows 7 x64 installation medium, in the same language as your system

**How-to**
1. Mount Windows 7 install media by double clicking on it
2. Have the explorer7 package extracted somewhere handy. A good folder would be `X:\Program Files\explorer7`
3. Run ex7forw8.exe. The installer will ask for Windows 7 files. You can select any of the 2 options provided the installation media is mounted.
4. You should see the following dialog if the installer succeeded:
   
   ![image](https://github.com/user-attachments/assets/000d1a87-7297-4c58-93ba-03ea8cdb1035)
   
5. If you wish to switch your shell to the Windows 7 explorer right now, use the option for it. You can always change back by running ex7forw8.exe once again and selecting the "Use Windows 8 explorer" option (this is currently misnamed, all it does is revert to your system's default shell)
6. Enjoy!
</details>
 

In case you have an unsupported explorer.exe version you wish to try your luck on, or your installation medium is in another language, you may try **manually patching and installing** providing your own files:

<details>
  <summary>Manual Installation/Patching</summary>

**Pre-Requirements:**
1. explorer7 package from releases
2. [CFF Explorer](https://ntcore.com/files/CFF_Explorer.zip)
3. Valid installation medium of your choice (Windows XP x64 - Windows 7 SP1 x64)
4. [7-Zip](https://www.7-zip.org/) or [WinRAR](https://www.win-rar.com/start.html) unless you want to mount install.wim using DISM to extract a few files like a maniac
5. Slight experience utilizing a personal computer

**Step 1 - Fetching files**

**NOTE:** Windows XP did not have MUI files You only need the `explorer.exe` from it. You can also skip the en-US folder creation part.

1. Mount your install media
2. Open `\sources\install.wim` using your archiver of choice (listed 2 in the pre-requirements)
3. Fetch the following files from install.wim (copy them somewhere safe): `\1\Windows\explorer.exe`, `\1\Windows\en-US\explorer.exe.mui` and `\1\Windows\System32\en-US\shell32.dll.mui`
4. Make an "en-US" folder in the folder which contains the explorer7 package. The file tree will look something like the following:
```
ex7_example/
├─ theme/
├─ en-US/
├─ ex7forw8.exe
├─ Import Me.reg
├─ README.txt
├─ wrp64.dll

```
5. Copy `shell32.dll.mui` and `explorer.exe.mui` to the `en-US` folder you've just created, and `explorer.exe` alongside `wrp64.dll`:
```
ex7_example/
├─ theme/
├─ en-US/
│  ├─ explorer.exe.mui
│  ├─ shell32.dll.mui
├─ ex7forw8.exe
├─ explorer.exe
├─ Import Me.reg
├─ README.txt
├─ wrp64.dll

```

Now you should have all of the necessary files to go onto the next step.

**Step 2 - Patching explorer.exe**

**NOTE:** For now, do not replace the `SHLWAPI.DLL` import on XP x64's `explorer.exe`.

By default, explorer.exe will not use the wrapper dll, so you have to change out a few imports in the executable. Make sure you've fetched [CFF Explorer](https://ntcore.com/files/CFF_Explorer.zip) from the requirements.
1. Open CFF Explorer, drag explorer.exe into the window
2. Open the "Import Directory" folder in the left sidebar
3. Change out the imports for `SHLWAPI.DLL`, `OLE32.DLL` and (if applicable) `EXPLORERFRAME.DLL`:
![image](https://github.com/user-attachments/assets/3122093d-8068-49c1-80a5-161468a65dfe)
4. Save the file.

By now, you should be able to start `explorer.exe` from task manager or through other means. 

</details>


## Registry keys

These keys are located under `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced`.

| Name | Type | Description | Default Value |
| ---- | ---- | ----------- | ------------- |
| Theme | REG_SZ | Name of the theme file to use. This is relative to the installation directory. For example, `"aero"` will use the theme at `"explorer7\theme\aero.msstyles"`, `"Aero\aero"` will use the theme at `"explorer7\theme\Aero\aero.msstyles"`. If this is not specified, `aero` will be used. | **aero** |
| DisableComposition | REG_DWORD | When set to 1, Explorer7 will act as if the Desktop Window Manager is not running. | **0** |
| ClassicTheme | REG_DWORD | When set to 1, Explorer7 will use the Windows Classic theme. | **0** |
| EnableImmersive | REG_DWORD | Controls the ability to run UWP apps in the system. When set to 0, UWP apps won't run. | **1** |
| EnableUWPAppsInStart | REG_DWORD | When set to 0, UWP apps will be hidden from the All Programs list. | **1** |
| HideUserPicture | REG_DWORD | When set to 1, the user picture will be hidden from the Windows XP start menu, like when the Welcome screen is disabled. | **0** |

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

## Custom orbs

As an additional feature, explorer7 lets you import your own custom orbs without having to patch your explorer.exe using Resource Hacker or using specialized programs. Due to WinGDI limitations, it only supports .bmp images. To do this, simply make a directory inside the "orbs" folder and place your images inside it with the naming scheme from the example layout below. If it finds the appropiate images, the orb system will also account for 125% and 150% DPI (HiDPI) automatically. The layout should be as it follows:

<details>
  <summary>Valid layout for custom orbs:</summary>

`OrbDirectory` registry key set to `blue`
```
explorer7/
├─ orbs/
│  ├─ blue/
│  │  ├─ 6801.bmp (100% DPI - 52x162 - Bottom-aligned taskbar image)
│  │  │  6802.bmp (125% DPI - 66x198 - Bottom-aligned taskbar image)
│  │  │  6803.bmp (150% DPI - 81x243 - Bottom-aligned taskbar image)
│  │  │  6804.bmp (190% DPI - 106x318 - Bottom-aligned taskbar image)
│  │  │  6805.bmp (100% DPI - 52x162 - Left/right-aligned taskbar image)
│  │  │  6806.bmp (125% DPI - 66x198 - Left/right-aligned taskbar image)
│  │  │  6807.bmp (150% DPI - 81x243 - Left/right-aligned taskbar image)
│  │  │  6808.bmp (190% DPI - 106x318 - Left/right-aligned taskbar image)
│  │  │  6809.bmp (100% DPI - 52x162 - Top-aligned taskbar image)
│  │  │  6810.bmp (125% DPI - 66x198 - Top-aligned taskbar image)
│  │  │  6811.bmp (150% DPI - 81x243 - Top-aligned taskbar image)
│  │  │  6812.bmp (190% DPI - 106x318 - Top-aligned taskbar image)

```

`OrbDirectory` registry key set to `colors\green`
```
explorer7/
├─ orbs/
│  ├─ colors/
│  │  ├─ green/
│  │  │  ├─ 6801.bmp (100% DPI - 52x162 - Bottom-aligned taskbar image)
│  │  │  │  6802.bmp (125% DPI - 66x198 - Bottom-aligned taskbar image)
│  │  │  │  6803.bmp (150% DPI - 81x243 - Bottom-aligned taskbar image)
│  │  │  │  6804.bmp (190% DPI - 106x318 - Bottom-aligned taskbar image)
│  │  │  │  6805.bmp (100% DPI - 52x162 - Left/right-aligned taskbar image)
│  │  │  │  6806.bmp (125% DPI - 66x198 - Left/right-aligned taskbar image)
│  │  │  │  6807.bmp (150% DPI - 81x243 - Left/right-aligned taskbar image)
│  │  │  │  6808.bmp (190% DPI - 106x318 - Left/right-aligned taskbar image)
│  │  │  │  6809.bmp (100% DPI - 52x162 - Top-aligned taskbar image)
│  │  │  │  6810.bmp (125% DPI - 66x198 - Top-aligned taskbar image)
│  │  │  │  6811.bmp (150% DPI - 81x243 - Top-aligned taskbar image)
│  │  │  │  6812.bmp (190% DPI - 106x318 - Top-aligned taskbar image)

```
  
</details>

**NOTE 1:** BE CAREFUL! If the image corresponding to your case DOES NOT exist in your orb directory, it will automatically fall back to the original image inside explorer.exe.

**NOTE 2:** If an image is larger than what the system expects, the image might clip out. Use the example layout as a reference! For more information, you can also check out this guide: https://www.sevenforums.com/tutorials/73616-how-create-custom-start-orb-image.html

**NOTE 3:** If you're looking to create high-quality orbs (32-bit bitmaps), you could use a tool to convert your images from other formats. Check out [Pixelformer](https://www.qualibyte.com/pixelformer/).

## Development plan

We're working based on a milestone stage. Here's the planned stages of development:

|   Stage   | Goal | Status |
| -------- | --------- | ------ |
| Milestone 1 | Project start, stability on Windows 8.1 and a solid base for Windows 10 support. | ✅ Completed |
| Milestone 2 | Ironing out any last Windows 8.1-specific bugs, stability on Windows 10, UWP support, some QOL work (installer, configurator, older .msstyles support, custom orb support) and a solid base for Windows 11 | ⏳ Work in progress |
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
