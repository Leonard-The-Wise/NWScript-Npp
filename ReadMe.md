![Notepad++ Plugin](https://img.shields.io/badge/Notepad++-Plugin-green.svg?&logo=notepad%2B%2B)
![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-blue?logo=visual-studio)
![C++](https://img.shields.io/badge/c++-red.svg?&logo=c%2B%2B)

![GPL-3.0 License](https://img.shields.io/badge/License-GPL%20v3-green)
 
 # NWScript Tools for Notepad++

Since there's not much functionality to User-Defined Languages in [`Notepad++`](https://notepad-plus-plus.org/) I decided to create a custom Lexer to support [`Bioware's NWScript language`](https://en.wikipedia.org/wiki/NWScript). A [`Lexer`](https://en.wikipedia.org/wiki/Lexical_analysis) is just a program that can read, interpret and highlight syntax for code. 

But in this plugin, things rapidly expanded, so I also decided to provide the tools to compile and publish NWScript files directly into the target games: **`Neverwinter Nights` [`Enhanced`](https://www.beamdog.com/games/neverwinter-nights-enhanced/)** and **[`2`](https://dnd.wizards.com/products/digital-games/pcmac/neverwinter-nights-2-complete)** (this one still a Work In Progress).

The plugin adds support for function Auto-Completion and can import newly defined functions from NWScripts.nss (the main Neverwinter engine header).


## Installation
Just download the latest release and put it into `Notepad++` `plugins directory`, overwritting any existing file or folder.
- The plugin requires it's XML to be properly placed on the **`plugins/Config/`** folder in `Notepad++` in order for it to work, or else it will be considered **“incompatible”** as and asked to uninstall.

```
Here is a sample of the plugin's funcionality:
```
<div align="center"><img src="Media/Sample-Colorization.jpg" width="285"/></div>
<p></p>
<p></p>

```
Dark Mode version:
```
<div align="center"><img src="Media/Sample-Colorization-DarkMode.jpg" width="285"/></div>
<p></p>


The unity test file I used to take this screenshot is provided [here](Media/UnityTest.nss).

## Known Issues
***`Up to Notepad++ version 8.3.2`***: 

- To use the plugign built-in auto-indentation, you must first disable `Notepad++`'s Auto-Indentation function:
```
Settings -> Auto-Completion -> Auto-Indent (checkbox)
```
- Also while playing back a macro, especially the type-in ones, you'll want to disable the Plugin's auto-indent feature, as the plugin cannot currently detect a macro playback and will end-up messing any text typed within your macro.

***`From Notepad++ version 8.3.3 and beyond`***

- Those issues were fixed *(thanks [`@DonHo`](https://github.com/donho) for accepting my pull request)* and no longer a concern, the option to select the plugin built-in auto-indentation won't even show up as an option to it's users anymore. So I suggest you to always keep your `Notepad++` version up-to-date.
*(if not possible because you use legacy plugins, well, just use the built-in auto-indent function then, it won't bite too)*.

## For Plugin Developers
<details><summary>Click here to expand</summary>
	
This plugin is based on [Notepad++ plugin template](https://github.com/npp-plugins/plugintemplate) and the official [Scintilla](https://www.scintilla.org/) C++ Lexer. I managed to rewrite much of the code, clear and organize classes, so anyone desiring to write future lexers will find it much easier to integrate a new lexer inside the Plugin. Just put your LexXXX.cpp file on the project and add it to the [Lexer Catalogue](src/Lexers/LexerCatalogue.cpp) and export it as a DLL.

Also, for the NWScript compilation, I *“borrowed”* the [`nwnsc` code](https://github.com/Leonard-The-Wise/nwnsc), since trying to write a compiler from scratch would a monstrous task.

As for the [`PCRE2`](PCRE/) folder up there... this is because during development and testing, I found out that this is the best regex engine out there, far superseding `std::regex` library, and (even the boost version). At least for the purpose of this project. See the [`development trivia`](#trivia) section down bellow for more info.

All files under this project are provided under the [GPL v3.0 License](license.txt).

For reutilization of the project, the `NWScript-Npp.vcxproj` is organized in the following way:

- **`Custom Lexers`**: Here you'll write your new custom Lexers (example: [LexNWScript.cpp](src/Lexers/LexNWScript.cpp) and edit [LexerCatalogue.cpp](src/Lexers/LexerCatalogue.cpp) for the code to auto-initialize it upon plugin load.

- **`Notepad Controls`**: Contains some class templates to display dialog boxes: versions of Static, Modal and Dockable dialogs are avaliable.

- **`Plugin Interface`**: Contains all code necessary to initialize the DLL and communicate with `Notepad++` Plugins interface, including the Lexer part. You Probably won't need to change much of this code, **EXCEPT** to make it point to YOUR plugin main class instead of mine's.

- **`Resource Files`**: Contains the [XML](src/Lexers/Config/NWScript-Npp.xml) necessary for the Lexer to work with `Notepad++` (without it, `Notepad++` will just mark your plugin as incompatible). It will be copied to the notepad/plugin/Config folder.
     * Also contains a [.targets](Publish.Dll.To.Notepad.targets) file that is imported inside the [vcxproj](NWScript-Npp/NWScript-Npp.vcxproj) to automate deployment of the plugin DLL and associated XML to `Notepad++` to help with plugin debugging. Make sure `Notepad++` isn't running when you build your code. Also make sure to give yourself **write permissions** to the Notepad/plugin installation folder and subfolders, so the compiler can copy the output DLL and the anexed XML styler to that path. You'll be notified if it cannot and also the build will fail and the debugger will not run if it can't copy at least the DLL there (the XML deploying is optional and only emmits warnings).
     * Also, I've setup a [`ProjectVersion.rc`](src/ProjectVersion.rc) file along with a header called [`ProjectVersion.h`](src/ProjectVersion.h) to perform auto-increments  on the `VS_VERSION_INFO` associated resource. This works as following:
         * Every time you hit `build` command, a [pre-build event](https://docs.microsoft.com/en-us/visualstudio/ide/specifying-custom-build-events-in-visual-studio?view=vs-2022) occurs, which calls [this](IncrementBuild.ps1) `PowerShell` script on the project root that will edit `ProjectVersion.h` and increment the `VERSION_BUILD` macro inside that file.
         * Then the pre-compiler will read that macro and since VS_VERSION_INFO is setup to use macros for replacing version information, it will compile with whichever version is printed on ProjectVersion.h at the time of compilation.
         * Hence I advise you to ***`NEVER`*** touch or edit `ProjectVersion.rc` inside the Resource Editor, or it will overwrite the macros inside and cause you to lose the auto-increment funcionality. Edit it manually (inside any raw notepad app) only to change other info, like DLL name, company name, copyright info, etc and leave all the macros there about versioning untouched.
         * To increment major, minor or patch numbers, edit the `ProjectVersion.h` file instead. Only build numbers are setup to auto-increment on my script, so if you want your major, minor or patch versions to change, you'll have do it manually. I designed this intentionally, since every person or team have its own standards for managing project versions.
	
- **`Utils`**: Contains utilitary headers and code to help dealing with settings, `.ini` files, `regular expressions`, etc.

- **`Root Directory`**: This is where the Plugin code really begins. I created a base class called [`PluginMain`](src/PluginMain.h) to setup the Menu Commands, to deal with message processing, and all of the main plugin funcions. You'll need to change this as suitable. Perhaps in the future I'll clean up the code from my specific usage and leave a framework for others to developed upon. No promises made, though **(and hey, it's easy to delete a `PluginMain.cpp` and add your own class... just don't forget to update `PluginInterface.cpp` to point to your own classes instead of mine for handling plugin initialization, message parsing, etc)**.
   * Also, since many plugins use `.ini` files to store their settings, I already provided a `Settings.cpp` class that will do that (almost) automatically for you. Just replace my variables with yours, update the `Save()` and `Load()` functions to save/load your variables instead and you're done. The Settings class uses a modified version of [`MiniINI`](https://github.com/pulzed/mINI/blob/master/src/mini/ini.h) code to handle file reading, writting, etc., so it's really simple to use instead of writting your own version. It supports ASCII and UNICODE files and filenames, being them ANSI or UTF-8/16 codified.
	
- **Last** but not least: `Plugin Dialogs` are just the instanced versions of `Notepad Controls` classes, to manage MY specific dialog boxes, etc. You really don't need these, except if you want to use them as examples.
	
```
All other files on this project are just internal workings and the main plugin funcionality, and so I will not be providing too much information on them here. I consider the code reasonably documented already anyway, so feel free to explore it.
```
	
### Remarks
`NWScript-Npp.vcxproj` file sets  `<PlatformToolset>` = v143 for Visual Studio 2022.

Also, we target ISO C++ 20 Language Standard.

Interface functions required for NPP to use the lexer are declared with...
`extern "C" __declspec(dllexport) ... __stdcall`.

`src\Lexers\Scintilla` is unmodified files copied from [NPP\Scintilla\include](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/scintilla/include), so you can overwrite those with more up-to-date versions in your own taste.

`src\Lexlib` contains required files copied from [NPP\Scintilla\lexlib](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/scintilla/lexlib) - unchanged other than ripping out some headers that were not required. You can add more if your project needs.

`src\Lexers\Config\NWScript-Npp.xml` defines the language keywords & styles. Required for the plugin and will be published on project build. When changing the DLL name, you also MUST change this to the exact name your DLL gets, or else Notepad++ will not recognize it.

The Debugger is already set to autorun `Notepad++.exe` for all supported plataforms (x86 or x64).
	
</details>
	
## <a name="trivia"></a>Some development trivia
<details><summary>Click here to expand</summary>

- This plugin was actually a self-imposed test, to write a nice piece of software in `C++`, something I've never done before. So far, I enjoyable experience, although with some severe caveats and dreadful cryptical errors - like linkage missing symbols, a LOT of different compiling warnings, unexpected code behavior, the mind-boggling pointer and reference usage that can get really complex and really messed up really fast... and a world of language differences and different standards and issues that other more “high level” syntax-siblings like `Java`, `Javscript` and even `C#` wouldn't experience. But aside from this, `C++` gave me a view of internal machine workings that no other language ever gave me before... and I'm glad for that.

So, I thanks the community for sharing some amazing piece of code! Not to mention all the entire references from StackOverflow(https://stackoverflow.com/) and other online helpers without whom, this work would **NEVER** be feasible!

A special mention to [The Cherno C++ series](https://www.youtube.com/watch?v=18c3MTX0PK0&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&ab_channel=TheCherno) which helped an old developer a lot, that alghough had many years of IT experience (I'm actually a professional database architect), would never have touched a `C++` code since about the early 2000s *(yeah, I tried to use `C#` syntax here and sooner than later you can presume I was screwing things up really fast - like, using **new** keyword to “instantiate” classes... yeah, lol to that 😅)*. Then I decided to scratch all I presumed I knew about `C++` language and started all over with his series. Yeah, that changed things really fast!

- Also, while dealing with regular expressions - something I needed to use to [parse NWScript files](src/NWScriptParser.cpp) for `Notepad++` auto-complete integration, I was severally struggling with [backtracking](https://www.regular-expressions.info/catastrophic.html) up until I learned about possessive operators (\*+, ++, ?+) and atomic groups (?>). That was a life-changing experience... So I advise you before trying to write regexes, I REALLY recommend to study the subject first, instead of just copy-pasting code from google searches like I was doing my entire life up to that day... (yeah, never bothered in really learning regex for a long time :/). And that lead me up to...

### The PCRE2 regex engine saga...

Well, during the regex development phase, I first started with [`std::regex`](https://en.cppreference.com/w/cpp/regex) library to parse my strings, since it is in fact a international STANDARD definitions, so it must be a good, reliable and fast code, right? Yeah... until I found out that this engine had severe restrictions and wasn't even compiling expressions with named capture subgroups... a nuisance to keep changing matching indexes everytime an expression was updated to fix a bug or another... Also, I found the execution was really slow - It took aprox. 80 seconds to fully parse a [nwscript.nss definitions file](https://jadeempire-modding.fandom.com/wiki/Nwscript.nss) in debug mode - in release mode that dropped to 8 seconds, so even getting rid of every debug overhead wasn't helping that much. 

I was bugged with that, because in an end-user perspective that seemed like my plugin was crashing, and they could even end up <kbd>ctrl</kbd>+<kbd>alt</kbd>+<kbd>del</kbd> to `task manager kill` `Notepad++`... so, instead of thinking up in going ahead and adding multithreading and a `% file analysis complete` dialog screen to the execution, I first decided to test other engine alternatives... after some research on regex benchmarking throughout the web, I decided to go with [`boost::regex`](https://www.boost.org/doc/libs/1_78_0/libs/regex/doc/html/index.html), since that's the one being used by `Notepad++` up to now and the one that appeared to have the most compatibility with the code I was alreaady using - just a matter of variable re-declaration and no needed to rewrite any of my already tested routines. Sounded good.

Amazing... the parsing time dropped from 80 seconds to 8, just by merely replacing the regex variable declarations from `std::regex` namespace to `boost::regex`. A whooping 10x increase, and now it even supports named capture groups so I didn't need to change indexes anymore! Hurray!

But that all changed when I decided to write more robust versions of my regular expressions, since any malformed file was easily causing backtrackings, stack overflows and crashing my code... hence I decided to go back to [regex101](https://regex101.com/), halt my other feature developments and stay there for an indeterminate amount of time, until the regular expressions were working like a charm to any file I dumped there. After successfully finishing the expressions, I went back to Visual Studio... just to find out that `boost::regex` did not support [subroutines](https://www.regular-expressions.info/subroutine.html), something crucial for interpreting object-nestings and other stuff my now-robust code was requiring... a quote from [www.regular-expressions.info](https://www.regular-expressions.info/subroutine.html):

> Boost does not support the Ruby syntax for subroutine calls. In Boost `\g<1>` is a backreference—not a subroutine call—to capturing group 1. So `([ab])\g<1>` can match aa and bb but not ab or ba. In Ruby the same regex would match all four strings. No other flavor discussed in this tutorial uses this syntax for backreferences.

Then again, in frustration, I needed to change the engine...

So I decided to go back and integrate [`PCRE2`](https://github.com/PhilipHazel/pcre2) into my code, since that was the marked engine I was using while developing at [`Regex101`](https://regex101.com/) anyway. I knew `PCRE2` was not **`C++`** - friendly, since it's a pure **`C`** implementation, then I decided to use a [`wrapper`](https://github.com/jpcre2/jpcre2) here, so I would not have an indigestable code-salad in my project. Now I just need to link with `PCRE2` libraries... whoops! those aren't avaliable as a package, just as source code... and this code wasn't even written specifically to build under Visual Studio - the author had it written in the most generic form possible, to allow ports to Linux, Solaris, and even zOS specs... 

There I go again, spending a couple more days trying to [figure out](https://www.pcre.org/current/doc/html/pcre2build.html) how to configure the package to compile under [`VS2022`](https://visualstudio.microsoft.com/vs/community/)... having to write a full [visual studio configuration file](PCRE2/vstudio/config/visualstudioconfig.h), dealing in what windows had or had not avaliable, the confusing different flags, like `PCRE2_CODE_UNIT_WIDTH` for different library compilations, and even spending a night figuring out how to link statically with the library until I finding out I had to `#define PCRE2_STATIC` also within my project scope, because if i just `#include <pcre2.h>` on the project to use the library, that would lead to functions being redeclared by that header as `extern __declspec(dllimport)`, causing build linkage missing symbols... Jesus! (didn't understand? Don't worry, you don't have to, until you try to use the library yourself, hehe 😉).

And then I had to rewrite all my file parsing routines, since the new wrapper worked differently from the standard ones. Okay, that last part took the least amount of time among this whole process.

But ALL of that work paid off when I put my new robust regexes to run inside `PCRE2` engine. It dropped from `boost`'s 8 seconds (on debug mode) to an amazing 500ms parsing time! Yeah, another 16x gain... but I know that is a bit of an unfair comparison with `boost` engine, because now I didn't have the chance to re-test my new regular expressions against `boost` with the new remade syntax and code blocks - like atomic groups, possessive operators and subroutines to avoid as much backtracking as possible - just because `boost` didn't compile my regexes anymore... so I wonder what performance gap this would really be. Anyway... 

What I did know thera was that now I was able to close this issue and go back to adding features to my plugin again.

*(and here ends the PCRE2 engine saga)*
	
</details>
	
## A final word

If you read up to here - also passing though the collapsed sections, congrats, you got patience, and that's a virtue. :)

But anyway, if you're reading this, I just wish you the best luck in your Neverwinter project (since this plugin is just a helper to Neverwinter content creators, nothing more). As NWN player myself, I always found that the community provided such an amazing ammount of good content and spent years doing so... so that community creators deserved some of my weeks to do a work for them and provide more support and better tools to help them in developing their inventions for the community. Sorry for the timming though... I know the game is out there for a **looong** time, and this plugin just came out now, in 2022. But well, that was because I just “rediscovered” Enhanced Edition and learned all about the [`Vault Community`](https://neverwintervault.org/) a couple of months ago. I never thought a [2002 game](https://en.wikipedia.org/wiki/Neverwinter_Nights) would have so much untapped potential before. So, I think it's better later than never anyway, hehe.

So, to all content developers, this piece of software is made for you, and especially for you. Use it as it best suits you!

As for any *#issues* found, please report them back [here](https://github.com/Leonard-The-Wise/NWScript-Npp/issues) on github. I intend to support this project for at least the next couple years ahead of this date (Mar-2022), and while `Notepad++` still keeps it compatible without having to rewrite large portions of code (unlikely).

Best regards,

Leonardo Silva.<br>
(*aka: Leonard-The-Wise, my [D&D](https://dnd.wizards.com/) chosen [DM](https://en.wikipedia.org/wiki/Dungeon_Master) name*)
