MacInject
=========

Command line tool and library to inject and execute program code in another process.  
It is easy to use and does not require deep knowledge like [mach_inject](https://github.com/rentzsch/mach_inject) asserts.  
Just compile a piece of code you want to inject as shared lib and call the MacInjectTool.  
Notice: The tool waits for the injected code to finish in order to free all resources.  
Platforms: Only Intel x64 on Mac OS 10.7+ is supported.  


Example
-------

```c
#include <syslog.h>
#include <unistd.h>

void init() {
    syslog(LOG_NOTICE, "I got inside %d\n", getpid());
}
```

`clang -shared -o InjectMe.image InjectMe.c`

`sudo MacInjectTool -p InjectMe.image -t 1234`