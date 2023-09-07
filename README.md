## SOD

# Project STM32

This project is focused on developing and implementing an algorythm for human face counting in a closed space using _STM32F429_ and _OV7670_ low cost camera.
The algorythm chosen is **Viola-Jones**, able to detect faces using visual features like the distribution of shadows of a human head, allowing an implementation that requires low computational power.
After the hardware setup (STM32 clock, interfaces, camera wiring), the actual algorythm is implemented in MATLAB using _Computer Vision Toolbox_; then the code is converted to C code through _C Coder_ and implemented in *STM32CUBEIDE* software interface.

The workflow consists in:
1. requirement definition
2. hardware setup
3. code workflow design
4. Viola-Jones algorythm code in Matlab
5. conversion to C code
6. implementation on STM32 and debugging
