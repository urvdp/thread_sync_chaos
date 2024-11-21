# Project Overview
This project models a road intersection using threads as vehicle representations to synchronize the crossing of an east-west and north-south lane.

![image](https://github.com/user-attachments/assets/f09a2fae-d5eb-47a0-aab3-84ec1a8eafea)

## Build Instructions

To build the project, ensure you create the `build` directory before running `make`:

```bash
mkdir build
make
```



# Project Structure

```plaintext
project/
├── src/                # Source files
│   ├── main.c          # Main entry point
│   ├── pantalla.c      # Module: Display logic
│   ├── cola.c          # Module: Queue logic
│   ├── vehiculo.c      # Module: Vehicle-related logic
│   └── utils.c         # Utility functions
├── include/            # Header files
│   ├── pantalla.h      # Header for pantalla.c
│   ├── cola.h          # Header for cola.c
│   ├── vehiculo.h      # Header for vehiculo.c
│   └── utils.h         # Header for utils.c
├── build/              # Compiled objects and binaries
│   ├── *.o             # Object files
│   ├── executable      # Final executable
├── logs/               # Log files (if applicable)
│   └── *.log           # Log output
├── tests/              # Test files
│   ├── test_pantalla.c # Test for pantalla module
│   └── test_utils.c    # Test for utility functions
├── CMakeLists.txt      # Build system (if using CMake)
├── Makefile            # Build system (if using Make)
└── README.md           # Project documentation
