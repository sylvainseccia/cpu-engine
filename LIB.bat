rmdir /s /q "cpulib"
mkdir cpulib
mkdir cpulib\include
mkdir cpulib\include\cpu-core
mkdir cpulib\include\cpu-render
mkdir cpulib\include\cpu-engine
mkdir cpulib\lib
copy vs\cpu-core\*.h cpulib\include\cpu-core\
copy vs\cpu-render\*.h cpulib\include\cpu-render\
copy vs\cpu-engine\*.h cpulib\include\cpu-engine\
del cpulib\include\cpu-core\pch.h
del cpulib\include\cpu-render\pch.h
del cpulib\include\cpu-engine\pch.h
copy vs\x64\Debug\cpu-core.lib cpulib\lib\cpu-core-debug.lib
copy vs\x64\Debug\cpu-render.lib cpulib\lib\cpu-render-debug.lib
copy vs\x64\Debug\cpu-engine.lib cpulib\lib\cpu-engine-debug.lib
copy vs\x64\Release\cpu-core.lib cpulib\lib\
copy vs\x64\Release\cpu-render.lib cpulib\lib\
copy vs\x64\Release\cpu-engine.lib cpulib\lib\
