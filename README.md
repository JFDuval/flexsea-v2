# FlexSEA Communication Protocol v2.0

**FlexSEA: Flexible & Scalable Electronics Architecture**

## History and context

- 2013-2015: original work by Jean-François (JF) Duval as part of his MIT Media Lab - Biomechatronics thesis work.
  - For documentation, refer to BioRob 2016 papers "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications" and "FlexSEA-Execute: Advanced Motion Controller for Wearable Robotic Applications", as well as to MIT thesis "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications".
  - "flexsea-comm" was part of the larger FlexSEA software project, but it wasn't usable as a standalone piece of comunication code
- 2015-20xx: project used and maintained by Dephy, Inc.
  - Sources hosted at https://github.com/JFDuval/ are licensed GNU General Public License v3.0 (GPL-3.0)
- 2024: version 2.0
  - New repo (https://github.com/JFDuval/flexsea-v2) to get a clean start
  - This project is exclusively a communication stack
  - Goals: simpler code, improved API, easier integration with various projects, better unit test coverage
  - Same GPL-3.0 license, updated copyright to capture the full re-write

## Setup - Tools

### Eclipse and MINGW (PC)

This project was developed using Eclipse IDE for C/C++ Developers, Version 2024-06 (4.32.0). I used a copy of MinGW I already had.

### Unit Tests

1. Download Unity (https://www.throwtheswitch.org/unity), or clone the git repository
1. Create a folder named 'unity' and place it at the same level as your flexsea_v2 project
1. Copy unity.c, unity.h and unity_internals.h from the Unity/src folder into unity/ 
1. Open unity.c. If setUp() and tearDown() are not implemented, add "void setUp(void) {};" & "void tearDown(void) {};" to the file.
1. Exclude 'main.c' from your build, and run tests.c instead

Note: this is not the best way of integrating Unity into our project, but until we setup a build system this is far easier than any other way.