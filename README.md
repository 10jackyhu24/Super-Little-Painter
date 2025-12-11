# Super Little Painter 🎨

A simple yet powerful OpenGL-based paint application that allows users to draw various shapes and text on a canvas.

## Overview

Super Little Painter is an interactive graphics program built using OpenGL and GLUT (OpenGL Utility Toolkit). This project demonstrates fundamental computer graphics concepts including 2D rendering, user input handling, and interactive menu systems.

## Features

### Drawing Tools
- **Line Tool**: Draw straight lines between two points
- **Rectangle Tool**: Create rectangles by defining two opposite corners
- **Triangle Tool**: Draw triangles by specifying three vertices
- **Points Tool**: Draw colorful squares with random colors
- **Text Tool**: Add text annotations to your drawing

### Customization Options
- **Color Selection**: 8 color options (Red, Green, Blue, Cyan, Magenta, Yellow, White, Black)
- **Fill Mode**: Toggle between filled and outlined shapes
- **Pixel Size**: Adjust the size of point drawings (increase/decrease)

### User Interface
- Toolbar with visual icons for each drawing tool
- Context menus for easy access to settings
- Real-time drawing feedback

## System Requirements

### Windows
- MinGW or similar GCC compiler
- OpenGL support
- GLUT (FreeGLUT recommended)

### Libraries Required
- OpenGL (`opengl32`)
- GLU (`glu32`)
- FreeGLUT (`freeglut`)

## Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/10jackyhu24/Super-Little-Painter.git
   cd Super-Little-Painter
   ```

2. **Install FreeGLUT** (if not already installed)
   - Download FreeGLUT for MinGW from [transmissionzero.co.uk](https://www.transmissionzero.co.uk/software/freeglut-devel/)
   - Copy `freeglut.dll` to the project root directory
   - Copy `libfreeglut.a` to the `lib/` directory
   - Copy `GL/*.h` headers to the `include/GL/` directory

3. **Build the project**
   ```bash
   # Using the provided VS Code task
   # Press Ctrl+Shift+B in VS Code
   
   # Or manually compile:
   g++ -g -std=c++17 -I./include -L./lib src/newpaint.c -lopengl32 -lglu32 -lfreeglut -o main.exe
   ```

4. **Run the application**
   ```bash
   ./main.exe
   ```

## Usage

### Basic Drawing

1. **Select a Drawing Tool**
   - Click on the toolbar at the top of the window
   - Icons from left to right: Line, Rectangle, Triangle, Points, Text

2. **Draw Shapes**
   - **Line**: Click once to set start point, click again to set end point
   - **Rectangle**: Click once for first corner, click again for opposite corner
   - **Triangle**: Click three times to define the three vertices
   - **Points**: Click anywhere to draw colorful squares
   - **Text**: Click to set position, then type characters on the keyboard

### Customization

#### Using the Middle Mouse Button Menu
- **Colors Submenu**: Choose from 8 different colors
- **Pixel Size Submenu**: Increase or decrease point size
- **Fill Submenu**: Toggle fill mode on/off for shapes

#### Using the Right Mouse Button Menu
- **Quit**: Exit the application
- **Clear**: Clear the canvas and reset the drawing area

### Keyboard Controls
- When in **Text mode**, simply type to add characters to the canvas
- Characters appear at the position you clicked

## Project Structure

```
paint/
├── include/          # Header files
│   ├── GL/          # OpenGL and GLUT headers
│   ├── glad/        # GLAD loader (if used)
│   ├── GLFW/        # GLFW headers (if used)
│   └── KHR/         # Khronos headers
├── lib/             # Library files
│   └── libfreeglut.a
├── src/             # Source code
│   ├── newpaint.c   # Main paint application
│   ├── newpaint2.c  # Alternative version
│   ├── main.c       # Entry point
│   └── glad.c       # GLAD implementation (if used)
├── .vscode/         # VS Code configuration
│   └── tasks.json   # Build tasks
├── main.exe         # Compiled executable
├── glfw3.dll        # Runtime library
└── README.md        # This file
```