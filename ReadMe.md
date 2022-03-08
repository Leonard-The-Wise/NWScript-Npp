![Notepad++ Plugin](https://img.shields.io/badge/Notepad++-Plugin-green.svg?&logo=notepad%2B%2B)
![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-blue?logo=visual-studio)
![C++](https://img.shields.io/badge/c++-red.svg?&logo=c%2B%2B)

![GPL-3.0 License](https://img.shields.io/badge/License-GPL%20v3-green)
 
 # NWScript Tools for Notepad++.

Since there's not much functionality to User-Defined Languages in [Notepad++](https://notepad-plus-plus.org/) I decided to create a custom Lexer to support Bioware's NWScript language. A [Lexer](https://en.wikipedia.org/wiki/Lexical_analysis) is just a program that can read, interpret and highlight syntax for code. 

But in this plugin, things rapidly expanded, so I also decided to provide the tools to compile and publish NWScript files directly into the target games: **Neverwinter Nights [Enhanced](https://www.beamdog.com/games/neverwinter-nights-enhanced/)** and **[2](https://dnd.wizards.com/products/digital-games/pcmac/neverwinter-nights-2-complete)** (this one still a Work In Progress).

The plugin adds support for function Auto-Completion and can import newly defined functions from NWScripts.nss (the main Neverwinter engine header).


## Installation
Just download the latest release and put it into Notepad++ plugins directory, overwritting any existing file or folder.
- The plugin requires it's XML to be properly placed on the **`plugins/Config/`** folder in Notepad++ in order for it to work, or else it will be considered "incompatible".

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

- To use the plugign built-in auto-indentation, you must first disable Notepad++'s Auto-Indentation function:
```
Settings -> Auto-Completion -> Auto-Indent (checkbox)
```
- Also while playing back a macro, especially the type-in ones, you'll want to disable the Plugin's auto-indent feature, as the plugin cannot currently detect a macro playback and will end-up messing any text typed within your macro.

***`From Notepad++ version 8.3.3 and beyond`***

- Those issues were fixed *(thanks [@DonHo](https://github.com/donho) for accepting my pull request)* and no longer a concern, the option to select the plugin built-in auto-indentation won't even show up anymore. So I suggest you to always keep your Notepad++ version up-to-date.
(if not possible because you use legacy plugins, well, just use the built-in auto-indent function then, it won't bite too)

## For Plugin Developers
<details><summary>Click me to expand</summary>
This plugin is based on [Notepad++ plugin template](https://github.com/npp-plugins/plugintemplate) and the official [Scintilla](https://www.scintilla.org/) C++ Lexer. I managed to rewrite much of the plugin code, clear and organize classes, so anyone desiring to write future lexers will find it much easier to integrate a new lexer inside the Plugin. Just put your LexXXX.cpp file on the project and add it to the [Lexer Catalogue](src/Lexers/LexerCatalogue.cpp)

Also, for the NWScript compilation, I *"borrowed"* the [nwnsc code](https://github.com/Leonard-The-Wise/nwnsc), since trying to write a compiler from scratch would a monstrous task.

As for the [`PCRE2`](PCRE/) folder up there... this is because during development and testing, I found out that this is the best regex engine out there, far superseding `std::regex` library, and (even the boost version). At least for the purpose of this project. See the [`development trivia`](#trivia) section down bellow for more info.

All files under this project are provided under the [GPL v3.0 License](license.txt).

For reutilization of the project, the `NWScript-Npp.vcxproj` is organized in the following way:

- **Plugin Interface**: Contains all code necessary to initialize the DLL and communicate with Notepad++ Plugins interface, including the Lexer part. You Probably won't need to change this code.
- **Resource Files**: Contains the [XML](src/Lexers/Config/NWScript-Npp.xml) necessary for the Lexer to work with Notepad++ (without it, Notepad++ will just mark your plugin as incompatible). It will be copied to the notepad/plugin/Config folder.
	* Also contains a [.targets](Publish.Dll.To.Notepad.targets) file that is imported inside the [vcxproj](NWScript-Npp/NWScript-Npp.vcxproj) to automate deployment of the plugin DLL and associated XML to Notepad++ to help debugging. Make sure Notepad++ isn't running while you build your code.
- **Utils**: Contains utilitary headers and code to help dealing with settings, INI files, etc.

- **Custom Lexers**: Here you'll write your new custom Lexer file (example: [LexNWScript.cpp](src/Lexers/LexNWScript.cpp) and edit [LexerCatalogue.cpp](src/Lexers/LexerCatalogue.cpp) for the code to auto-initialize it upon plugin load.
- **PluginMain**: This is where your Plugin code begins. I created a base class to setup Plugin Menu Commands, to deal with message processing, etc. here. Change this and add more code as suitable.

### Remarks
`NWScript-Npp.vcxproj` file sets  `<PlatformToolset>` = v143 for Visual Studio 2022.

Also, we target ISO C++ 20 Language Standard.

Interface functions required for NPP to use the lexer are declared with...
`extern "C" __declspec(dllexport) ... __stdcall`.

`src\Lexers\Scintilla` is unmodified files copied from [NPP\Scintilla\include](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/scintilla/include)

`src\Lexlib` contains required files copied from [NPP\Scintilla\lexlib](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/scintilla/lexlib) - unchanged other than ripping out some headers that were not required. You can add more if your project needs.

`src\Lexers\Config\NWScript-Npp.xml` defines the language keywords & styles. Required for the plugin and will be published on project build.

The Debugger is already set to autorun Notepad++.exe for all plataforms.
	</details>
## <a name="trivia"></a>Some development trivia

<details><summary>Click me to expand</summary>

- This plugin was actually a self-imposed test, to write a nice piece of software in C++, something I've never done before. So far, I enjoyable experience, although with some severe caveats and dreadful cryptical errors - like linkage missing symbols, a LOT of different compiling warnings, unexpected code behavior, the mind-boggling pointer and reference usage that can get really complex and really messed up really fast... and a world of language differences and different standards and issues that other more "high level" syntax-siblings like Java, Javscript and even C# wouldn't experience. But aside from this, C++ gave me a view of internal machine workings that no other language ever gave me before... and I'm glad for that.

So, I thanks the community for sharing some amazing piece of code! Not to mention all the entire references from StackOverflow(https://stackoverflow.com/) and other online helpers without whom, this work would **NEVER** be feasible!

A special mention to [The Cherno C++ series](https://www.youtube.com/watch?v=18c3MTX0PK0&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&ab_channel=TheCherno) which helped an old developer a lot, that alghough had many years of IT experience (I'm actually a professional database architect), would never have touched a C++ code since about the early 2000s *(yeah, I tried to use C# syntax here and sooner than later you can presume I was screwing things up really fast - like, using **new** keyword to "instantiate" classes... lol 😅)*. Then I decided to scratch all I presumed I knew about C++ language and started all over with his series. Yeah, that changed things really fast!

- Also, while dealing with regular expressions - something I needed to use to [parse NWScript files](src/NWScriptParser.cpp) for Notepad++ auto-complete integration, I was severally struggling with [backtracking](https://www.regular-expressions.info/catastrophic.html) up until I learned about possessive operators (\*+, ++, ?+) and atomic groups (?>). That was a life-changing experience... So I advise you before trying to write regexes, I REALLY recommend to study the subject first, instead of just copy-pasting code from google searches like I was doing my entire life up to that day... (yeah, never bothered in really learning regex for a long time :/). And that lead me up to...

### The PCRE2 regex engine saga...

Well, during the regex development phase, I first started with [`std::regex`](https://en.cppreference.com/w/cpp/regex) library to parse my strings, since it is in fact a international STANDARD definitions, so it must be a good, reliable and fast code, right? Yeah... until I found out that this engine had severe restrictions and wasn't even compiling expressions with named capture subgroups... a nuisance to keep changing matching indexes everytime an expression was updated to fix a bug or another... Also, I found the execution was really slow - It took aprox. 80 seconds to fully parse a [nwscript.nss definitions file](https://jadeempire-modding.fandom.com/wiki/Nwscript.nss) in debug mode - in release mode that dropped to 8 seconds, so even getting rid of every debug overhead wasn't helping that much. 

I was bugged with that, because in an end-user perspective that seemed like my plugin was crashing, and they could even end up doing `ctrl+alt+del` to task manager - kill Notepad++... so, instead of thinking up in going ahead and adding multithreading and a "% file analysis complete" dialog screen to the execution, I first decided to test other engine alternatives... after some research on regex benchmarking throughout the web, I decided to go with [`boost::regex`](https://www.boost.org/doc/libs/1_78_0/libs/regex/doc/html/index.html), since that's the one being used by Notepad++ up to now and the one that appeared to have the most compatibility with the code I was alreaady using - just a matter of variable re-declaration and no needed to rewrite any of my already tested routines. Sounded good.

Amazing... the parsing time dropped from 80 seconds to 8, just by merely replacing the regex variable declarations from `std::regex` namespace to `boost::regex`. A whooping 10x increase, and now it even supports named capture groups so I didn't need to change indexes anymore! Hurray!

But that all changed when I decided to write more robust versions of my regular expressions, since any malformed file was easily causing backtrackings, stack overflows and crashing my code... hence I decided to go back to [regex101](https://regex101.com/), halt my other feature developments and stay there for an indeterminate amount of time, until the regular expressions were working like a charm to any file I dumped there. After successfully finishing the expressions, I went back to Visual Studio... just to find out that `boost::regex` did not support [subroutines](https://www.regular-expressions.info/subroutine.html), something crucial for interpreting object-nestings and other stuff my now-robust code was requiring... a quote from www.regular-expressions.info:

> Boost does not support the Ruby syntax for subroutine calls. In Boost `\g<1>` is a backreference—not a subroutine call—to capturing group 1. So `(\[ab\])\g<1>` can match aa and bb but not ab or ba. In Ruby the same regex would match all four strings. No other flavor discussed in this tutorial uses this syntax for backreferences.

Then again, in frustration, I needed to change the engine...

So I decided to go back and integrate [`PCRE2`](https://github.com/PhilipHazel/pcre2) into my code, since that was the marked engine I was using while developing at `Regex101` anyway. I knew PCRE2 was not **C++** - friendly, since it's a pure **C** implementation, then I decided to use a [wrapper](https://github.com/jpcre2/jpcre2) here, so I would not have an indigestable code-salad in my project. Now I just need to link with PCRE2 libraries... whoops! those aren't avaliable as a package, just as source code... and this code wasn't even written specifically to build under Visual Studio - the author had it written in the most generic form possible, to allow ports to Linux, Solaris, and even zOS specs... 

There I go again, spending a couple more days trying to [figure out](https://www.pcre.org/current/doc/html/pcre2build.html) how to configure the package to compile under VS2022... having to write a full [visual studio configuration file](PCRE2/vstudio/config/visualstudioconfig.h), dealing in what windows had or had not avaliable, the confusing different flags, like `PCRE2_CODE_UNIT_WIDTH` for different library compilations, and even spending a night trying to link statically with the library until I figured I had to `#define PCRE2_STATIC` within my project scope, because if i just `#include <pcre2.h>` on the project, that will lead to functions being redeclared by that header as `extern __declspec(dllimport)`, causing my linkage with an already compiled library to lead to missing symbols... Jesus! (didn't understand? Don't worry, you won't have to, until you try to use the library yourself, hehe).

And then I had to rewrite all my file parsing routines, since the new wrapper worked differently from the standard ones... okay, that took the least time of this whole process.

But ALL of that work paid off when I put my new robust regexes to run inside PCRE2 engine. It dropped from `boost`'s 8 seconds (on debug mode) to an amazing 500ms parsing time! Yeah, another 16x gain... but that is a bit of injustice with `boost` engine, because I didn't have the chance to test my new regular expressions against `boost` with the new remade syntax and code blocks - like atomic groups, possessive operators and subroutines to avoid as much backtracking as possible - just because `boost` didn't compile my regexes anymore... so I wonder what performance gap this would really be. Anyway... 

What I did know was that now I was able to close this issue and go back to adding features to my plugin again.

*(and here ends the PCRE2 engine saga)*

</details>

## A final word

Well, if you read up to here - also passing though the collapsed sections, congrats, you got patience, and that's a virtue. :)

But anyway, if you're reading this, I just wish you the best luck in your Neverwinter project (since this plugin is just a helper to Neverwinter content creators, nothing more). As a Neverwinter Nights player and a GM myself, I always found that the community provided such an amazing ammount of good content, so that community creators deserved even more support and better tools to help them in developing their inventions for the community. 

So, this software is made for you, and especially for you alone. Use it as it best suits you!

As for any #issue found, please report it back [here](https://github.com/Leonard-The-Wise/NWScript-Npp/issues) on github. I intend to support this project for at least the next couple years ahead of this date (Mar-07-2022), and while Notepad++ still keeps it compatible without having to rewrite most part of the code.


Best regards,

Leonardo Silva
*(aka: Leonard-The-Wise, my [D&D](https://dnd.wizards.com/) chosen [DM](https://en.wikipedia.org/wiki/Dungeon_Master) name)*
