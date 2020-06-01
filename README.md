# lokisim
Simulator for the [Loki many-core architecture](https://link.springer.com/article/10.1007/s11265-014-0944-6).

## Model
Loki is a tiled, many-core processor, where the on-chip network is deeply integrated into the components. e.g. Most instruction encodings allow operands to be received directly from the network or results to be sent directly onto the network, and all memory access is performed over the network. This allows flexibility in which resources are accessible, and allows the simple cores and memory banks to cooperate [in novel ways](https://www.cl.cam.ac.uk/~db434/publications/hip3es16.pdf) to achieve more complex behaviour.

We aim to make components
* **A**ccessible: any core should be able to make use of any resource on the chip to help with its task.
* **B**ypassable: it should be possible to bypass functionality when it is not needed to reduce energy consumption (e.g. avoid checking cache tags when we already know data is present).
* **C**omposable: components should be able to work closely together, forming larger *virtual processors*.

![](img/chip-and-tile.png)


## Native installation
### Prerequisites
Requires Ubuntu >= 19.04.

```
sudo apt install libsystemc-dev
```

### Build
```
cd build
make -j8
```

### Usage
```
lokisim [simulator options] <program> [program arguments]
```

See the [wiki](https://github.com/ucam-comparch-loki/lokisim/wiki) for further information on available parameters. Program binaries must be compiled for the Loki architecture.

## Docker
### Build
```
docker build --network=host . -t lokisim
```

### Usage
```
docker run -v </abs/path/to/program/dir>:/app lokisim [simulator options] /app/<program> [program arguments]
```
