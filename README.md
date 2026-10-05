<h1> LiteCityForge / Procedural City Generation Tool </h1>
<p>An interactive C++ and OpenGL tool for generating customizable 3D cities</p>
<h6>C++17 · OpenGL 3.3 · GLSL · GLFW · Dear ImGui · CMake</h6>

![2D view generation](src/docs/images/city-2D-showcase.gif)

<h3>What is LiteCityForge?</h3>
<p>LiteCityForge is an interactive C++17 and OpenGL application for generating customizable 3D city layouts. Starting from a user-selected seed and generation settings, it builds a road network, subdivides the surrounding land into lots, and creates building footprints and geometry with varied urban density. A graphical interface lets you tune generation and appearance, inspect the result in 2D or 3D, and export selected city elements as OBJ and MTL files for use in other 3D tools.</p>

<h3>Features</h3>
<ul>
  <li>Generate road networks, city blocks, building lots, and 3D buildings from a configurable seed.</li>
  <li>Adjust generation settings for city size, road layout, lot coverage, and urban density.</li>
  <li>Explore generated cities in 2D or 3D.</li>
  <li>Customize the scene’s materials and textures through the GUI.</li>
  <li>Export selected city geometry as OBJ files with accompanying MTL materials.</li>
</ul>

<h3>Build and run (Windows) 🪟</h3>
<h4>Requirements:</h4>
<ul>
  <li>Windows 10 or 11</li>
  <li>Git for Windows</li>
  <li>Visual Studio 2022 with the "Desktop development with C++" workload</li>
  <li>CMake 3.16 or newer</li>
  <li>A graphics card and driver that support OpenGL 3.3</li>
</ul>
<h4>Build steps</h4>
<ol>
  <li>Open PowerShell in the folder </li>
  <li>Clone the repository:

   ```powershell
   git clone https://github.com/MMulevicius/LiteCityForge.git
   ```
  </li>
  <li>Move into the project folder:

   ```powershell
   cd LiteCityForge
   ```
  </li>
  <li>Configure the project with CMake. This creates a `build` folder and generates Visual Studio build files for 64-bit Windows:

   ```powershell
   cmake -S . -B build -A x64
   ```
  </li>
  <li>Compile the project in Release mode:

   ```powershell
   cmake --build build --config Release
   ```
  </li>
  <li>Run the application from the project folder:

   ```powershell
   .\build\Release\ProceduralCityGenerator.exe
   ```
  </li>
  <p>NOTE: CMake copies the required "assets" folder beside the executable during the build.</p>
</ol>

<h3>Build and run (Arch Linux) 🐧</h3>
<h4>Requirements:</h4>
<ul>
  <li>A C++17 compiler, such as GCC</li>
  <li>CMake 3.16 or newer</li>
  <li>Git</li>
  <li>X11 and OpenGL development packages</li>
  <li>An OpenGL 3.3-capable graphics driver</li>
</ul>

<h4>Build steps</h4>
<ol>
  <li>Install required packages:
    
  ```bash
  sudo pacman -S --needed base-devel cmake git mesa libglvnd \
    xorgproto libx11 libxext libxrandr libxinerama libxcursor libxi
  ```
  </li>
  <li>
    Clone the repository:

   ```bash
   git clone https://github.com/MMulevicius/LiteCityForge.git
   ```
  </li>
  <li>Go into the project folder:

   ```bash
   cd LiteCityForge
   ```
  </li>
  <li>Configure a Release build:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
  </li>
  <li>Build the application:

   ```bash
   cmake --build build --parallel
   ```
  </li>
  <li>Run it:

   ```bash
   ./build/ProceduralCityGenerator
   ```
  </li>
</ol>
<h2>Controls</h2>

<table>
  <thead>
    <tr>
      <th>Mode</th>
      <th>Input</th>
      <th>Action</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>2D</td>
      <td><kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd></td>
      <td>Pan the camera</td>
    </tr>
    <tr>
      <td>2D</td>
      <td>Scroll wheel</td>
      <td>Zoom in or out</td>
    </tr>
    <tr>
      <td>3D</td>
      <td><kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd></td>
      <td>Move the camera</td>
    </tr>
    <tr>
      <td>3D</td>
      <td><kbd>Q</kbd> / <kbd>E</kbd></td>
      <td>Move down / up</td>
    </tr>
    <tr>
      <td>3D</td>
      <td>Hold right mouse button and move mouse</td>
      <td>Look around</td>
    </tr>
    <tr>
      <td>Any</td>
      <td><kbd>Esc</kbd></td>
      <td>Exit the application</td>
    </tr>
  </tbody>
</table>

<p>Switch between 2D and 3D using the <strong>3D mode</strong> checkbox.</p>

<h3>Export</h3>
<h3>Process</h3>

<h3>Known Issues</h3>
<ul></ul>

