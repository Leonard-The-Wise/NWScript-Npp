![Notepad++ Plugin](https://img.shields.io/badge/Notepad++-Plugin-green.svg?&logo=notepad%2B%2B)
![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-blue?logo=visual-studio)
![C++](https://img.shields.io/badge/c++-red.svg?&logo=c%2B%2B)

![GPL-3.0 License](https://img.shields.io/badge/License-GPL%20v3-green)
 
 # Bioware's NWScript Language support and compiler for Notepad++.

Since there's not much functionality to User-Defined Languages in [Notepad++](https://notepad-plus-plus.org/) I decided to create a custom Lexer to support Bioware's NWScript. A [Lexer](https://en.wikipedia.org/wiki/Lexical_analysis) is just a program that can read, interpret and highlight syntax for code. 

But in this plugin, things rapidly expanded, so I also decided to provide the tools to compile and publish NWScript files directly into the target games: Neverwinter Nights [Enhanced](https://www.beamdog.com/games/neverwinter-nights-enhanced/) and [2] (this one still a Work In Progress).

The plugin adds support for function Auto-Completion and can import newly defined functions from NWScripts.nss (the main Neverwinter engine header).


## Installation
Just download the latest release and put it into Notepad++ plugins directory, overwritting any existing file or folder.
- The plugin requires it's XML to be properly placed on the **plugins/Config/** folder in Notepad++ in order for it to work, or else it will be considered "incompatible".

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
- To use auto-indentation, you must first disable Notepad++'s Auto-Indentation function:
```
Settings -> Auto-Completion -> Auto-Indent (checkbox)
```
- Also while playing back a macro, especially the type-in ones, you'll want to disable the Plugin's auto-indent feature, as the plugin cannot currently detect a macro playback and will end-up messing with your macro.

_**[ Note: These are not Plugin bugs, but technical limitations due to how Notepad++ exposes it's contents and functions to plugins. I am trying to work with Notepad++ team to solve these issues in the future, so stay tuned. ]**_

## For Plugin Developers

This plugin is based on [Notepad++ plugin template](https://github.com/npp-plugins/plugintemplate) and the official [Scintilla](https://www.scintilla.org/) C++ Lexer, thanks for the community to share some amazing piece of code! Also, for the NWScript compilation, I "borrowed" the 

This plugin was a test to check my C++ skills, also an example on how to write a REAL plugin since neither of Notepad++ or Scintilla documentation provide much information on how to create a [lexer plugin](https://npp-user-manual.org/docs/plugin-communication/) - let alone an external one!

I managed to rewrite much of the plugin code, clear and organize classes, so anyone desiring to write future lexers will find it much easier to integrate a new lexer inside the Plugin. Just put your LexXXX.Cxx file on the project and add it to the [Lexer Catalogue](src/Lexers/LexerCatalogue.cxx)

All files under this project are provided under the [GNU License](license.txt).

For reutilization of the project, the `NWScript-Npp.vcxproj` is organized in the following way:

- **Plugin Interface**: Contains all code necessary to initialize the DLL and communicate with Notepad++ Plugins interface, including the Lexer part. You Probably won't need to change this code.
- **Resource Files**: Contains the [XML](src/Lexers/Config/NWScript-Npp.xml) necessary for the Lexer to work with Notepad++ (without it, Notepad++ will just mark your plugin as incompatible). It will be copied to the notepad/plugin/Config folder.
	* Also contains a [.targets](Publish.Dll.To.Notepad.targets) file that is imported inside the [vcxproj](NWScript-Npp/NWScript-Npp.vcxproj) to automate deployment of the plugin DLL and associated XML to Notepad++ to help debugging. Make sure Notepad++ isn't running while you build your code.
- **Utils**: Contains utilitary headers and code to help dealing with settings, INI files, etc.

- **Custom Lexers**: Here you'll write your new custom Lexer file (example: [LexNWScript.cxx](src/Lexers/LexNWScript.cxx) and edit [LexerCatalogue.cxx](src/Lexers/LexerCatalogue.cxx) for the code to auto-initialize it upon plugin load.
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
