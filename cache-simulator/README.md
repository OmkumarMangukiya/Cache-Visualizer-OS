# Cache Simulator - Professional Edition

A comprehensive, high-performance cache simulator with C++ backend and Python GUI frontend. This project demonstrates professional software engineering practices including proper project structure, build automation, and clean API design.

## 🏗️ **Project Structure**

```
cache-simulator/
├── build/                          # CMake build directory (auto-generated)
├── cpp_logic/                      # C++ Backend
│   ├── include/simulator/          # Header files
│   │   ├── Cache.h                 # Main cache implementation
│   │   ├── CacheHierarchy.h        # Multi-level cache support
│   │   └── policies/               # Replacement policy interfaces
│   │       ├── IReplacementPolicy.h
│   │       └── LruPolicy.h
│   └── src/                        # Source files
│       ├── Cache.cpp               # Cache implementation
│       ├── CacheHierarchy.cpp      # Hierarchy implementation
│       ├── Bridge.cpp              # C-style API for Python interface
│       ├── test_main.cpp           # C++ testing
│       └── policies/
│           └── LruPolicy.cpp       # LRU policy implementation
├── python_gui/                     # Python Frontend
│   ├── main.py                     # Main GUI application
│   ├── cache_connector.py          # Python-C++ interface
│   ├── ui/                         # UI components (future)
│   └── assets/                     # Resources (future)
├── traces/                         # Sample trace files
│   ├── simple_trace.txt            # Basic test trace
│   └── complex_trace.txt           # Advanced test trace
├── CMakeLists.txt                  # Build configuration
└── README.md                       # This file
```

## 🚀 **Quick Start**

### **1. Build the C++ Backend**

```bash
# Navigate to project directory
cd cache-simulator

# Create and enter build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build the shared library
make -j$(nproc)

# Verify build
ls -la cache_logic.*  # Should show cache_logic.so (Linux) or cache_logic.dll (Windows)
```

### **2. Run the GUI Application**

```bash
# From project root
cd python_gui
python3 main.py
```

### **3. Test C++ Backend Directly**

```bash
# From build directory
./cache_test
```

## 🎯 **Features**

### **C++ Backend**

- **High-performance cache simulation** with optimized algorithms
- **Multiple replacement policies**: LRU, FIFO, Random
- **Flexible cache configurations**: Size, block size, associativity
- **Trace file processing** for realistic workload simulation
- **Thread-safe shared library** for integration
- **Comprehensive statistics** collection
- **Modular policy system** for easy extension

### **Python GUI**

- **Interactive visual cache representation** with real-time updates
- **Step-by-step animation** of cache operations
- **Configuration panel** for runtime cache parameter changes
- **Performance metrics dashboard** with hit rates and statistics
- **Trace file loading and generation** capabilities
- **Manual access testing** for educational purposes
- **Professional interface** with tabbed organization

### **Integration Features**

- **Seamless C++/Python integration** via ctypes
- **Automatic backend detection** with graceful fallback
- **JSON-based communication** for robust data exchange
- **Cross-platform compatibility** (Linux, macOS, Windows)
- **Error handling and recovery** throughout the stack

## 📊 **Supported Cache Configurations**

| Parameter          | Range             | Description          |
| ------------------ | ----------------- | -------------------- |
| Cache Size         | 256B - 1MB+       | Total cache capacity |
| Block Size         | 16B - 256B        | Cache line size      |
| Associativity      | 1-16+ way         | Set associativity    |
| Replacement Policy | LRU, FIFO, Random | Eviction algorithms  |

## 🔧 **Dependencies**

### **Required**

- **C++17 compiler** (GCC 7+, Clang 6+, MSVC 2017+)
- **CMake 3.10+** for building
- **Python 3.6+** with tkinter
- **ctypes** (usually included with Python)

### **Optional**

- **matplotlib** for advanced visualization (auto-detected)
- **ninja** for faster builds
- **clang-format** for code formatting

## 📖 **Usage Examples**

### **Basic GUI Usage**

1. **Launch**: `python3 python_gui/main.py`
2. **Configure**: Set cache parameters in the Configuration tab
3. **Generate Trace**: Click "Generate Sample Trace" for test data
4. **Animate**: Click "▶ Play" to watch cache operations
5. **Experiment**: Try different policies and configurations

### **Programmatic Usage**

```python
from cache_connector import CacheConnector

# Create and configure cache
connector = CacheConnector()
connector.create_simulator()
connector.configure_cache(cache_size=1024, block_size=64, associativity=2, policy="LRU")

# Process memory accesses
result = connector.process_access(0x1000, 'R')
print(f"Access result: {result}")

# Get statistics
stats = connector.get_statistics()
print(f"Hit rate: {stats['hit_rate']:.1f}%")
```

### **Trace File Format**

```
# Comments start with #
R 0x1000        # Read from address 0x1000
W 0x1040 100    # Write value 100 to address 0x1040
R 0x1080        # Read from address 0x1080
```

## 🏛️ **Architecture**

### **Backend (C++)**

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Cache.cpp     │    │   Bridge.cpp     │    │ Python GUI      │
│                 │    │                  │    │                 │
│ • Cache logic   │◄───┤ • C-style API    │◄───┤ • Visualization │
│ • Policies      │    │ • JSON responses │    │ • User interface│
│ • Statistics    │    │ • Error handling │    │ • Animation     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### **Key Components**

1. **SetAssociativeCache**: Core cache implementation with configurable parameters
2. **IReplacementPolicy**: Abstract interface for replacement algorithms
3. **Bridge.cpp**: C-style API layer for Python integration
4. **CacheConnector**: Python wrapper for seamless backend access
5. **CacheVisualizer**: Interactive GUI with real-time visualization

## 🧪 **Testing**

### **Automated Tests**

```bash
# Build and run C++ tests
cd build && make && ./cache_test

# Test Python connector
cd python_gui && python3 cache_connector.py
```

### **Manual Testing**

- Use the GUI's "Manual Access" feature
- Load sample traces from the `traces/` directory
- Compare different replacement policies on the same workload

## 🔬 **Performance Analysis**

The simulator supports comprehensive performance analysis:

- **Hit/Miss ratios** for different policies
- **Memory traffic analysis** with writeback tracking
- **Access pattern impact** on cache performance
- **Configuration sensitivity** studies
- **Replacement policy comparison** on real workloads

## 🛠️ **Development**

### **Adding New Replacement Policies**

1. Create header in `cpp_logic/include/simulator/policies/`
2. Implement interface from `IReplacementPolicy.h`
3. Add to enum in `Cache.h`
4. Update factory in `Cache.cpp`

### **Building for Different Platforms**

**Linux/macOS:**

```bash
cmake .. && make
```

**Windows (Visual Studio):**

```bash
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

**With specific compiler:**

```bash
export CXX=clang++
cmake .. && make
```

## 📚 **Educational Use**

This simulator is excellent for:

- **Computer architecture courses** - Understanding cache behavior
- **Performance analysis** - Comparing different cache designs
- **Algorithm study** - Replacement policy effectiveness
- **System optimization** - Memory access pattern analysis

## 🤝 **Contributing**

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure code follows project style
5. Submit a pull request

## 📄 **License**

This project is licensed under the MIT License - see the LICENSE file for details.

## 🎉 **Acknowledgments**

- Inspired by real CPU cache designs
- Built with modern C++ and Python best practices
- Designed for educational and research purposes

---

**Happy Cache Simulating! 🚀**
