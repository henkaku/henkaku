# Build

Only Linux is supported. You will need:

* Python 3
* [Vita SDK](http://vitasdk.org/)
* Uglify JS (Ubuntu 16.04 package node-uglify) tested version 2.4.15
* Other standard tools that you should already have installed, such as openssl, dd, cat, touch

All tools should be in your PATH.

Run `./build.sh` to build everything. This script first cleans up all build directories and then builds the exploit.

# Develop

It's useful to have everything be automatically rebuilt when a source file changes. To achieve that, install `entr` and execute in a separate termina:

```
while sleep 1; do find build.sh krop/ payload/ urop/ webkit/ | entr -d ./build.sh ; done
```

Then when you change a source file (or add a new one), everything will be automatically rebuilt. Note that if you add a new directory, you will need to update the script.

# Distrib

Distribute only the files from the `output` directory, don't distribute any other files.

There are two versions of the exploit, located in the `output` directory. Either can be served using any regular web server (e.g. `python3 -m http.server`).

The `offline` payload is for running with the Email client. The `web` payload is for running on the Browser app.

# Exploit

To run the exploit, first cold boot your Vita, then navigate to `exploit.html`. The following situations are possible:

* browser doesn't display any alert()s and displays a gray "Please wait..." screen after a few seconds: this is normal, allow up to 10 reloads before the exploit actually triggers
* you get a "trigger" alert(), then nothing happens: the exploit succeeded
* you get a "trigger" alert(), then the browser crashes: the exploit failed, retry it a few times, then reboot the Vita and try again
* you get a "trigger" alert(), then the system crashes: that shouldn't happen, try the exploit again
