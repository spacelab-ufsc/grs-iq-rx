<h1 align="center">
    GRS IQ Receiver
    <br>
</h1>

<h4 align="center">SDR IQ Receiver of the SpaceLab's Ground Station.</h4>

<p align="center">
    <a href="https://github.com/spacelab-ufsc/grs-iq-rx">
        <img src="https://img.shields.io/badge/status-development-green?style=for-the-badge">
    </a>
    <a href="https://github.com/spacelab-ufsc/grs-iq-rx/releases">
        <img alt="GitHub commits since latest release (by date)" src="https://img.shields.io/github/commits-since/spacelab-ufsc/grs-iq-rx/latest?style=for-the-badge">
    </a>
    <a href="https://github.com/spacelab-ufsc/grs-iq-rx/blob/main/LICENSE">
        <img src="https://img.shields.io/badge/license-GPL3-yellow?style=for-the-badge">
    </a>
</p>

<p align="center">
    <a href="#overview">Overview</a> •
    <a href="#dependencies">Dependencies</a> •
    <a href="#building">Building</a> •
    <a href="#installing">Installing</a> •
    <a href="#documentation">Documentation</a> •
    <a href="#license">License</a>
</p>

## Overview

The GRS IQ Receiver is the SDR front-end of SpaceLab's Ground Station signal processing pipeline. It reads raw IQ samples from a Software Defined Radio (SDR) device and publishes them over a ZMQ PUB socket to downstream components such as the FFT Calculator and the demodulator.

The component is available in two implementations targeting different SDR hardware:

- **C implementation**: Targets RTL-SDR devices. Supports both synchronous and asynchronous acquisition modes, configurable gain, sample rate, bandwidth, PPM correction, and block size via command-line arguments.
- **Python implementation**: Targets ADALM-Pluto (Pluto) and USRP devices. Accepts configuration via command-line arguments and additionally listens for retune commands from the Frequency Synthesizer over a ZMQ SUB socket.

Both implementations normalize raw samples to floating-point IQ format and publish them continuously for consumption by downstream pipeline components.

## Dependencies

### C Implementation

* librtlsdr-dev (>= 2.0.1-2)
* libczmq-dev (>= 4.2.1-2)

#### Installation on Ubuntu

```
sudo apt install librtlsdr-dev libczmq-dev
```

#### Installation on Fedora

```
sudo dnf install rtl-sdr-devel czmq-devel
```

### Python Implementation

* [pyzmq](https://pypi.org/project/pyzmq/)
* [numpy](https://pypi.org/project/numpy/)
* [pyadi-iio](https://pypi.org/project/pyadi-iio/) (for Pluto support)
* [uhd](https://github.com/EttusResearch/uhd) (for USRP support)

#### Installation

```
pip install pyzmq numpy pyadi-iio
```

## Building

### C Implementation

```
make
```

## Installing

### C Implementation

```
make install
```

## Usage

### C Implementation

```
grs-iq-rx -f <frequency_hz> -s <sample_rate> -g <gain> [options]
```

| Argument | Description | Default |
|---|---|---|
| `-f`, `--frequency` | RF center frequency in Hz | 100000000 |
| `-s`, `--sample-rate` | Sample rate in S/s | 2048000 |
| `-g`, `--gain` | Tuner gain (0 for auto) | 0 (auto) |
| `-d`, `--device` | RTL-SDR device index | 0 |
| `-w`, `--bandwidth` | Tuner bandwidth in Hz | auto |
| `-p`, `--ppm-error` | Frequency correction in PPM | 0 |
| `-b`, `--block-size` | Acquisition block size | 262144 |
| `-n`, `--frames` | Number of frames to read (0 = infinite) | 0 |
| `-S`, `--sync-mode` | Force synchronous acquisition | disabled |
| `-v`, `--verbose` | Enable verbose output | disabled |

### Python Implementation

```
python iq_receiver.py -s <sdr> -f <frequency_hz> -r <sample_rate> -g <gain>
```

| Argument | Description |
|---|---|
| `-s`, `--sdr` | SDR type: `pluto` or `usrp` |
| `-f`, `--frequency` | RF center frequency in Hz |
| `-r`, `--sample_rate` | Sample rate in S/s |
| `-g`, `--gain` | Tuner gain |

## Documentation

The documentation of this project is generated using the Sphinx tool, and it is available [here](https://spacelab-ufsc.github.io/grs-iq-rx/).

### Dependencies

* Sphinx
* sphinx-rtd-theme

### Building the Documentation

```
make html
```

## License

This project is licensed under GPLv3 license.
