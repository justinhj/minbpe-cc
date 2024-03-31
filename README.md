# minbpe-cc

## Build

```
cmake -S . -B ninjabuild -G Ninja
```

```
cmake -S . -B ninjabuild -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -s ninjabuild/compile_commands.json compile_commands.json
```

```
ninja -C ninjabuild
```
