# NWScript Tools for Notepad++ Help

## PLUGIN USAGE:

### Automatic Syntax Highlighting

   - To see the colored syntax of NWScript files, select NWScript Language from Languages menu.

### Auto-Indentation support:

   - This plugin adds features to support auto-indentation for NWScript language. In `Notepad++` versions `8.3.2` and bellow, chose the option `Use Auto-Indentation` to enable the plugin's built-in auto-indentation support. You'll need to disable Notepad++ Auto-Indentation in user preferences to avoid any conflicts. 
   
   This feature is automatic on `Notepad++ 8.3.3` and beyond so this menu option won't show anymore for users with up-to-date versions.

### Menu option - “Compile script”:

   - Compiles the current opened document into the `Output Directory` set in `Compiler Settings`.

### Menu option - “Disassemble file”:

   - Disassembles a compiled NWscript file from the disk and put results into the `Output Directory` set in `Compiler Settings`.

### Menu option - “Batch processing”:

   - Opens the Batch-processing dialog box. Batch processing enables to compile/disassemble multiple files at once.

### Menu option - “Run last batch”:

   - Runs the last successful batch operation in this session.

### Menu option - “Fetch preprocessor output”:

   - Runs a compile preprocessing phase on current script and display the results in a new document for the user. Useful to view what final text the compiler will ACTUALLY use to compile the script. This will replace whatever `#include` directives you have in your script with the ACTUAL `#included` file content, recursing if necessary... so the results of preprocessing can get really large real quickly.

### Menu option - “View Script Dependencies”:

   - Parse the script file's dependencies and display to the user as a new human-readable document.
   NOTICE: This is `NOT` the same of `generate makefile (.d) dependencies file` option on the Compiler Settings. That one must be used instead if you are exporting scripts to makebuild projects. Also, the Compiler Settings will generate makefile dependencies in batch operations also... this option here will only display dependencies on a single file.

### Menu option - “Compiler settings”:

   - Opens the compiler settings. This is required when compiling scripts (if you try to compile anything before setting configurations here you will be prompted to configure first). All settings are persisted automatically upon closing `Notepad++`.
   - Also, to compile a script, you **MUST** have at least the `nwscript.nss` file present in one of your compilation paths (except for the `output directory`). If you set the plugin to a Neverwinter Nights installation path, that will be automatically loaded for you, so you won't have to worry about that.
   - `Neverwinter Nights 1` support any version of the original game and also the `Enhanced Edition` version.
   - Same for the `Neverwinter Nights 2`.
   
   **Some considerations**:
   - Don’t use the same file names on different `Additional Include Paths`, or else you may end up having unexpected behaviors – like the wrong version of the file being used. If for example, you set to use the `Neverwinter Nights Installation Folders`, don’t copy the original scripts to other include paths, because the first option will be considered and if you edit the copies outside the Neverwinter Folders, those edits won’t be taken into account. In this case you should first disable using the `Neverwinter Nights Installation Folders` before editing the files. (*It’s not recommended to edit the original scripts anyway, but they are known to emit some compilation warnings, especially when including scripts that deal with spells, so some people do like changing them on their projects, hence I decided to leave this warning here*).   
   - The option to run on strict mode is set to enable some stock compiler unsafe conditions that were possible on older versions of the compiler library. The recommended option is to leave this turned `OFF`, unless you know what you're doing.
   - Also, the default compilation is set to `Game Target Version` = `174`. This is the default option for `Neverwinter Nights 2`, `Enhanced Edition`, `Gold`, etc., only older versions will use `169`. (Default = `174`).
   - If you want faster compilation times for a large number of files, recommended to use `batch operations`, since that will cache game resources and treat the batch as a whole project, preserving include header files already parsed in memory to speed up the process.
   - Options to disable `\”` and `\\` processing were added in `NWN:EE` versions.

### Menu option - “User's Preferences”:

   - Opens the User's Preferences window. Here you may chose to:
   - Auto-open files disassembled files upon disassemble (default `ON`);
   - Auto-open Debug Symbols on compilations (default `ON`);
   - And also to auto-install Dark Theme support on Notepad++ upgrades if previously installed - see Menu Option `Install Dark Theme` bellow (default `OFF`).
	
   **Remarks**
   - To be able to auto-install the `Dark Theme` when `Notepad++` changes version, you'll either be asked to run `Notepad++` with `Admin privileges`, or if you don't want to do that, you'll have to manually provide write permissions on the following path (depending on which build you use), if that file is located on a secure path (like `%ProgramFiles%`):
	
    %NotepadInstallPath%\themes\DarkTheme.xml
	
   If you cancel the auto-install attempt (because you refused to run `Notepad++` as `Administrator`), no other attempts will be made until the next version of `Notepad++` - given, of course that the `Auto-Install` option is still checked when version changes.

### Menu option - “Install Dark Theme”:

   - Installs Dark Theme support if not already present. When installation is detected, this option won't show up.
   
   **Remarks**
   - Since the `DarkTheme.xml` file used for the `Dark Mode` (User Preferences) is distributed within each release of a new `Notepad++`, it is probable that when upgrading your `Notepad++` version, this file will be overwritten, hence, uninstalling the plugin's patch (if you did install the plugin's `Dark Theme` before). We can't change the stock `DarkTheme.xml`, because that one is made for all built-in `Notepad++` languages, and we are a plugin... so the only option is to re-patch this file in each and every `Notepad++` upgrade. Fortunately we have the option to `auto-install` this theme everytime `Notepad++` upgrades - the plugin will detect if the file has been modified and a previous installation was done, and will auto-install the feature upon `Notepad++` `post-upgrade initialization`. For more information, see above for the option to auto-install the Dark Theme in `User's Preferences`.

### Menu option - “Import NWScript definitions”:

   - With this option, you may import a new `nwscript.nss` to replace the current engine definitions like constants, functions and engine structures to use with syntax coloring and highlighting and also this enables the `Auto Complete` functions to them. This will overwrite any previous engine definitions present on the plugin configuration.

### Menu option - “Import user-defined tokens”:

   - With this option, you may import new `user-defined functions` and `constants` from any `.nss` file to enable `color-syntax highlighting` and `auto-completion` to them. 
   
   Please notice that only **`FUNCTION DECLARATIONS`** and **`GLOBAL CONSTANTS`** will be imported by this process. And if you already have any `user-defined functions` and `constants` previously imported or in use, don't worry, they will be preserved, as long as you did **NOT** put them manually inside the `reserved sections` of the [`XML configuration`](https://github.com/Leonard-The-Wise/NWScript-Npp/blob/master/src/Lexers/Config/NWScript-Npp.xml) file. So I advise you to **NEVER** edit that file manually. Like, ***ever***!!!

### Menu option - “Reset user-defined tokens”:

   - This will clear ANY `user-defined functions` and `constants` previously imported to the plugin's configurations. Usefu when you are done working in a project and want all your symbols to be clear, so you can start anew.

### Menu option - “Reset editor colors”:

   - This will reset all editor `color styles` to the `default values` - either for `light` and `dark` themes. No functions or constants definitions will be ereased in the process. The main purpose of this function is to correct a known `Notepad++` behavior that changes your `Npp-Plugin.xml` stylers when you change themes inside the `Settings -> Style Configurator` dialog box. Since `Notepad++` must edit the plugin styler directly to save the settings, that will overwrite whichever colors it had before, with no custom theme support. Hence, it is handy to have a `reset` option so when you change back themes you won't need to manually set ALL of the stylers manually again.

### Menu option - “About me”:

   - Opens the `About Dialog Box` (yeah, really!). 🙄

## REMARKS:

   - If you noticed the `SHIELD` icons near some menu options, that's because if you want to use that option you must either run Notepad++ with Administrator Privileges OR you may also manually provide write permissions to the option's required files, because they are located in a secured path like %ProgramFiles% or so. Don't worry, you will be notified of which file you need to provide permissions to for each operation.
   
   Options that requires elevated privileges will usually access these files bellow, in a combination of one or more files for each option:
   
       - %ProgramFiles%\Notepad++\themes\DarkTheme.xml
       - %ProgramFiles%\Notepad++\functions\nwscript.xml
       - %ProgramFiles%\Notepad++\plugins\Config\NWScript-Npp.xml
  
   So, unless you don't want to keep being bugged about files permissions, you can set write permission on those files permanently by going into the `Windows Explorer`, selecting the file's `Properties`, `Security tab`, selecting your `Username` and then marking all permission options avaliable. Then magically, the shield icons will vanish from your interface (requires a `Notepad++` restart).

## FINAL CONSIDERATIONS:

   - Any bug report or improvement request must be done [`here`](https://github.com/Leonard-The-Wise/NWScript-Npp/issues), so I can keep track of all changes. No email requests, forum messages or personal communications will be taken in consideration for that. Please be noticed.

## EXTRA COPYRIGHT INFO:

Copyright notices for the embbeded version of the [`NWScript Compiler Library`](https://github.com/nwneetools/nwnsc):

  Portions copyright (C) 2008-2015 `Skywing`<br>
  Portions copyright (C) 2002-2003, `Edward T. Smith`<br>
  Portions copyright (C) 2003, `The Open Knights Consortium`<br>
  Adapted for Neverwinter Nights Enhanced Edition and cross platform use by: `Glorwinger` and `Jakkn`<br>
  Readaptations and tweaks for Windows GUI applications usage by: `Leonard-The-Wise`<br>
  
  
