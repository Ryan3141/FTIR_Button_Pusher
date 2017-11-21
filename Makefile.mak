# Sample makefile  
  
PATH=$(PATH);C:\Program Files (x86)\Old Microsoft Visual Studio\Common\MSDev98\Bin;C:\Program Files (x86)\Old Microsoft Visual Studio\VC98\Bin
 
all: test.exe 
  
.c.obj:  
  $(cc) $(cdebug) $(cflags) $(cvars) $*.c  
  
test.exe: main.obj  
  LINK $(ldebug) $(conflags) /out:test.exe main.obj $(conlibs) lsapi32.lib user32.lib 
 
