# minbpe-cc

## Build

Examples to build with the ninja build system as release or debug. The `compile_commands.json` output is to help the ccls lsp server.


```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -fs ninjabuildrelease/compile_commands.json compile_commands.json
```

```
cmake -S . -B ninjabuilddebug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -fs ninjabuilddebug/compile_commands.json compile_commands.json
```

Build it


```
ninja -C ninjabuild[release|debug]
```
