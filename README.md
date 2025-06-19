<h1 align="center">
    GRS IQ RECEIVER
    <br>
</h1>

<h4 align="center">SDR IQ Receiver Application of the SpaceLab's Ground Station.</h4>

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
    <a href="#license">License</a>
</p>

## Overview

SDR IQ receiver application of the SpaceLab's ground station. This application reads IQ samples from an SDR (RTL-SDR for now) and transmits it over a Pub/Sub ZMQ socket.

## Dependencies

* librtlsdr-dev (>= 2.0.1-2)
* libczmq-dev (>= 4.2.1-2)

### Installation on Ubuntu

```sudo apt install librtlsdr-dev libczmq-dev```

### Installation on Fedora

```sudo dnf install rtl-sdr czmq```

## Building

```make```

## Installing

```make install```

## License

This project is licensed under GPLv3 license.
