# ⛏️ Duino-Coin C Miner

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Language](https://img.shields.io/badge/language-C-green.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20WSL-lightgrey)](https://github.com/nam348tnh3gp/CDUCO)
[![Build](https://img.shields.io/badge/build-Makefile-brightgreen)](https://github.com/nam348tnh3gp/CDUCO)

> A lightweight, high-performance Duino-Coin miner written in C with multithreading support.  
> Optimized for low-resource devices and maximum efficiency.

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🚀 **High Performance** | Optimized SHA1 implementation for faster mining |
| 🧵 **Multithreading** | Configurable thread count (1-4 threads) |
| 🔌 **Direct TCP Connection** | Native protocol implementation without HTTP overhead |
| 📊 **Real-time Statistics** | Live hashrate and acceptance rate display |
| ⚙️ **Easy Configuration** | Simple config.txt file for all settings |
| 🔄 **Auto Reconnection** | Automatically reconnects on connection loss |
| 🎯 **Multiple Difficulty** | Works with LOW, MEDIUM, HIGH difficulties |

---

## 📋 Prerequisites

### Required Packages
- **Clang** or **GCC** compiler
- **libcurl** development library
- **pthread** (included by default)

### Install Dependencies

<details>
<summary><b>Ubuntu/Debian</b></summary>

```bash
sudo apt update
sudo apt install clang libcurl4-openssl-dev
```

</details><details>
<summary><b>Arch Linux</b></summary>```bash
sudo pacman -S clang curl
```

</details><details>
<summary><b>macOS</b></summary>```bash
brew install clang curl
```

</details><details>
<summary><b>Fedora</b></summary>```bash
sudo dnf install clang libcurl-devel
```

</details>---

🚀 Quick Start

1. Clone the Repository

```bash
git clone https://github.com/nam348tnh3gp/CDUCO.git
cd CDUCO
```

2. Configure the Miner

Create config.txt in the project directory:

```ini
username=YourDuinoUsername
mining_key=YourMiningKey
difficulty=LOW
rig_identifier=MyRig
thread_count=2
```

Configuration Options

Option Description Example
username Your Duino-Coin username JohnDoe
mining_key Your mining key (from wallet) 123456
difficulty Mining difficulty LOW / MEDIUM / HIGH
rig_identifier Name for your mining rig MyRig
thread_count Number of threads (1-4) 2

3. Build the Miner

```bash
make
```

4. Run the Miner

```bash
./miner
```

Press Ctrl+C to stop the miner gracefully.

---

📊 Performance Examples

Device Threads Difficulty Hashrate
Raspberry Pi 4 2 LOW ~800 kH/s
Intel i5-8250U 4 LOW ~3.2 MH/s
Intel i7-10750H 4 MEDIUM ~4.5 MH/s
Intel i9-12900K 4 HIGH ~6.8 MH/s

---

🛠️ Building with Makefile

Available Commands

```bash
make          # Build the miner
make clean    # Remove build artifacts
make install  # Install to /usr/local/bin (requires sudo)
```

Custom Build

To compile manually with optimization flags:

```bash
clang -o miner main.c DSHA1.c -lpthread -lcurl -O3 -march=native
```

---

📁 Project Structure

```
CDUCO/
├── main.c          # Main miner implementation
├── DSHA1.c         # SHA1 algorithm implementation
├── DSHA1.h         # SHA1 header file
├── Makefile        # Build automation
├── config.txt      # Configuration file (create this)
├── LICENSE         # Apache License 2.0
└── README.md       # This file
```

---

🔧 Troubleshooting

<details>
<summary><b>Connection Issues</b></summary>· Test pool connectivity: curl https://server.duinocoin.com/getPool
· Check internet connection: ping 8.8.8.8

</details><details>
<summary><b>Compilation Errors</b></summary>· Verify libcurl is installed: curl-config --libs
· Check compiler version: clang --version
· Ensure all dependencies are installed

</details><details>
<summary><b>Low Hashrate</b></summary>· Try reducing thread_count if CPU is overloaded
· Close other CPU-intensive applications
· Use -march=native flag for CPU optimization
· Consider using LOW difficulty for better stability

</details>---

📝 How It Works

```mermaid
graph LR
    A[Get Pool] --> B[TCP Connect]
    B --> C[Request Job]
    C --> D[Solve SHA1]
    D --> E[Submit Solution]
    E --> F[Get Feedback]
    F --> C
```

1. Get Pool - Fetches optimal mining pool from Duino-Coin API
2. Connect - Establishes TCP connection to the pool
3. Request Job - Sends JOB,username,difficulty,mining_key
4. Solve - Finds nonce producing SHA1 hash matching target
5. Submit - Sends solution back to pool
6. Repeat - Continuous mining loop

---

🤝 Contributing

Contributions are welcome! Here's how you can help:

1. 🍴 Fork the repository
2. 🌿 Create your feature branch (git checkout -b feature/AmazingFeature)
3. 💾 Commit your changes (git commit -m 'Add some AmazingFeature')
4. 📤 Push to the branch (git push origin feature/AmazingFeature)
5. 🔍 Open a Pull Request

---

📄 License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

```
Copyright 2024 Nam348tnh

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

---

 Acknowledgments

· Duino-Coin - The cryptocurrency project
· DSHA1 Implementation - SHA1 algorithm in C
· The Duino-Coin community for continuous support

---

## 📧 Contact & Support

- **Issues**: [GitHub Issues](https://github.com/nam348tnh3gp/CDUCO/issues)
- **Discord**: [Join Duino-Coin Discord](https://discord.gg/duinocoin)

---

<div align="center">

**⭐ Star this repository if you find it useful!**

[Report Bug](https://github.com/nam348tnh3gp/CDUCO/issues) · [Request Feature](https://github.com/nam348tnh3gp/CDUCO/issues)

</div>
