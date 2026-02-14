# Standalone Trajectory Editor

A visual trajectory editing application for automotive/robotics applications, designed as an independent tool extracted from Autoware's trajectory editing capabilities.

## 🚗 Overview

This standalone application provides intuitive visual editing of CSV trajectory data using Qt GraphicsView. It supports dual trajectory comparison, speed visualization with color coding, and interactive editing operations.

## ✨ Key Features

### 🎯 Trajectory Editing
- **Dual Trajectory Support**: Load and compare two CSV trajectories simultaneously
- **Color-coded Speed Display**: Green system for trajectory 1, Blue system for trajectory 2
- **Unit Conversion**: Internal storage in m/s, UI display in km/h
- **Multiple CSV Formats**: Supports both 4-column (x,y,z,velocity) and 8-column (x,y,z,qx,qy,qz,qw,speed) formats
- **Point Editing**: Click and drag points to modify trajectory coordinates
- **Speed Editing**: Individual point and range velocity editing
- **Edit History**: Undo/Redo functionality with command pattern

### 🗺️ Track Visualization
- **Track Boundaries**: Display left/right boundaries as gray points
- **Coordinate Systems**: East-North, East-South, South-West, North-West support
- **Auto-fit**: Automatic view adjustment when changing coordinate systems
- **Zoom Persistence**: Maintains zoom level during trajectory editing

### 🖱️ Interactive Controls
- **Mouse Operations**:
  - **Left Click**: Select and edit trajectory points
  - **Middle Click + Drag**: Pan view to explore track areas
  - **Mouse Wheel**: Zoom in/out
  - **Drag Points**: Move trajectory points with 5-pixel sensitivity
- **Keyboard Shortcuts**: Standard zoom and fit operations
- **Responsive UI**: Optimized for high-density trajectory data

### 🎨 Visual Customization
- **Point Size**: Adjustable from 0.5 to larger sizes
- **Line Width**: Configurable trajectory line thickness
- **No Outlines**: Clean point display without borders
- **Speed Color Mapping**: Customizable min/mid/max speed ranges

## 🏗️ Architecture

```
src/
├── core/                    # Data Management
│   ├── trajectory_data.hpp/.cpp    # Trajectory data structure
│   ├── track_boundaries.hpp/.cpp   # Track boundary data
│   └── edit_history.hpp/.cpp       # Command pattern editing
├── gui/                     # User Interface
│   └── graphics_trajectory_view.hpp/.cpp  # Qt GraphicsView display
├── utils/                   # Utilities
│   ├── csv_parser.hpp/.cpp         # CSV file handling
│   └── osm_parser.hpp/.cpp         # OSM/Lanelet2 parsing
└── main.cpp                 # Main application
```

## 🔧 Technical Specifications

### Development Environment
- **Language**: C++17
- **GUI Framework**: Qt5 (Core, Widgets)
- **Build System**: CMake 3.16+
- **Platform**: Linux (tested on Ubuntu)

### Dependencies
- Qt5 Core & Widgets
- CMake 3.16+
- C++ Standard Library

## 📁 Data Formats

### Trajectory Data (CSV)
```csv
# 4-column format
x,y,z,velocity_ms
89626.42,43187.84,6.5,2.777

# 8-column format (AWSIM)
x,y,z,qx,qy,qz,qw,speed
89631.12,43131.45,0,0,0,0.893,0.449,9.72222
```

### Track Boundaries (CSV)
```csv
left_x,left_y,left_z,right_x,right_y,right_z
89684.1,43139.7,6.5,89682.9,43141.4,6.5
```

## 🔨 Build Instructions

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install qt5-default cmake build-essential

# Clone and build
git clone [repository-url]
cd standalone-trajectory-editor
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./trajectory_editor
```

## 🎮 Usage Guide

### Basic Operations

1. **Load Trajectories**:
   - Green trajectory: Use "Load CSV 1" button
   - Blue trajectory: Use "Load CSV 2" button
   - Files are displayed with color-coded indicators

2. **Navigation**:
   - **Pan**: Middle mouse button + drag
   - **Zoom**: Mouse wheel
   - **Fit View**: Use coordinate system dropdown to auto-fit

3. **Edit Trajectories**:
   - **Select Point**: Left click on trajectory point (5px sensitivity)
   - **Move Point**: Drag selected point to new position
   - **Edit Speed**: Use velocity editor panel for individual or range editing

4. **Visual Controls**:
   - **Point Size**: Adjust from 0.5 for dense trajectories
   - **Line Width**: Modify trajectory line thickness
   - **Coordinate System**: Switch between East-North, East-South, etc.

5. **Save Results**:
   - "Save CSV 1" / "Save CSV 2": Save trajectories separately in m/s units

### Advanced Features

- **Zoom Maintenance**: Zoom level persists during point editing
- **Dual Comparison**: Load two trajectories for side-by-side analysis
- **High-Density Support**: Optimized for files like `raceline_awsim_shortest_m_11.csv`
- **Filename Display**: Current loaded files shown in control panel

## 📊 Performance

### Tested Datasets
- **Dense Trajectories**: 1000+ points (raceline_awsim_shortest_m_11.csv)
- **Track Boundaries**: 268+ boundary points
- **Real-time Response**: Smooth interaction even with high-density data

### Coordinate Range (Example)
- **X-axis**: 89612.7 ~ 89686.7
- **Y-axis**: 43116.6 ~ 43193.1  
- **Z-axis**: Typically 6.5m (ground level)

## 🚀 Future Enhancements

### Planned Features
- [ ] 3D visualization support
- [ ] Spline interpolation for smooth trajectories
- [ ] Animation/playback functionality
- [ ] Export to KML/GPX formats
- [ ] ROSbag integration

### UI Improvements
- [ ] Dark theme support
- [ ] Configurable color schemes
- [ ] Multi-language support
- [ ] Plugin architecture

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 🐛 Bug Reports

Please report issues through GitHub Issues with:
- Operating system and version
- Qt version
- Steps to reproduce
- Sample data files (if applicable)

## 📈 Development Status

**Current Version**: Prototype v1.0
- ✅ Core trajectory editing
- ✅ Dual trajectory support  
- ✅ Interactive pan/zoom
- ✅ Speed visualization
- ✅ Unit conversion (m/s ↔ km/h)
- ✅ Command pattern editing
- ✅ Multiple CSV format support

---

*Generated with Claude Code* 🤖