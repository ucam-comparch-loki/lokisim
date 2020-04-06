# lokisim
Simulator for the [Loki many-core architecture](https://link.springer.com/article/10.1007/s11265-014-0944-6).

## Prerequisites
```
sudo apt install libsystemc-dev
```

## Build
```
cd build
make -j8
```

## Usage
```
lokisim [simulator options] program [program arguments]
```

See `lokisim --help` and `lokisim --list-parameters` for available simulator options. Program binaries must be compiled for the Loki architecture.
