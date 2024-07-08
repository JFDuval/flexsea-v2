# FlexSEA Communication Protocol v2.0

**FlexSEA: Flexible & Scalable Electronics Architecture**

- 2013-2015: original work by Jean-François (JF) Duval as part of his MIT Media Lab - Biomechatronics thesis work.
  - For documentation, refer to BioRob 2016 papers "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications" and "FlexSEA-Execute: Advanced Motion Controller for Wearable Robotic Applications", as well as to MIT thesis "FlexSEA: Flexible, Scalable Electronics Architecture for Wearable Robotic Applications".
  - "flexsea-comm" was part of the larger FlexSEA software project, but it wasn't usable as a standalone piece of code
- 2015-20xx: project used and maintained by Dephy, Inc.
  - Sources hosted at https://github.com/JFDuval/ are licensed GNU General Public License v3.0 (GPL-3.0)
- 2024: version 2.0
  - New repo (https://github.com/JFDuval/flexsea-v2) to get a cleaner start
  - This project is exclusively a communication stack
  - Simpler code, better unit test coverage

## Setup - Unit tests

1. Download Unity (https://www.throwtheswitch.org/unity)
1. Same in a folder named 'unity' in the project
1. Open src/unity.c. If setUp() and tearDown() are not implemented, add "void setUp(void) {};" & "void tearDown(void) {};" to the file.
1. Exclude 'main.c' from your build, and run tests.c instead