
HELPER FILES FOR POPULAR DEBUGGERS

vangui.gdb
    GDB: disable stepping into trivial functions.
    (read comments inside file for details)

vangui.natstepfilter
    Visual Studio Debugger: disable stepping into trivial functions.
    (read comments inside file for details)

vangui.natvis
    Visual Studio Debugger: describe VanGUI types for better display.
    With this, types like VanVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)

vangui_lldb.py
    LLDB-based debuggers (*): synthetic children provider and summaries for VanGUI types.
    With this, types like VanVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)
    (*) Xcode, Android Studio, may be used from VS Code, C++Builder, CLion, Eclipse etc.
