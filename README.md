## Nullboard Agent

This is a repo with the source code for Nullboard Backup Agent - a
small companion utility for [Nullboard](https://github.com/apankrat/nullboard)
that lives in the Windows system tray and acts as a storage
provider for making automatic backups of NB's boards.

See [nullboard.io/backups](https://nullboard.io/backup) for
details.

## Notes

The project file is for Visual Studio 2017, so if you are
on something newer, it will require a conversion.

The UI is implemented using a couple of private UI libraries
that are not the part of this repo. I currently have no plans
to publish these, but their functionality should be pretty
obvious from how they are used in [ui.cpp](src/ui.cpp).

Outside of `ui.cpp` and with few other exceptions the code 
should be reasonably portable. It was certainly written with 
portability in mind, so Windows-specific stuff will usally be
clumped together.

### Execution flow

The entry point is in [entry.cpp](src/entry.cpp). It installs
various safety traps and calls `wmain` from [wmain.cpp](src/wmain.cpp),
which then loads the config, parses the args and spaws the **engine**
and the **UI**, each running in its own thread. It then simply 
loops waiting for either to die.

### The engine

The **engine** implements a simple web server that accepts new
connections, reads inbound requests and answers to Nullboard's 
API calls and browser's CORS requests. See [engine.cpp](src/engine.cpp)
for details.

Web server is quite barebone, but it will fail unsupported and
malformed requests gracefully.

### The UI

The **UI** implements the system tray icon and the "New Backup"
dialog. Again, the bare minimum to get Nullboard hooked up and
backups rolling.

### The asserts

Asserts are used extensively and they are compiled into Release
builds. We really want those invariants to hold, because when
they don't, there's no point in continuing. Release version or 
not. In fact, _especially_ if it's a Release one.

That would be the [`__enforce`](src/enforce.h) macro.

## License

The 2-clause BSD license.

## Feedback

Spot a problem or got a question - phrase it eloquently and
open an issue.
