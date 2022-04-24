# NWScript Tools for Notepad++ Help

## PLUGIN USAGE:

### Automatic Syntax Highlighting and first procedures

   - When first installed, the plugin will ask you permission to copy some extra `XML` files to (sometimes) a protected directory inside the `Notepad++` installation path. It is **recommended** that you allow this action to take place, and then the `Automatic Syntax Coloring` will happen. If you want to know why this is necessary, it is because when booted for the first time, the `NWScript-Npp.xml` definitions file for the plugin will be empty. This happens because the `Auto Complete XML` file will also not exist -> and this last one is usually placed inside a protected directory on Notepad++ installation path. Since the `NWScript-Npp.xml` and the `Auto Complete XML` files need to be in sync with each other at all times, the only startup option is to create a dummy `NWScript-Npp.xml` just to avoid `Notepad++` rejecting the plugin entirely, and then wait for the user to install the files - or allow us to do it for them. This action will often require `elevation` - so the Plugin will ask you to run `Notepad++` in `Administrator Mode` to copy the files automatically for you.
**Remark:** If the plugin detects write permissions to those files *(eg: you have a portable Notepad++ installation on an unprotected directory)*, this process will happen automatically and no action on your part will be required.
   - And if for any reasons you'd rather not give the plugin `Administrator` access, you may also **copy the required files manually** to their respective destinations. When you download a release `.zip` file from the plugin `Github page` (see this [`link`](https://github.com/Leonard-The-Wise/NWScript-Npp/releases)), inside that archive you will find two folders - one named `autoCompletion` and other `functionList`. Just put their respective contents inside the `Notepad++` installation path, overwriting any file if necessary. After placing the two folders, copy the `NWScript-Npp.xml` file to your `%AppData%\Notepad++\plugins\Config` folder, overwriting the file present there (remember, it's a `dummy` file) and then you are done.
**Note:** when the plugin does it, it does not overwrite the `overrideMap.xml`, instead we patch the file properly, so if you use multiple lexers you won't lose any pre-configured options for them, hence the option to give us admin access is the most recommended. But if you don't use other `Lexer plugins` or `User-Defined Languages` it is completely safe to just ovewrite the `overrideMap.xml` anyway.
   - After doing the above process, we also recommend you to take a moment to import your `Custom Constants` and `functions` to the plugin so we may Color Syntax them for you. Check the Menu Option `Import User-Defined Tokens` bellow. You may also want to install the Dark Theme lexer support for Notepad++ (this usually requires elevated privileges also), so when you switch between Dark and Light mode, the transition on your text will be seamless.
   - If for any reason you miss or deny the Plugin's first request to install the configuration files, there will be a Menu Option called `Install Plugin's XML Config Files` avaliable to you. This option only shows up when the plugin don't detect the files, so if you already installed them manually you won't see this option anymore.

Remarks:
   - To see the colored syntax of `NWScript files`, just select `NWScript` from the Languages menu. Do this **after** the required files are installed (and doing a Notepad++ restart if necessary) or else you may not notice any difference.


### Auto-Indentation support:

   - This plugin adds features to support auto-indentation for `NWScript` language. In `Notepad++` versions `8.3.2` and bellow, chose the option `Use Auto-Indentation` to enable the plugin's built-in auto-indentation support. You'll need to disable `Notepad++` Auto-Indentation in user preferences to avoid any conflicts. 
   
   This feature is automatic on `Notepad++ 8.3.3` and beyond so this menu option won't show anymore for users with up-to-date versions.

### Menu option - “Compile script”:

   - Compiles the current opened document into the `Output Directory` set in `Compiler Settings`. If no output directory is specified, the script current directory will be used as output instead.

**Remarks**
   - The plugin’s version of the compiler now supports UTF-16 encoding. Previous versions only supported UTF-8. Although this support is primarily intended for convenience use only – since UTF-16 is also part of Notepad++ standard editor. I don’t really recommend using extended characters here, unless inside strings and it is untested whether the game can display them properly. So, use with caution.

### Menu option - “Disassemble file”:

   - Disassembles a compiled `NWscript file` from the disk and put results into the `Output Directory` set in `Compiler Settings`. If no output directory is specified, the file current directory will be used as output instead.


### Menu option - “Batch processing”:

   - Opens the `Batch-processing` dialog box. Batch processing enables to compile/disassemble multiple files at once.

**Remarks**
   - The same `include folders` on the `compiler settings` will be used to `batch process` files, also the `Neverwinter Installation Paths`, etc., and other options not exclusive for batch processing.
   - When writing file filters, you may use any normal Windows file wildcards. Also, this field accepts a list of file filters separated by commas. **Do not** use spaces in between commas. Example of valid filter: `*x.nss,nw_s0_*.nss,nw_s1_*.nss`.
   - File filters are saved separately for `Compile` and `Disassembly` mode, hence you may have 2 different sets, one for each mode.

### Menu option - “Run last batch”:

   - Runs the last successful batch operation in this session. This option is only enabled after a successful batch is processed on a given session.

### Menu option - “Fetch preprocessor output”:

   - Runs a compile preprocessing phase on current script and display the results in a new document for the user. Useful to view what final text the compiler will ACTUALLY use to compile the script. This will replace whatever `#include` directives you have in your script with the ACTUAL `#included` file content, recursing if necessary... so the results of preprocessing can get really large real quickly.

### Menu option - “View Script Dependencies”:

   - Parse the script file's dependencies and display to the user as a new human-readable document.
   NOTICE: This is `NOT` the same of `generate makefile (.d) dependencies file` option on the Compiler Settings. That one must be used instead if you are exporting scripts to makebuild projects. Also, the Compiler Settings will generate makefile dependencies in batch operations if `generate makefile (.d) dependencies file` is set... this option here will only display dependencies on a single file inside a Notepad++ document window.

### Menu option - “Compiler settings”:

   - Opens the compiler settings. This is required when compiling scripts. If you try to compile anything before setting configurations here you will be prompted to configure first. All settings are persisted automatically upon closing `Notepad++`.
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
   - And also to auto-install `Dark Theme` support on `Notepad++ upgrades` if previously installed - see Menu Option `Install Dark Theme` bellow (default `OFF`).
	
   **Remarks**
   - To be able to auto-install the `Dark Theme` when `Notepad++` changes version, you'll either be asked to run `Notepad++` with `Admin privileges`, or if you don't want to do that, you'll have to manually provide write permissions on the following path (depending on which build you use), if that file is located on a secure path (like `%ProgramFiles%`):
```	
    %NotepadInstallPath%\themes\DarkTheme.xml
```	
   If you cancel the auto-install attempt (because you refused to run `Notepad++` as `Administrator`), no other attempts will be made until the next version of `Notepad++` - given, of course that the `Auto-Install` option is still checked when version changes.

### Menu option - “Install Dark Theme”:

   - Installs Dark Theme support if not already present. When installation is detected, this option won't show up.
   
   **Remarks**
   - Since the `DarkTheme.xml` file used for the `Dark Mode` (`User Preferences`) is distributed within each release of a new `Notepad++`, it is probable that when upgrading your `Notepad++` version, this file will be overwritten, hence, uninstalling the plugin's patch (if you did install the plugin's `Dark Theme` before). We can't change the stock `DarkTheme.xml`, because that one is made for all built-in `Notepad++` languages, and we are a plugin... so the only option is to re-patch this file in each and every `Notepad++` upgrade. Fortunately we have the option to `auto-install` this theme everytime `Notepad++` upgrades - the plugin will detect if the file has been modified and a previous installation was done, and will `auto-install` the feature upon `Notepad++` `post-upgrade initialization`. For more information, see above for the option to auto-install the Dark Theme in `User's Preferences`.

### Menu option - “Import NWScript definitions”:

   - With this option, you may import a new `nwscript.nss` to replace the current engine definitions like constants, functions and engine structures to use with syntax coloring and highlighting and also this enables the `Auto Complete` functions to them. This will overwrite any previous engine definitions present on the plugin configuration.

### Menu option - “Import user-defined tokens”:

   - With this option, you may import new `user-defined functions` and `constants` from any `.nss` file to enable `color-syntax highlighting` and `auto-completion` to them. 
   
   Please notice that only **`FUNCTION DECLARATIONS`** and **`GLOBAL CONSTANTS`** will be imported by this process. And if you already have any `user-defined functions` and `constants` previously imported or in use, don't worry, they will be preserved, as long as you did **NOT** put them manually inside the `reserved sections` of the [`XML configuration`](https://github.com/Leonard-The-Wise/NWScript-Npp/blob/master/src/Lexers/Config/NWScript-Npp.xml) file. So I advise you to **NEVER** edit that file manually. Like, ***ever***!!!

### Menu option - “Reset user-defined tokens”:

   - This will clear ANY `user-defined functions` and `constants` previously imported to the plugin's configurations. Usefu when you are done working in a project and want all your symbols to be clear, so you can start anew.

### Menu option - “Reset editor colors”:

   - This will reset all editor `color styles` to the `default values` - either for `light` and `dark` themes. No functions or constants definitions will be ereased in the process. The main purpose of this function is to correct a known `Notepad++` behavior that changes your `Npp-Plugin.xml` stylers when you change themes inside the `Settings -> Style Configurator` dialog box. Since `Notepad++` must edit the plugin styler directly to save the settings, that will overwrite whichever colors it had before, with no custom theme support. Hence, it is handy to have a `reset` option so when you change back themes you won't need to manually set ALL of the stylers manually again.
   
### Menu option - “Repair Function List”:

   - This is useful if more than one Lexer plugin is present. Sometimes other plugins may overwrite the `overrideMap.xml` file inside the `functionList` directory, rendering the Functions List panel for NWScript files to not work anymore. In this case, the NWScript Tools can auto-fix that file for you (and this also will also cause the plugin to re-copy nwscript.xml into the `functionList` directory.

**Remarks**
   - The plugin will always checkup the `overrideMap.xml` file integrity upon startup, so if everything is ok, then this option will not show up to you.
   - Unfortunately, up to version `8.3.3`, Notepad++ still does not support Function List for external languages (like of Plugin's). But still I found this option to be relevant for shipping on this release, because we are working on a patch for that, so when it comes (soon), the plugin will already be prepared for that.

### Menu option - “Install Plugin's XML Config Files”:
   - This option will only be avaliable when you choose to not let the plugin install the required XML files for it to function properly. If you select it, you'll be asked to run `Notepad++` in `Administrator Mode` if the plugin don't detect to have write permission to the required files.

### Menu option - “About me”:

   - Opens the `About Dialog Box` (yeah, really!). 🙄

## REMARKS:

   - If you noticed the `SHIELD` icons near some menu options, that's because if you want to use that option you must either run `Notepad++` with `Administrator Privileges` OR you may also manually provide write permissions to the option's required files, because they are located in a secured path like `%ProgramFiles%` or so. Don't worry, you will be notified of which file you need to provide permissions to for each operation.
   
   Options that requires elevated privileges will usually access these files bellow, in a combination of one or more files for each option:
```   
       - %ProgramFiles%\Notepad++\themes\DarkTheme.xml
       - %ProgramFiles%\Notepad++\autoCompletion\nwscript.xml
       - %ProgramFiles%\Notepad++\functionList\nwscript.xml
       - %ProgramFiles%\Notepad++\functionList\overrideMap.xml
       - %AppData%\Notepad++\plugins\Config\NWScript-Npp.xml
```  
   So, unless you don't want to keep being bugged about files permissions, you can set write permission on those files permanently by going into the `Windows Explorer`, selecting the file's `Properties`, `Security tab`, selecting your `Username` and then marking all permission options avaliable. Then magically, the shield icons will vanish from your interface (requires a `Notepad++` restart).
   
   **Note:** Files on the `%AppData%` folder usually are **not protected**, so that last one is listed there just for reference.

## FINAL CONSIDERATIONS:

   - Any bug report or improvement request must be done [`here`](https://github.com/Leonard-The-Wise/NWScript-Npp/issues), so I can keep track of all changes. No email requests, forum messages or personal communications will be taken in consideration for that. Please be noticed.

## EXTRA COPYRIGHT INFO:

Copyright notices for the embbeded version of the [`NWScript Compiler Library`](https://github.com/nwneetools/nwnsc):

  Portions copyright (C) 2008-2015 `Skywing`<br>
  Portions copyright (C) 2002-2003, `Edward T. Smith`<br>
  Portions copyright (C) 2003, `The Open Knights Consortium`<br>
  Adapted for Neverwinter Nights Enhanced Edition and cross platform use by: `Glorwinger` and `Jakkn`<br>
  Readaptations and tweaks for Windows GUI applications usage by: `Leonard-The-Wise`<br>
  
  
