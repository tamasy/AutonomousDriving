# Contributing to Standalone Trajectory Editor

Thank you for your interest in contributing to the Standalone Trajectory Editor! This document provides guidelines for contributing to the project.

## 🚀 Quick Start

1. **Fork** the repository
2. **Clone** your fork locally
3. **Create** a feature branch
4. **Make** your changes
5. **Test** your changes
6. **Submit** a pull request

## 🛠️ Development Setup

### Prerequisites
- Qt5 development libraries
- CMake 3.16+
- C++17 compatible compiler
- Linux environment (Ubuntu recommended)

### Build Instructions
```bash
# Install dependencies
sudo apt-get install qt5-default cmake build-essential

# Build project
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 📝 Code Style

### C++ Guidelines
- Follow C++17 standards
- Use meaningful variable and function names
- Add comments for complex logic
- Maintain consistent indentation (4 spaces)

### Qt Specifics
- Use Qt naming conventions for signals/slots
- Prefer modern Qt features (Qt5)
- Memory management: use smart pointers where appropriate

### File Organization
```
src/
├── core/     # Data structures and business logic
├── gui/      # User interface components
├── utils/    # Helper functions and utilities
└── main.cpp  # Application entry point
```

## 🧪 Testing

### Manual Testing
- Test with various CSV formats
- Verify dual trajectory loading
- Check editing operations (move, speed change)
- Test pan/zoom functionality
- Validate unit conversions (m/s ↔ km/h)

### Test Data
Use the provided sample files:
- `data/raceline_awsim_15km.csv` - Standard format
- `data/raceline_awsim_shortest_m_11.csv` - High-density data
- `data/track_boundaries.csv` - Track boundaries

## 📋 Pull Request Process

### Before Submitting
1. **Build successfully** on Linux
2. **Test basic functionality** with sample data
3. **Update documentation** if adding features
4. **Follow commit message conventions**

### Commit Messages
```
type(scope): brief description

- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit first line to 72 characters
- Reference issues with "Fixes #123" or "Closes #123"
```

Examples:
```
feat(gui): add middle mouse button panning
fix(core): correct speed unit conversion in CSV loading
docs(readme): update build instructions for Qt5
```

### Review Process
1. Maintainers will review your PR
2. Address any feedback or requested changes
3. Once approved, your PR will be merged

## 🐛 Bug Reports

### Before Reporting
- Check existing issues
- Test with latest version
- Prepare minimal reproduction case

### Bug Report Template
```
**Environment**
- OS: Ubuntu 20.04
- Qt Version: 5.12.8
- Compiler: GCC 9.4.0

**Steps to Reproduce**
1. Load CSV file...
2. Click on point...
3. Error occurs...

**Expected Behavior**
[What you expected to happen]

**Actual Behavior**
[What actually happened]

**Sample Data**
[Attach CSV file if relevant]
```

## 💡 Feature Requests

### Feature Request Template
```
**Feature Description**
[Clear description of the feature]

**Use Case**
[Why this feature would be useful]

**Proposed Implementation**
[Optional: how you think it could be implemented]

**Alternatives Considered**
[Other solutions you've considered]
```

## 🎯 Areas for Contribution

### High Priority
- [ ] Performance optimization for large datasets
- [ ] Additional CSV format support
- [ ] Improved error handling and user feedback
- [ ] Cross-platform compatibility (Windows, macOS)

### Medium Priority
- [ ] 3D visualization capabilities
- [ ] Animation/playback features
- [ ] Plugin architecture
- [ ] Internationalization (i18n)

### Low Priority
- [ ] Dark theme support
- [ ] Keyboard shortcuts
- [ ] Export to other formats (KML, GPX)
- [ ] Unit tests and continuous integration

## 📚 Resources

### Qt Documentation
- [Qt5 Documentation](https://doc.qt.io/qt-5/)
- [Qt Graphics View Framework](https://doc.qt.io/qt-5/graphicsview.html)

### C++ Resources
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Modern C++ Features](https://github.com/AnthonyCalandra/modern-cpp-features)

## 🤝 Code of Conduct

### Our Standards
- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Maintain professional communication

### Enforcement
Maintainers have the right to remove, edit, or reject contributions that don't align with these standards.

## 📞 Getting Help

- **Issues**: Use GitHub Issues for bugs and feature requests
- **Discussions**: Use GitHub Discussions for questions and ideas
- **Code Review**: Comment on pull requests for implementation feedback

## 🙏 Recognition

Contributors will be acknowledged in:
- Release notes
- CONTRIBUTORS.md file (if created)
- Project documentation

Thank you for helping make this project better! 🚀