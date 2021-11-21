# How-to: Use VSC to debug GoL implementation

- Step 1: add the `-g` flag to `CC` in the Makefile
- Step 2: recompile the program by runnin `make`
- Step 3: copy the included [launch.json](./docs/launch.json) file in your workspace's `.vscode` folder
- Step 4: set your brake points
- Step 5: press `ctl+shift+d` and then `enter` to start debugging with vsc