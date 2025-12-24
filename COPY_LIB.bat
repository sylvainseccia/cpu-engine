copy vs\cpu-engine\*.h lib\include\
del lib\include\stdafx.h
copy vs\x64\Debug\cpu-engine.lib lib\lib\cpu-engine-debug.lib
copy vs\x64\Release\cpu-engine.lib lib\lib\
