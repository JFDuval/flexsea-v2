# FlexSEA Communication Protocol v2.0

**FlexSEA: Flexible & Scalable Electronics Architecture**

## History and context

- 2013-2015: original work by Jean-François (JF) Duval as part of his MIT Media Lab - Biomechatronics thesis work.
  - For documentation, refer to BioRob 2016 papers "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications" and "FlexSEA-Execute: Advanced Motion Controller for Wearable Robotic Applications", as well as to MIT thesis "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications".
  - "flexsea-comm" was part of the larger FlexSEA software project, but it wasn't usable as a standalone piece of comunication code
- 2015-20xx: project used and maintained by Dephy, Inc.
  - Sources hosted at https://github.com/JFDuval/ are licensed GNU General Public License v3.0 (GPL-3.0)
- 2024-2025: version 2.0
  - New repo (https://github.com/JFDuval/flexsea-v2) to get a clean start
  - This project is exclusively a communication stack
  - Goals: simpler code, improved API, easier integration with various projects, better unit test coverage
  - Same GPL-3.0 license, with an updated copyright to capture the full re-write

## Repository organization

FlexSEA Comm v2.0 is intended to be used with other projects. Therefore, it does not contain a `main()` or any application code. The 'projects' folder can be used to quickly build the stack as a static or dynamic library, and to run unit tests.

To test the stack on your PC use a project from the 'demo' folder. That folder also contains a demo project for a Nucleo-G431KB dev board.

- **src/:** Communication stack, source files
- **inc/:** Communication stack, header files
- **tests/:** Unit tests for the communication stack
- **projects/:**
  - **eclipse_pc/:** Eclipse C project that can be used to compile the communication stack (static and dynamic libs) and run unit tests.
- **demo/:**
  - **pc_c/:** Demo/test code written in C, with Eclipse C project. It compiles the stack (it doesn't use the static lib)
  - **pc_python/:** Demo/test code written in Python, with PyCharm project. You need to compile a DLL first. It can interact with the STM32 demo project.
  - **stm32_c/:** STM32 demo code. It's the default STM test project with a very minimalist stack integration. It can interact with the Python demo project.

## Setup & Integration

### List of software development tools

The specific versions are likely not critical, as long as they are current. What is listed below is simply what I happened to use during the development of this project.

- C IDE: Eclipse IDE for C/C++ Developers, Version 2024-06 (4.32.0)
- C compiler: GCC, You can get a copy packaged with Eclipse, from https://www.msys2.org/, or from any other provider.
  - If you use MSYS2, follow their instruction, and then call `pacman -S --needed base-devel mingw-w64-x86_64-toolchain`. This will populate the mingw64/bin folder.
  - Add the bin folder (ex.: C:\msys64\mingw64\bin) to your path. Reboot as needed.
  - Eclipse should now detect your compiler.
- Python IDE: PyCharm 2022.2.4 Community Edition
- Python interpreter: Python 3.9 and 3.11 have been used with success
  - Packages to install: 'pyserial'
- Embedded C IDE: STM32CubeIDE with the STM32G4 source package

### Unity Unit Tests (optional)

1. Download Unity (https://www.throwtheswitch.org/unity), or clone the git repository
1. Create a folder named 'unity' and place it at the same level as your flexsea_v2 project
1. Copy `unity.c`, `unity.h` and `unity_internals.h` from the Unity/src folder into unity/ 
1. Open `unity.c`. If `setUp()` and `tearDown()` are not implemented, add `void setUp(void) {};` & `void tearDown(void) {};` to the file.
1. Run `tests.c`. If needed, exclude your project's `main()`.

### How to use the Eclipse PC C project

I did not write down good notes when setting this up. If you go through the install, please contribute to this section!

- Workspace in root folder
- Active configurations are saved, but will need to be selected. The project has three configurations:
  - Test
  - StaticLib
  - DynamicLib
- Run/debug configs might need to be reconfigured
- Test is the most complicated because you need to link files outside of the project directory.


### How to use FlexSEA with your embedded project

1. We recommend placing the 'flexsea-v2' directory at the same level as 'your_project'.
1. Add 'flexsea-v2/inc' to your include folders (Properties > C/C++ Build > Settings > MCU GCC Compiler > Include paths)
1. Add 'flexsea-v2/src' to your source folders (Properties > C/C++ General > Paths and Symbols > Source Location > Link folder)
  - Note: if you Link it in Debug, you might have to simply Add it in Release, otherwise the IDE will say it already exists
1. Add `#include <flexsea.h>` to your main file. At this point, you should be able to compile the stack alongside your project.
1. To complete the integration:
  1. Copy/paste fx_receive.h & fx_transmit.h to your project
  1. Modify main.h to include flexsea.h, and these
  1. Modify main.c to include these two files
1. Follow the example 'stm32_c' project to see how the stack can be used. There is too much to document here, but a few key points are:
  1. Feed bytes into the circular buffer when they are received (via HAL_UART_RxCpltCallback())
  1. Once new bytes have been received, try parsing them
  1. Call the appropriate function to deal with the received commands

### How to use FlexSEA with your Python project

1. Start by copying the content of demo/pc_python/flexsea_demo.py
1. Adjust the sys.path.append command to point to your FlexSEA Python module
1. To get PyCharm to recognize the imported sources, click on File > Settings... > Project: (Project Name) > Project Structure > Add Content Root > Select flexsea_python > Mark as Sources.

### Stack configuration

A few `#define` can be used to change the size of buffers and communication packets.

- flexsea_codec.h/MAX_ENCODED_PAYLOAD_BYTES:
  - The size can be adjusted based on your microcontroller's memory, and communication interface
  - 48 bytes has been used as the default, based on limitations of the original stack
  - 200 bytes is currently being tested, so far so good
  - Do not exceed 256
- circ_buf.h/CIRC_BUF_SIZE:
  - Make it large enough to hold a few communication packets
  - If you are not RAM limited, bigger is better

It is critical to use the same `#define` values on the embedded side, and on the PC side. This means recompiling the libraries if those values are changed, and updating `flexsea_python.py`.

### Adding your own commands

ToDo