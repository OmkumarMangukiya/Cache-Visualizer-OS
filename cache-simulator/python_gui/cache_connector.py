"""
Python connector for the Cache Logic shared library
Provides a clean Python interface to the C++ cache implementation
"""
import ctypes
import os
import json
import platform
from pathlib import Path
class CacheConnector:
    """Python interface to the C++ cache library"""
    def __init__(self):
        self.lib = None
        self.simulator = None
        self._load_library()
        self._setup_function_signatures()
    def _load_library(self):
        """Load the shared library based on the platform"""
        if platform.system() == "Windows":
            lib_extension = ".dll"
        elif platform.system() == "Darwin":
            lib_extension = ".dylib"
        else:
            lib_extension = ".so"
        current_dir = Path(__file__).parent
        project_root = current_dir.parent
        build_dir = project_root / "build"
        lib_path = build_dir / f"cache_logic{lib_extension}"
        alternative_paths = [
            current_dir / f"cache_logic{lib_extension}",
            project_root / f"cache_logic{lib_extension}",
            Path(f"./cache_logic{lib_extension}"),
            Path(f"../build/cache_logic{lib_extension}")
        ]
        for path in [lib_path] + alternative_paths:
            if path.exists():
                try:
                    self.lib = ctypes.CDLL(str(path))
                    print(f"✓ Loaded library from: {path}")
                    return
                except OSError as e:
                    print(f"✗ Failed to load {path}: {e}")
                    continue
        error_msg = f"""
  Could not find cache_logic library!
Searched in:
{chr(10).join(f"  - {path}" for path in [lib_path] + alternative_paths)}
To build the library:
1. cd {project_root}
2. mkdir -p build && cd build
3. cmake ..
4. make
Or on Windows:
1. cd {project_root}
2. mkdir build && cd build
3. cmake ..
4. cmake --build .
"""
        raise RuntimeError(error_msg)
    def _setup_function_signatures(self):
        """Setup the function signatures for type safety"""
        if not self.lib:
            raise RuntimeError("Library not loaded")
        self.lib.create_simulator.restype = ctypes.c_void_p
        self.lib.create_simulator.argtypes = []
        self.lib.configure_cache.restype = ctypes.c_int
        self.lib.configure_cache.argtypes = [
            ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int
        ]
        self.lib.process_access.restype = ctypes.c_char_p
        self.lib.process_access.argtypes = [
            ctypes.c_void_p, ctypes.c_uint, ctypes.c_char, ctypes.c_int
        ]
        self.lib.get_statistics.restype = ctypes.c_char_p
        self.lib.get_statistics.argtypes = [ctypes.c_void_p]
        self.lib.process_trace_file.restype = ctypes.c_char_p
        self.lib.process_trace_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.lib.reset_simulator.restype = None
        self.lib.reset_simulator.argtypes = [ctypes.c_void_p]
        self.lib.destroy_simulator.restype = None
        self.lib.destroy_simulator.argtypes = [ctypes.c_void_p]
    def create_simulator(self):
        """Create a new cache simulator instance"""
        self.simulator = self.lib.create_simulator()
        if not self.simulator:
            raise RuntimeError("Failed to create simulator")
        return True
    def configure_cache(self, cache_size=1024, block_size=64, associativity=2, policy="LRU"):
        """Configure the cache with specified parameters"""
        if not self.simulator:
            raise RuntimeError("Simulator not created. Call create_simulator() first.")
        policy_map = {"LRU": 0, "FIFO": 1, "Random": 2}
        policy_int = policy_map.get(policy.upper(), 0)
        result = self.lib.configure_cache(
            self.simulator, cache_size, block_size, associativity, policy_int
        )
        if result != 1:
            raise RuntimeError("Failed to configure cache")
        return True
    def process_access(self, address, operation='R', data=0):
        """Process a single memory access"""
        if not self.simulator:
            raise RuntimeError("Simulator not configured")
        if isinstance(address, str):
            address = int(address, 16)
        operation_char = operation.upper().encode('ascii')[0]
        result_bytes = self.lib.process_access(
            self.simulator, address, operation_char, data
        )
        if result_bytes:
            result_str = result_bytes.decode('utf-8')
            try:
                return json.loads(result_str)
            except json.JSONDecodeError:
                return {"error": f"Invalid JSON response: {result_str}"}
        else:
            return {"error": "No response from library"}
    def get_statistics(self):
        """Get current cache statistics"""
        if not self.simulator:
            raise RuntimeError("Simulator not configured")
        result_bytes = self.lib.get_statistics(self.simulator)
        if result_bytes:
            result_str = result_bytes.decode('utf-8')
            try:
                return json.loads(result_str)
            except json.JSONDecodeError:
                return {"error": f"Invalid JSON response: {result_str}"}
        else:
            return {"error": "No response from library"}
    def process_trace_file(self, filename):
        """Process an entire trace file"""
        if not self.simulator:
            raise RuntimeError("Simulator not configured")
        if not os.path.isabs(filename):
            current_dir = Path(__file__).parent.parent
            filename = str(current_dir / filename)
        filename_bytes = filename.encode('utf-8')
        result_bytes = self.lib.process_trace_file(self.simulator, filename_bytes)
        if result_bytes:
            result_str = result_bytes.decode('utf-8')
            try:
                return json.loads(result_str)
            except json.JSONDecodeError:
                return {"error": f"Invalid JSON response: {result_str}"}
        else:
            return {"error": "No response from library"}
    def reset_simulator(self):
        """Reset the simulator state"""
        if self.simulator:
            self.lib.reset_simulator(self.simulator)
    def __del__(self):
        """Cleanup when the object is destroyed"""
        if hasattr(self, 'simulator') and self.simulator:
            self.lib.destroy_simulator(self.simulator)
if __name__ == "__main__":
    print("Testing Cache Connector...")
    try:
        connector = CacheConnector()
        connector.create_simulator()
        connector.configure_cache(cache_size=1024, block_size=64, associativity=2, policy="LRU")
        print("✓ Simulator created and configured")
        result1 = connector.process_access(0x1000, 'R')
        print(f"Access 1: {result1}")
        result2 = connector.process_access(0x1040, 'W', 100)
        print(f"Access 2: {result2}")
        result3 = connector.process_access(0x1000, 'R')
        print(f"Access 3: {result3}")
        stats = connector.get_statistics()
        print(f"Statistics: {stats}")
        print("✓ All tests passed!")
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()