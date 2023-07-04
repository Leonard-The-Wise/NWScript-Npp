![Notepad++ Plugin](https://img.shields.io/badge/Notepad++-Plugin-green.svg?&logo=notepad%2B%2B) ![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-blue?logo=visual-studio) ![C++](https://img.shields.io/badge/c++-red.svg?&logo=c%2B%2B)

![GPL-3.0 License](https://img.shields.io/badge/License-GPL%20v3-green)

# NWScript Tools for Notepad++ 

## What is this plugin?

Well, since there's not much functionality to User-Defined Languages in [`Notepad++`](https://notepad-plus-plus.org/) I decided to create a custom `Lexer` to support [`Bioware's NWScript language`](https://en.wikipedia.org/wiki/NWScript).

A [`Lexer`](https://en.wikipedia.org/wiki/Lexical_analysis) is just a program that can read, interpret and highlight programming code syntax.

But in this plugin, things rapidly expanded, so I also decided to provide the tools to compile and publish `NWScript` files directly into the target games: **`Neverwinter Nights` [`Enhanced`](https://www.beamdog.com/games/neverwinter-nights-enhanced/)** and **[`2`](https://dnd.wizards.com/products/digital-games/pcmac/neverwinter-nights-2-complete)** (this one still a Work In Progress).

The plugin adds support for function Auto-Completion and can import newly defined functions from `nwscripts.nss` (the main Neverwinter engine header).

<br>

## Installation

If installing manually, create a folder called `NWScript-Npp` inside the `Notepad++` plugins installation folder and extract all of the contents of the `.zip` archive there. Or, use the Notepad++ Plugins manager that will do it all automatically for you (TBA). After that, when the plugin starts, it will ask you to copy or patch some of the Notepad++ XML configurations file. Follow the instructions on screen or open the `About Me` dialog box and read the first section of the Help there (also avaliable as online text [here](OnlineHelp.md)).

<p align="center">Here is a sample of the plugin's funcionality:</p>
<p align="center"><img src="https://github.com/Leonard-The-Wise/NWScript-Npp/blob/master/Media/Sample-Colorization.jpg" width="45%" height="45%"></p>

<p align="center">Dark Mode version:</p>
<p align="center"><img src="https://github.com/Leonard-The-Wise/NWScript-Npp/blob/master/Media/Sample-Colorization-DarkMode.jpg" width="45%" height="45%"></p>

The unity test file I used to take this screenshot is provided [here](Media/UnityTest.nss).

<br>

## Known Issues

***`On NWN Enhanced Edition patch 8193.35`***:

- This game version contains an NWScript.nss with a problem that won't let any script compile. The error is the following:
 <p align="center"><img src="https://user-images.githubusercontent.com/68194417/240757677-40e36a11-a7ce-4b15-bfa8-8df18a125643.png" width="45%" height="45%"></p>

-   The only way to fix this is getting an older version of NWScript.nss and do not use game path while compiling the script.

***`Up to Notepad++ 8.3.2 and prior versions`***:

-   To use the plugin's built-in auto-indentation, you must first disable `Notepad++'s` Auto-Indentation function:
```
Settings -> Auto-Completion -> Auto-Indent (checkbox)
```

-   Also while playing back a macro, especially the type-in ones, you'll want to disable the Plugin's auto-indent feature, as the plugin will not be able to detect a macro playback and will end messing-up any text typed within that macro.

***`From Notepad++ version 8.3.3 to later`***:

-   Those issues were fixed *(thanks [`@DonHo`](https://github.com/donho) for accepting my pull request)* and no longer a concern, the option to select the plugin built-in auto-indentation won't even show up as an option to it's users anymore. So I suggest you to always keep your `Notepad++` version up-to-date. *(if not possible because you use legacy plugins, well, just use the built-in auto-indent function then, it won't bite, I promise)*.

<br>

## Informations For Plugin Developers, Students and Code Explorers
<details>
<summary>Click here to expand</summary>
<br>

This plugin is based on [Notepad++ plugin template](https://github.com/npp-plugins/plugintemplate) and the official [`Scintilla`](https://www.scintilla.org/) `C++ Lexer`. I managed to rewrite much of the code, clear and organize
classes, so anyone desiring to write future lexers will find it much easier to integrate a new lexer inside the Plugin. Just put your `LexXXX.cpp` file on the project and add it to the [`Lexer Catalogue`](/Leonard-The-Wise/NWScript-Npp/blob/master/src/Lexers/LexerCatalogue.cpp) and export it as a `DLL`.

Also, for the `NWScript` compilation, I *"borrowed"* the [`NWScript Compiler`](https://github.com/nwneetools/nwnsc) code, since trying to write a compiler from scratch would be a monstrous task.

All files under this project are provided under the [`GPL v3.0 License`](license.txt).

For reutilization of the project, the `NWScript-Npp.vcxproj` is organized in the following way:

-   **`lib`**: All linked library submodules found here. I got two things there: my ***personal port*** for `NWScript Compiler` called [`NscLib`](https://github.com/Leonard-The-Wise/NscLib) (because it's only the library without the executable) and the [`LunaSVG`](https://github.com/sammycage/lunasvg) library - for managing vectorial images for high DPI support. The plugin project also depends on PCRE2, but it's installed/managed by [`vcpkg`](https://vcpkg.io/). Dependencies are listed on the [`vcpkg.json manifest`](/Leonard-The-Wise/NWScript-Npp/blob/master/NWScript-Npp/vcpkg.json) of the project. Hence, to build my code from source you need it. Follow these steps:

    -   Install vcpkg. Just follow [`this guide`](https://vcpkg.io/en/getting-started.html).
    -   Don't forget the `vcpkg integrate install` part.
    -   Done. The first time you build the project, all dependencies will be automatically installed. Click on `Rescan Solution` after building to update Intellisense. After that you can go to the vcpkg install dir and delete the temporary /downloads and /buildtrees.

-   **`Custom Lexers`**: Here you'll write your new custom Lexers (example: [`LexNWScript.cpp`](src/Lexers/LexNWScript.cpp)) and edit/place them inside [`LexerCatalogue.cpp`](src/Lexers/LexerCatalogue.cpp) `InstalledLexers[]` static object for the code to auto-initialize it upon plugin load. Something like this:

    ``` C++
	constexpr static LexerDefinition InstalledLexers[] = {
	{"YourLexerName", TEXT("Your Lexer Status Text"), ANY_NUMBER, 
	   LexerScript::LexerFactoryFunction, ExternalLexerAutoIndentMode::XXX},}
    };
    ```

    -   Where:

        -   `YourLexerName` is a 16 bytes-length string;
        -   `Your Lexer Status Text` is a 32 bytes-length string (that will be displayed in Notepad++ status bar on the bottom of the screen);
        -   `ANY_NUMBER` is just a number to uniquely-identify the Lexer inside your code (this is not used by `Notepad++` in any way, this is just an internal number and you can set to `0` if wanted ). In my case I `#defined` a `MACRO` for this;
        -   A pointer to a [`“Factory”`](https://en.wikipedia.org/wiki/Factory_method_pattern) function to get your lexer's instantiated object. In my case it just returns a new `LexerNWScript` class pointer - which implements the [`ILexer5`](https://www.scintilla.org/LexillaDoc.html) interface. Like this:
            
        ``` C++
        static ILexer5* LexerFactoryNWScript() {
            return new LexerNWScript(ConstructorVariables, ...);
        }
		```
        -   As a remark, this method is now deprecated, since `Notepad++` implemented the `Scintilla 5.2.2` engine version. Now an additional method is required: `CreateLexer(name)`. This is how `Notepad++` will from now on (version 8.3.4 and forward) will instantiate Lexers, so you must also modify this method to include any other custom lexer avaliable.
        -   The `ExternalLexerAutoIndentMode` `enum class`. This is a new feature I developed for `Notepad++` to help plugins dealing with auto-indentation. Prior to `Notepad++ version 8.3.3`, if you tried to perform a custom-made auto-indentation with your plugin, and `Notepad++` had it's Auto-Indentation preference set to `ON`, it would override your plugin behavior and you wouldn't be able to properly auto-indent user inputs. So from `8.3.3` version and forward, since this is not a standard `ILexer5` functionality, you'll be able to send `Notepad++` the message `NPPM_SETEXTERNALLEXERAUTOINDENTMODE` to make `Notepad++` work in 3 different ways about auto-indentation with your custom language: `Standard`, which will tell `Notepad++` to perform the default behavior (to just maintain any amount of tab spacing of previous line), `C_Like` to tell `Notepad++` your code support a C-Like syntax indentation-\> which will read any curly brackets `{` before a new line and advance the indent amount by one on the next line and then read the other paired curly bracket `}` and go back one step in indentation... or you can tell `Notepad++` that your plugin does `Custom` indentation, so `Notepad++` won't perform ANY kind of auto-indent for your plugin lexer, even if it's set to `ON` inside the user's preferences - because now your plugin will be the one responsible for handling it. You can query `Notepad++` about this user setting with the `NPPM_ISAUTOINDENTON` message. For more info, just study the code, especially the methods `SetAutoIndentSupport()` and `LoadNotepadLexer()` inside my [`PluginMain.cpp`](src/PluginMain.cpp) class, also along with `ProcessMessagesSci()`, especially the `SCN_CHARADDED` message processing, to see how my plugin handles auto-indentation with newer and older versions of `Notepad++`. That field is only present there (on `InstalledLexers[]` variable) to help you if you want your plugin to have more than one `lexer` installed, so you can checkup which `lexers` are installed and to keep track of which auto-indent `mode` they use. `Notepad++` will never need or read that value in any way. Again, check the `SetAutoIndentSupport()` and `LoadNotepadLexer()` methods to understand this ***"language auto-indentation"*** thing better. ***(I also strongly suggest studying `Notepad++'s` `maintainIndentation()` method inside [`NotepadPlus.cpp`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Notepad_plus.cpp) file so you can see how `Notepad++` performs it's own auto-indentation functionality).***

-   **`Notepad Controls`**: Contains some class templates to display dialog boxes. Versions of `Static`, `Modal` and `Dockable` dialogs boxes are avaliable.

-   **`Plugin Interface`**: Contains all code necessary to initialize the DLL and communicate with `Notepad++` main executable, including the Lexer part. You probably won't need to change *(much of)* this code, **EXCEPT** to make it point to YOUR plugin class(es) instead of mine's.

-   **`Resource Files`**: Contains the [XML](src/Lexers/Config/NWScript-Npp.xml) necessary for the Lexer to work with `Notepad++`. Without it, `Notepad++` will just mark your plugin as `incompatible`.

    -   Also contains a [`.targets`](Publish.Dll.To.Notepad.targets) file that is imported inside the [`vcxproj`](NWScript-Npp/NWScript-Npp.vcxproj) [`MSBuild`](https://docs.microsoft.com/en-us/visualstudio/msbuild/msbuild?view=vs-2022) project file to automate deployment of the plugin `DLL` to `Notepad++'s` install directory to help you with your plugin debugging. Make sure `Notepad++` isn't running when you build your code. Also make sure to give yourself **write permissions** to the Notepad/plugin installation folder and subfolders, so the compiler can copy the output `DLL` to that path. You'll be notified if it cannot and also the build will fail and the debugger will not run if it can't deploy at least the `DLL` there (the `XML` deploying is optional and only emits a warning).
    -   Also, I've setup a [`ProjectVersion.rc`](src/ProjectVersion.rc) file along with a header called [`ProjectVersion.h`](src/ProjectVersion.h) to perform auto-increments on the `VS_VERSION_INFO` associated resource. This works as following:
    -   ~~Every time you hit the <kbd>build</kbd> command in Visual Studio, a [pre-build
        event](https://docs.microsoft.com/en-us/visualstudio/ide/specifying-custom-build-events-in-visual-studio?view=vs-2022) occurs, which calls this [`PowerShell`](IncrementBuild.ps1) script on the project root that will edit `ProjectVersion.h` and increment the `VERSION_BUILD` macro inside that file.~~ ***The auto-increment is now disabled by default***.
        -   Then the pre-compiler will read that macro and since `VS_VERSION_INFO` is setup to use macros for replacing version information, it will compile with whichever version is printed on ProjectVersion.h at the time of compilation.
        -   Hence I advise you to ***`NEVER`*** touch or edit `ProjectVersion.rc` inside the [`Resource Editor`](https://docs.microsoft.com/en-us/cpp/windows/resource-editors?view=msvc-170), or it will overwrite and destroy the macros inside and cause you to lose the `build auto-increment` funcionality. Edit it manually (inside any ***raw text editor***) and ***`only`*** to change other info, like `DLL Name`, `Company Name`, `Copyright Info`, etc and leave all the macros there about versioning untouched.
        -   To increment major, minor or patch numbers, edit the `ProjectVersion.h` file instead. ~~Only `build` numbers are setup to auto-increment on my script~~ *(as stated above, the auto-increment is now disabled by default)*, so if you want your `major`, `minor` or `patch` versions to change, you'll have do it manually, editting their respective `VERSION_MAJOR`, `VERSION_MINOR` and `VERSION_PATCH` macros *(leave `VERSION_STRING` and `VERSION_STRING_BUILD` alone as they are)*. I designed this intentionally, since every person or team have its own standards for managing project versions.

-   **`Utils`**: Contains utilitary headers and code to help dealing with settings, `.ini` files, `regular expressions`, etc.

-   **`DarkMode`**: This is a ripped-off and enhanced/modified version of `Notepad++` experimental `Dark Mode` support. It can be reused in other plugins with few to none modifications, so you may use the same interface `Notepad++` uses to implement it's `Dark Mode` interface now. It is based on `Uxtheme.lib` library and `Vsstyles.h`. Also contains a class called [`DPIManager`](/src/DarkMode/DPIManager.h) to deal with all kind of things related to high `DPI` support. Please, notice that `Dark Mode` for `Win32 API` is experimental, and a lot of things on it are undocumented features Microsoft implemented prior to pushing forward `Windows WPF` and `UWP`, so many things are unsupported and/or unimplemented there, making us to rely more on [`Subclassing`](https://docs.microsoft.com/en-us/windows/win32/controls/subclassing-overview) controls and `Custom/Owner-Drawing`(https://docs.microsoft.com/en-us/windows/win32/controls/about-custom-draw) our own versions. Expect to find most control classes avaliable there, and a few ones (like the `DataGrid`) that currently don't have this support.

-   **`Root Directory`**: This is where the Plugin code really begins. I designed a base [`Singleton`](https://en.wikipedia.org/wiki/Singleton_pattern) class called [`PluginMain`](src/PluginMain.h) to setup the Menu Commands, to deal with message processing, and all of the main plugin funcions, because, yeah... it will be created only once during a session or DLL loading. You'll need to change this as suitable. Perhaps in the future I'll clean up the code from my specific usage and leave a framework for others to developed upon. No promises made, though **(and hey, it's easy to delete a `PluginMain.cpp` and add your own class... just don't forget to update `PluginInterface.cpp` to point to your own classes instead of mine for handling plugin initialization, message parsing, etc)**.

    -   Also, since many plugins use `.ini` files to store their settings, I already provided a [`Settings.cpp`](src/Settings.cpp) class that will do that *(almost)* automatically for you. Just replace my variables with yours, update the `Save()` and `Load()` functions to save/load your variables instead and you're done. The Settings class uses a modified version of [`MiniINI`](https://github.com/pulzed/mINI/blob/master/src/mini/ini.h) API to handle ini files reading, writting, etc., so it's really simple to use instead of writting your own version. It supports `ANSI` and `UNICODE` files and filenames.
    -   And the `Common.h` file is just a bunch of aggregated functions I wrote myself or captured over the web, to help me dealing with unicode strings, conversions, Windows Icon and Bitmap handling, etc... (the method I developed for the `Notepad++` auto-restart functionality with a temporary [`batch`](https://www.windowscentral.com/how-create-and-run-batch-file-windows-10) file involved into a [`ShellExecute`](https://docs.microsoft.com/en-us/windows/win32/shell/launch) API call was kind of... crusty... 🤣 but since I did not know of any other method out there and was a bit lazy to research more on this when I was writting features, well... I'll just leave that there... for now. 😇).

-   **Last** but not least: `Plugin Dialogs` are just the instanced versions of `Notepad Controls` classes, to manage MY specific dialog boxes, etc. You really don't need these, except if you want to use them as examples.

> ***All other files on this project are just internal work for my plugin specific funcionalities, and hence I will not be providing too much information on them here. I consider the code at least reasonably documented and commented already anyway, so feel free to explore it by yourself.***

### Some Project Setup Remarks

-   [`NWScript-Npp.vcxproj`](NWScript-Npp/NWScript-Npp.vcxproj) file sets the `<PlatformToolset>` to [`v143`](https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-tools-and-features-in-visual-studio-editions?view=msvc-170) for using with [`Visual Studio 2022`](https://visualstudio.microsoft.com/vs/).

-   Also, we are targeting [`ISO C++ 20`](https://en.wikipedia.org/wiki/C%2B%2B20) standard here, and since Visual Studio 2022 still don't support `std::format` on its `ICO C++20` implementation, we set the project to use `Preview features from the latest C++ working draft`.

-   Interface functions required for NPP to use the lexer are all declared with:

    ``` C++
    extern "C" __declspec(dllexport)
    ```

    -   I created a `MACRO` called `DLLAPI` to help with that, so if parts of your code are to be used in other `DLLs`, it will change to:
        

    ``` C++
    extern "C" __declspec(dllimport)
    ```

    -   And if linking statically to a code, it will `#define` `DLLAPI` to nothing.

-   [`src/Lexers/Scintilla`](src/Lexers/Scintilla), [`src/Lexers/Lexilla`](src/Lexers/Lexilla) and [`src/Lexers/Lexlib`](src/Lexers/Lexlib) are unmodified files copied from the Scintilla and Lexilla projects appended to Notepad++. You can update them with newer versions when needed.

-   [`src/Lexers/Config/NWScript-Npp.xml`](src/Lexers/Config/NWScript-Npp.xml) defines the language keywords & styles. Required for the plugin and will be published on project build. When changing the `DLL` name, you MUST also change this to the exact name your `DLL` target gets, or else `Notepad++` will not recognize it. You'll also need to modify the `` and `` tags there and replace `name="NWScript"` to your `InstalledLexers[]` language name, or else it still won't link properly to `Notepad++` and no custom colors for your plugin. Also the `` attribute obviously points to which file extension your language is to be automatically associated with when opening under `Notepad++` and the `` attribute is what is displayed as the language name for the user when he goes to the `Settings -> Style Configurator` to customize the language colors.

-   To debug, just point the debugger to <kbd>autorun</kbd> `Notepad++.exe` for all supported plataforms (`x86` or `x64`), since this option can't be saved inside the `MSBUILD` files (it's more of an environment configuration for the project).
</details>
<br>	
	
## Development Trivia (aka: more of a blog-like summarized journal - reading is super optional)
<details>
<summary>Click here to expand</summary>
	

### The story from where it began

To tell the truth, I began this project as a self-imposed test. After spending quite some time enjoying community content from [`Neverwinter Vault`](http://neverwintervault.org/), I decided I should also give something back to the community. Add this to a self-motivation to write a nice piece of software in `C++`, something I've never done before. I am a somewhat old of a IT guy. Started programming at 13 in the earlies 1990's and already deeply knew many languages, being the `C language` among them (because I decided to follow the path more of an infrastructure architect rather than a pure programmer I quit programming on 2000's hence never followed the trends properly). Then I braced myself and seeing that all the major Notepad++ plugins used pure C++ and Win32, I decided to roll with that. Then I opened my Visual Studio IDE and started coding... and learning again stuff long forgotten. For me, this so far, is being an interesting experience, but with a lot of pitfalls and caveats. For instance, the LINKER can be tricky to manage. If your dependencies are not very well set, you can end up with missing symbols in your code that are ratherly hard to track, especially for any novice developer. Even if you are already very experient with other environments and languages, things are not so intuitive. Then you'll have to worry if you are linking against the static library, the dynamic library or the static library referencing the dynamic CRT library... The Visual Studio IDE also has it's own issues, like making easy to forget when you are editting your project properties to set configurations to Debug, Release, 32-Bit, 64-Bit... The C++ libraries around I found to be also very dependent on external examples. Lots of auto-generated documentations and many packages there don't come with many usage examples avaliable.

Joined to this, hundreds and hundreds of language peculiarities - memory leaks, access violations, strange and confusing declarations - like pointerofpointer\*\* variable, void (function\*)(arguments), variable&&, variable\*&, etc., and several other features that can quickly render your code mostly *unreadable* if you don't take a very special care with your code styling.

**Aside** from that, I still think it's one of the BEST languages around. Fast, portable, NATIVE (withuot depencencies on virtual machines), and the one that unleashes the FULL potential and control of your hardware and deliver that in your sole hands (this last one can be a very good or a very bad thing).

Between all the helping hands around, I give a special thanks to [The Cherno C++ series](https://www.youtube.com/watch?v=18c3MTX0PK0&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&ab_channel=TheCherno) which helped an old developer a lot, that although had many years of IT experience (I'm actually a professional database architect), would never have touched a `C++` code since about the early 2000s *(yeah, I tried to use `C#` syntax here and as you can presume, sooner than later I was screwing things up really fast - like, using the **`new`** keyword to "instantiate" classes - yeah, you may lol to that 😅 - and doing other things an experienced `C++` programmer would never think of doing with their code)*.

Because of this series, I decided to scratch all I assumed I knew about `C` language and started all over with his series. That changed things really fast - and the catchup wasn't even that big of an effort.

Also, a BIG thanks to the [https://regex101.com/](https://regex101.com/) creators. While dealing with `regular expressions` - something I needed to use to [parse NWScript files](src/NWScriptParser.cpp) for `Notepad++` auto-complete integration, I was severally struggling with [`backtracking`](https://www.regular-expressions.info/catastrophic.html) up until I learned about possessive operators ( `*+`, `++`, `?+` ), atomic groups ( `?>` ) and many other juicy concepts. That was a life-changing experience... So I REALLY advise you before trying to write `regexes`, to do a pause and study the subject deeper first, instead of just copy-pasting code from google searches like I was doing my entire life up to that day... (yeah, never bothered in really learning regex for a long, long time 😔). That website alone solved almost 90% of my problems, and offered a really good debugger, from where I could figure out what EXACTLY was going on when a regular expression was being processed.

Talking about regex, that learning step lead me up to...

### The PCRE2 Engine Saga...

During the `regex` development phase, I first started with [`std::regex`](https://en.cppreference.com/w/cpp/regex) library to parse my strings, since it is in fact THE international `STANDARD` library for doing this; so it **must** be a good, reliable and fast code to build your project upon... right? Until I found out that this engine had severe restrictions and wasn't even compiling expressions with [`named capture groups`](https://www.regular-expressions.info/named.html)... maybe not a big deal for simple `regular expressions` out there but for me, a nuisance to keep changing `matching indexes` everytime an expression was updated to fix a bug or another. Also, I found the execution really slow - it took aprox. **80 seconds** to fully parse a [nwscript.nss definitions file](https://jadeempire-modding.fandom.com/wiki/Nwscript.nss) in `debug mode`. In `release mode` that dropped to **8 seconds**, so even getting rid of every compiler debbuging overhead wasn't helping that much. All of this running in a pretty recent and fast machine setup (won't be spec'ing my setup here, for the sake story simplification).

I was bugged with that, because in an end-user perspective, especially if one used an older CPU, that seemed like my plugin was crashing or not responding, and they could even end up <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Del</kbd> to `task manager kill` the poor `Notepad++` app for that *(and prolly also swearing at me for hanging their machine up)*... so, instead of thinking in just accepting what I had and going ahead adding [`threads`] (https://www.cplusplus.com/reference/thread/thread/) and a possible `% file analysis complete` dialog screen to the file parsing execution, I first decided to test other "alternative" engines... after doing a [web scan](https://www.google.com/search?q=regex+engines+benchmark) on some researches about regular expressions benchmarking, I decided to go with [`boost::regex`](https://www.boost.org/doc/libs/1_78_0/libs/regex/doc/html/index.html), since that's the one being used by `Notepad++` up to now and the one that appeared to have the most compatibility with the code I was alreaady using - just a matter of variable re-declaration and no needed to rewrite any of my already tested routines (the correct name for that inside a class is a `method`, I know... but anyway...).

**Sounded good at first...**

Amazing! Parsing times dropped from **80** to **8** seconds, just by merely <kbd>Ctrl</kbd>+<kbd>H</kbd> replacing my variable declarations from `std::regex` to `boost::regex`. Nothing else changed. And a whooping 10x increase for that! And now it even supports my long sought `named capture groups`, so I didn't need to change indexes anymore! Wow!

But that all changed when I decided to write more robust versions of my `regular expressions`, since they were still unstable, and any malformed file could easily cause many severe [`catastrophic backtrackings`](https://javascript.info/regexp-catastrophic-backtracking), [`stack overflows`](https://en.wikipedia.org/wiki/Stack_overflow) and many other `crashes` inside my code. Not really a fan of too much [`#try-#catch`](https://www.w3schools.com/cpp/cpp_exceptions.asp) blocks of code into my projects here, and also, the user could think this was taking to long... back to the dreaded <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Del</kbd> #issue here *(with the probable **user-swearing** parts and all that stuff)*. Hence, I decided to go back to halt all my other feature developments, go to [regex101](https://regex101.com/), and stay there for an indeterminate amount of time, until my regular expressions were working like a charm to any file I dumped in my application - *well, not `ANY` kind of files like heavly mangled ones and anything severely unrelated to the nwscript language, but anyway... you got the spirit*.

After successfully finishing the expressions, I went back to `Visual Studio` ... just to find out that `boost::regex` did not support [`subroutines`](https://www.regular-expressions.info/subroutine.html), something now crucial for interpreting `object-nestings` and other stuff my new "robust" code was requiring... a quote from [www.regular-expressions.info](https://www.regular-expressions.info/subroutine.html) broke my heart:

> Boost does not support the Ruby syntax for subroutine calls. In Boost `\g<1>` is a backreference---not a subroutine call---to capturing group 1. So `([ab])\g<1>` can match aa and bb but not ab or ba. In Ruby the same regex would match all four strings. No other flavor discussed in this tutorial uses this syntax for backreferences.

**Then, in frustration, I realized I had to change the engine... again.**

So I decided to go back and integrate [`PCRE2`](https://github.com/PhilipHazel/pcre2) into my code, since that was the marked engine I was using while developing at [`Regex101`](https://regex101.com/) anyway. I knew `PCRE2` was not very **`C++`** - friendly, since it's a pure **`C`** implementation of code. So I decided to look for a [`C++ Wrapper`](https://en.wikipedia.org/wiki/Wrapper_library) to help me there, so I would't end up having an indigestable and inelegant code-salad in my project. Fortunately I [`found one`](https://github.com/jpcre2/jpcre2) so I did not have to write it myself. *A relief!* Now I just needed to link with `PCRE2` libraries aaand... ***Whoops!*** those aren't avaliable as a package, just as source code... and this code wasn't even written specifically to build under `Visual Studio` or even [`Windows`](https://www.microsoft.com/windows): the author had it designed in the most generic form possible, so to allow ports to [`POSIX`](https://en.wikipedia.org/wiki/POSIX), [`zOS`](https://en.wikipedia.org/wiki/Z/OS) or any other kind of operating system and anything else capable of chewing on a raw `C-language` `standard` file and spewing out machine code after...

And there I go again, spending a whole day more, studying the [`library documentation`](https://www.pcre.org/current/doc/html/pcre2build.html) trying to figure out how to configure the package to compile under [`VS2022`](https://visualstudio.microsoft.com/vs/community/), which features the author implemented and why... having to write my own [`visual studio configuration file`](https://github.com/Leonard-The-Wise/pcre2/blob/master/vstudio/config/visualstudioconfig.h), dealing in what `Windows` features and functions I had or had not avaliable, the confusing different library flags, like `PCRE2_CODE_UNIT_WIDTH` for different library compilations - must I use just ONE code with for my entire project or can I have them all? Why the author says it also supports a `0` there and says it's "generic", even thought its not compiling? How all of those `functions-types-and-other-stuff` declarations macros are all about, and so forth. And then, even spending a whole night alone just to figure out how to link the library [`statically`](https://en.wikipedia.org/wiki/Static_library) with my project until I found out I had to `#define` `PCRE2_STATIC` also within my project scope, because if I just `#defined` that inside the `LIBRARY` project, and then `#include <pcre2.h>` on my side to use the library, some of the complex `macros` there would lead to many functions being redeclared as `extern __declspec(dllimport)` on MY side, leading my [`linker`](https://www.learncpp.com/cpp-tutorial/introduction-to-the-compiler-linker-and-libraries/) into several [`missing symbols`](http://www.cplusplus.com/forum/general/57873/)! Yeah, that kind of nasty stuff to deal with!. *(And then you can imagine my face when I discovered that ALL of that stuff wasn't even really necessary, because `vcpkg` - something I came to discover only later in my endeavours - already had a `port` of `PCRE2` included, with ALL the configurations requirements already performed by Microsoft's team... 🤦 Anyway... 😊)*.

And then I had to rewrite all my file parsing `routines` (yeah, I know, *`methods`*), since my new `C++ Wrapper` worked differently from the standard ones defined both in `std::regex` and `boost::regex`. *(okay, that last part was a breeze and took the least insignificant amount of time on this whole process)*.

But ALL of that (re)work *DID* pay off when I put my new robust regexes to run inside `PCRE2` engine. It dropped from `boost's` **8** seconds (on debug mode) to an amazing **500ms** parsing time! Yeah, another **16x** gain... but now I know that this is a bit of an unfair comparison with `boost` engine, because now I didn't have the chance to re-test my new regular expressions against `boost` with the new remade syntax and code blocks - like atomic groups, possessive operators and subroutines to avoid as much backtracking as possible - just because `boost` didn't compile my regexes anymore... so I wonder what performance gap this would really be. Anyway...

What I did know then is that now I was able to finally close this `#issue` and go back to coding more features to my plugin peacefully again.

*(and here ends the PCRE2 regex engine saga, if you care to read it, I hope you find at least some useful information there)*.
</details>
<br>
	
## A Final Word

If you read up to here - also passing though the collapsed sections, congrats, you got patience, and that's a virtue! *(Not one of the 8 \*[officially defined virtues](https://wiki.ultimacodex.com/wiki/Eight_Virtues)\*, but anyway)*... 🤴🧘

If you're reading this, I just wish you the best luck in your Neverwinter project (since this plugin is just a helper to Neverwinter content creators, nothing more). As NWN player myself, I always found that the community provided such an amazing ammount of good content and spent years doing so... so that community creators deserved some of my weeks to do a work for them and provide more support and better tools to help them in developing their inventions for the community. Sorry for the timming though... I know the game is out there for a **looong** time, and this plugin just came out now, in 2022. But well, that was because I just "rediscovered" Enhanced Edition and learned all about the [`Vault Community`](https://neverwintervault.org/) a couple of months ago. I never thought a [2002 game](https://en.wikipedia.org/wiki/Neverwinter_Nights) would have so much untapped potential before. So, I think it's better later than never anyway, hehe.

So, to all content developers, this piece of software is made for you, and especially for you. Use it as it best suits you!

As for any *#issues* found, please report them back [`here`](https://github.com/Leonard-The-Wise/NWScript-Npp/issues) on `Github`. I intend to support this project for awhile yet - up to it becoming stable, and while `Notepad++` still keeps it compatible without having to rewrite large portions of code *(unlikely)*.

Best regards,

Leonardo Silva.\
(*aka: Leonard-The-Wise, my [D&D](https://dnd.wizards.com/) chosen [DM](https://en.wikipedia.org/wiki/Dungeon_Master) name*)
