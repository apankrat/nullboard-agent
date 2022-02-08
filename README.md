# Nullboard Agent

This is a repo with the source code for Nullboard Backup Agent
- a small companion utility for
[Nullboard](https://nullboard.io/preview)
that lives in the Windows system tray and acts as a storage
provider for making automatic backups of NB's boards.

See [nullboard.io/backups](https://nullboard.io/backup) for
details.

# Notes

Solution/project file is for Visual Studio 2017, so if you are
on something newer, it will require a conversion.

The UI is implemented using a couple of private UI libraries
that are not the part of this repo. I currently have no plans
(read - no time) to publish these, but their functionality
should be pretty obvious from how they are used in `ui.cpp`.

Outside of `ui.cpp` and few other exceptions the code should
be reasonably portable, and it was certainly written with 
portability in mind. Some shortcuts here and there, but these
should be easy to patch.

# Execution flow and

The entry point is in `entry.cpp`. It installs various safety
traps and calls `wmain` from `wmain.cpp`.

`wmain` loads the config, parses the args and spaws the engine
and the UI, each in its own thread. It then simply loops waiting
for either to die.

The *engine* implements a simple web server that accepts new
connections, reads the requests and answers to Nullboard's API
calls and browser's CORS requests. See code for details.

Web server is quite barebone, but it will fail unsupported and
malformed requests gracefully.

# Asserts

Asserts are used extensively and they are compiled into Release
builds. We really want those invariants to hold, because when
they don't, there's no point in sweeping continuing. Release
version or not. In fact, _especially_ if it's a Release one.

That would be the `__enforce` macro.

# License

The 2-clause BSD license.

