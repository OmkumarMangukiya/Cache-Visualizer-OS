import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import random
try:
    from cache_connector import CacheConnector
    BACKEND_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import cache_connector: {e}")
    BACKEND_AVAILABLE = False
try:
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False
    print("Matplotlib not available. Advanced charts will be disabled.")
class CacheVisualizer:
    def __init__(self, root):
        self.root = root
        self.root.title("Cache Visualizer - Professional Edition")
        self.root.geometry("1400x900")
        self.cache_size = 1024
        self.block_size = 64
        self.associativity = 2
        self.num_sets = self.cache_size // (self.block_size * self.associativity)
        self.replacement_policy = "LRU"
        self.connector = None
        self.backend_ready = False
        if BACKEND_AVAILABLE:
            try:
                self.connector = CacheConnector()
                self.connector.create_simulator()
                self.connector.configure_cache(
                    cache_size=self.cache_size,
                    block_size=self.block_size,
                    associativity=self.associativity,
                    policy=self.replacement_policy
                )
                self.backend_ready = True
                print("✓ Backend connected and configured successfully")
            except Exception as e:
                print(f"⚠ Backend initialization failed: {e}")
                messagebox.showwarning("Backend Warning", 
                    f"C++ backend not available: {e}\n\nFalling back to simulation mode.")
        self.cache_state = {}
        self.access_history = []
        self.current_trace = []
        self.trace_index = 0
        self.is_playing = False
        self.total_accesses = 0
        self.hits = 0
        self.misses = 0
        self.setup_gui()
        self.initialize_cache()
    def setup_gui(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        left_panel = ttk.Frame(main_frame)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        right_panel = ttk.Frame(main_frame)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        self.setup_control_panel(left_panel)
        self.setup_visualization_panel(right_panel)
    def setup_control_panel(self, parent):
        config_frame = ttk.LabelFrame(parent, text="Cache Configuration", padding=10)
        config_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(config_frame, text="Cache Size (bytes):").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.cache_size_var = tk.StringVar(value="1024")
        cache_size_entry = ttk.Entry(config_frame, textvariable=self.cache_size_var, width=10)
        cache_size_entry.grid(row=0, column=1, padx=5, pady=2)
        cache_size_entry.bind('<Return>', self.update_cache_config)
        ttk.Label(config_frame, text="Block Size (bytes):").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.block_size_var = tk.StringVar(value="64")
        block_size_entry = ttk.Entry(config_frame, textvariable=self.block_size_var, width=10)
        block_size_entry.grid(row=1, column=1, padx=5, pady=2)
        block_size_entry.bind('<Return>', self.update_cache_config)
        ttk.Label(config_frame, text="Associativity:").grid(row=2, column=0, sticky=tk.W, pady=2)
        self.associativity_var = tk.StringVar(value="2")
        assoc_entry = ttk.Entry(config_frame, textvariable=self.associativity_var, width=10)
        assoc_entry.grid(row=2, column=1, padx=5, pady=2)
        assoc_entry.bind('<Return>', self.update_cache_config)
        ttk.Label(config_frame, text="Replacement Policy:").grid(row=3, column=0, sticky=tk.W, pady=2)
        self.policy_var = tk.StringVar(value="LRU")
        policy_combo = ttk.Combobox(config_frame, textvariable=self.policy_var,
                                   values=["LRU", "FIFO", "Random"], state="readonly", width=8)
        policy_combo.grid(row=3, column=1, padx=5, pady=2)
        policy_combo.bind('<<ComboboxSelected>>', self.update_cache_config)
        ttk.Button(config_frame, text="Apply Config", 
                  command=self.update_cache_config).grid(row=4, column=0, columnspan=2, pady=10)
        trace_frame = ttk.LabelFrame(parent, text="Trace Control", padding=10)
        trace_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Button(trace_frame, text="Load Trace File", 
                  command=self.load_trace_file).pack(fill=tk.X, pady=2)
        ttk.Button(trace_frame, text="Generate Sample Trace", 
                  command=self.generate_sample_trace).pack(fill=tk.X, pady=2)
        ttk.Button(trace_frame, text="Manual Access", 
                  command=self.manual_access_dialog).pack(fill=tk.X, pady=2)
        self.trace_info_label = ttk.Label(trace_frame, text="No trace loaded")
        self.trace_info_label.pack(fill=tk.X, pady=2)
        anim_frame = ttk.LabelFrame(parent, text="Animation Control", padding=10)
        anim_frame.pack(fill=tk.X, pady=(0, 10))
        control_buttons = ttk.Frame(anim_frame)
        control_buttons.pack(fill=tk.X)
        self.play_button = ttk.Button(control_buttons, text="▶ Play", 
                                     command=self.toggle_animation)
        self.play_button.pack(side=tk.LEFT, padx=2)
        ttk.Button(control_buttons, text="⏸ Step", 
                  command=self.step_animation).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_buttons, text="⏹ Reset", 
                  command=self.reset_simulation).pack(side=tk.LEFT, padx=2)
        ttk.Label(anim_frame, text="Speed:").pack(anchor=tk.W, pady=(10,0))
        self.speed_var = tk.DoubleVar(value=1.0)
        speed_scale = ttk.Scale(anim_frame, from_=0.1, to=3.0, variable=self.speed_var, orient=tk.HORIZONTAL)
        speed_scale.pack(fill=tk.X, pady=2)
        ttk.Label(anim_frame, text="Progress:").pack(anchor=tk.W, pady=(10,0))
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(anim_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.pack(fill=tk.X, pady=2)
        metrics_frame = ttk.LabelFrame(parent, text="Performance Metrics", padding=10)
        metrics_frame.pack(fill=tk.X, pady=(0, 10))
        self.metrics_text = tk.Text(metrics_frame, height=8, width=25, font=('Courier', 9))
        self.metrics_text.pack(fill=tk.BOTH, expand=True)
        legend_frame = ttk.LabelFrame(parent, text="Legend", padding=10)
        legend_frame.pack(fill=tk.X)
        legend_text = """
Colors:
🟢 Hit (Green)
🔴 Miss (Red)
🟡 Recently Used (Yellow)
⚪ Empty (White)
🔵 Current Access (Blue)
        """
        ttk.Label(legend_frame, text=legend_text, font=('Arial', 8)).pack()
    def setup_visualization_panel(self, parent):
        canvas_frame = ttk.LabelFrame(parent, text="Cache Visualization", padding=10)
        canvas_frame.pack(fill=tk.BOTH, expand=True)
        canvas_container = ttk.Frame(canvas_frame)
        canvas_container.pack(fill=tk.BOTH, expand=True)
        self.canvas = tk.Canvas(canvas_container, bg='white', scrollregion=(0, 0, 1000, 800))
        v_scrollbar = ttk.Scrollbar(canvas_container, orient=tk.VERTICAL, command=self.canvas.yview)
        h_scrollbar = ttk.Scrollbar(canvas_container, orient=tk.HORIZONTAL, command=self.canvas.xview)
        self.canvas.configure(yscrollcommand=v_scrollbar.set, xscrollcommand=h_scrollbar.set)
        v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        access_frame = ttk.Frame(canvas_frame)
        access_frame.pack(fill=tk.X, pady=(10, 0))
        ttk.Label(access_frame, text="Current Access:").pack(side=tk.LEFT)
        self.current_access_label = ttk.Label(access_frame, text="None", font=('Courier', 12, 'bold'))
        self.current_access_label.pack(side=tk.LEFT, padx=10)
        self.result_label = ttk.Label(access_frame, text="", font=('Arial', 12, 'bold'))
        self.result_label.pack(side=tk.LEFT, padx=10)
    def initialize_cache(self):
        """Initialize cache state"""
        self.cache_state = {}
        for set_idx in range(self.num_sets):
            self.cache_state[set_idx] = {}
            for way in range(self.associativity):
                self.cache_state[set_idx][way] = {
                    'valid': False,
                    'tag': None,
                    'data': None,
                    'dirty': False,
                    'lru_counter': 0,
                    'access_time': 0
                }
        self.total_accesses = 0
        self.hits = 0
        self.misses = 0
        self.draw_cache()
        self.update_metrics()
    def update_cache_config(self, event=None):
        """Update cache configuration and reinitialize"""
        try:
            self.cache_size = int(self.cache_size_var.get())
            self.block_size = int(self.block_size_var.get())
            self.associativity = int(self.associativity_var.get())
            self.replacement_policy = self.policy_var.get()
            self.num_sets = self.cache_size // (self.block_size * self.associativity)
            if self.num_sets <= 0:
                raise ValueError("Invalid cache configuration")
            if self.connector:
                try:
                    success = self.connector.configure_cache(
                        cache_size=self.cache_size,
                        block_size=self.block_size,
                        associativity=self.associativity,
                        policy=self.replacement_policy
                    )
                    if success:
                        self.backend_ready = True
                        print(f"✓ Backend configured: {self.cache_size}B, {self.block_size}B blocks, {self.associativity}-way, {self.replacement_policy}")
                    else:
                        self.backend_ready = False
                        print("⚠ Backend configuration failed")
                except Exception as e:
                    print(f"⚠ Backend configuration failed: {e}")
                    self.backend_ready = False
            self.initialize_cache()
            messagebox.showinfo("Success", f"Cache configured successfully!\n\n"
                f"Size: {self.cache_size} bytes\n"
                f"Block Size: {self.block_size} bytes\n"
                f"Associativity: {self.associativity}-way\n"
                f"Sets: {self.num_sets}\n"
                f"Policy: {self.replacement_policy}\n"
                f"Backend: {'✓ Connected' if self.backend_ready else '✗ Simulation'}")
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid configuration: {e}")
    def draw_cache(self):
        """Draw the cache visualization"""
        self.canvas.delete("all")
        cell_width = 80
        cell_height = 60
        set_label_width = 50
        way_label_height = 30
        self.canvas.create_text(set_label_width // 2, way_label_height // 2, 
                               text="Set\\Way", font=('Arial', 10, 'bold'))
        for way in range(self.associativity):
            x = set_label_width + (way + 0.5) * cell_width
            y = way_label_height // 2
            self.canvas.create_text(x, y, text=f"Way {way}", font=('Arial', 10, 'bold'))
        for set_idx in range(min(self.num_sets, 16)):
            y_base = way_label_height + set_idx * cell_height
            self.canvas.create_text(set_label_width // 2, y_base + cell_height // 2, 
                                   text=f"Set {set_idx}", font=('Arial', 9, 'bold'))
            for way in range(self.associativity):
                x_base = set_label_width + way * cell_width
                block = self.cache_state[set_idx][way]
                if not block['valid']:
                    color = 'white'
                    outline = 'gray'
                elif hasattr(self, 'current_set') and hasattr(self, 'current_way') and \
                     set_idx == self.current_set and way == self.current_way:
                    color = 'lightblue'
                    outline = 'blue'
                elif block.get('recently_accessed', False):
                    color = 'lightyellow'
                    outline = 'orange'
                else:
                    color = 'lightgreen'
                    outline = 'green'
                self.canvas.create_rectangle(
                    x_base, y_base, x_base + cell_width, y_base + cell_height,
                    fill=color, outline=outline, width=2
                )
                if block['valid']:
                    tag_text = f"Tag: {block['tag']:X}" if block['tag'] is not None else "Tag: --"
                    self.canvas.create_text(x_base + cell_width // 2, y_base + 15, 
                                           text=tag_text, font=('Arial', 8))
                    if self.replacement_policy == "LRU":
                        self.canvas.create_text(x_base + cell_width // 2, y_base + 30, 
                                               text=f"LRU: {block['lru_counter']}", font=('Arial', 8))
                    elif self.replacement_policy == "FIFO":
                        self.canvas.create_text(x_base + cell_width // 2, y_base + 30, 
                                               text=f"Time: {block['access_time']}", font=('Arial', 8))
                    if block['data'] is not None:
                        data_text = f"Data: {block['data']}"
                        self.canvas.create_text(x_base + cell_width // 2, y_base + 45, 
                                               text=data_text, font=('Arial', 8), fill='blue')
                else:
                    self.canvas.create_text(x_base + cell_width // 2, y_base + cell_height // 2, 
                                           text="EMPTY", font=('Arial', 10), fill='gray')
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))
    def simulate_cache_access(self, address, operation='R', data=None):
        """Simulate a cache access using backend or fallback simulation"""
        if self.backend_ready and self.connector:
            try:
                result = self.connector.process_access(
                    address=address, 
                    operation=operation, 
                    data=data or 0
                )
                if "error" not in result:
                    self.total_accesses = result.get('total_accesses', self.total_accesses)
                    self.hits = result.get('hits', self.hits)
                    self.misses = result.get('misses', self.misses)
                    result_info = {
                        'address': result.get('address', f"0x{address:X}"),
                        'operation': operation,
                        'set': result.get('set_index', 0),
                        'tag': result.get('tag', f"0x{address // self.block_size:X}"),
                        'hit': result.get('result', 'MISS') == 'HIT',
                        'result': result.get('result', 'MISS'),
                        'way': 0,
                        'data': data,
                        'backend_used': True
                    }
                    self.update_visualization_state(address, result_info)
                    return result_info
                else:
                    print(f"Backend error: {result.get('error', 'Unknown error')}")
            except Exception as e:
                print(f"Backend access failed: {e}")
                self.backend_ready = False
        return self.simulate_cache_access_fallback(address, operation, data)
    def simulate_cache_access_fallback(self, address, operation='R', data=None):
        """Fallback simulation when backend is unavailable"""
        self.total_accesses += 1
        block_address = address // self.block_size
        set_index = block_address % self.num_sets
        tag = block_address // self.num_sets
        for set_idx in range(self.num_sets):
            for way in range(self.associativity):
                self.cache_state[set_idx][way]['recently_accessed'] = False
        self.current_set = set_index
        self.current_way = None
        hit_way = None
        for way in range(self.associativity):
            block = self.cache_state[set_index][way]
            if block['valid'] and block['tag'] == tag:
                hit_way = way
                break
        result_info = {
            'address': f"0x{address:X}",
            'operation': operation,
            'set': set_index,
            'tag': f"0x{tag:X}",
            'hit': hit_way is not None,
            'backend_used': False
        }
        if hit_way is not None:
            self.hits += 1
            self.current_way = hit_way
            block = self.cache_state[set_index][hit_way]
            if self.replacement_policy == "LRU":
                old_counter = block['lru_counter']
                block['lru_counter'] = self.total_accesses
                for way in range(self.associativity):
                    other_block = self.cache_state[set_index][way]
                    if other_block['valid'] and other_block['lru_counter'] > old_counter:
                        other_block['lru_counter'] -= 1
            if operation == 'W':
                block['dirty'] = True
                if data is not None:
                    block['data'] = data
            block['recently_accessed'] = True
            result_info['result'] = "HIT"
            result_info['way'] = hit_way
        else:
            self.misses += 1
            victim_way = self.find_replacement_way(set_index)
            self.current_way = victim_way
            victim_block = self.cache_state[set_index][victim_way]
            if victim_block['valid'] and victim_block['dirty']:
                result_info['writeback'] = True
            victim_block['valid'] = True
            victim_block['tag'] = tag
            victim_block['dirty'] = (operation == 'W')
            victim_block['data'] = data if operation == 'W' else f"Data_{address:X}"
            victim_block['recently_accessed'] = True
            if self.replacement_policy == "LRU":
                victim_block['lru_counter'] = self.total_accesses
            elif self.replacement_policy == "FIFO":
                victim_block['access_time'] = self.total_accesses
            result_info['result'] = "MISS"
            result_info['way'] = victim_way
            result_info['evicted'] = True
        return result_info
    def update_visualization_state(self, address, result_info):
        """Update visualization state based on access result"""
        block_address = address // self.block_size
        set_index = block_address % self.num_sets
        tag = block_address // self.num_sets
        for set_idx in range(self.num_sets):
            for way in range(self.associativity):
                self.cache_state[set_idx][way]['recently_accessed'] = False
        self.current_set = set_index
        self.current_way = result_info.get('way', 0)
        if result_info['hit']:
            if set_index in self.cache_state and result_info['way'] < len(self.cache_state[set_index]):
                block = self.cache_state[set_index][result_info['way']]
                block['recently_accessed'] = True
                block['lru_counter'] = self.total_accesses
                if result_info['operation'] == 'W' and 'data' in result_info:
                    block['data'] = result_info['data']
                    block['dirty'] = True
        else:
            if set_index not in self.cache_state:
                self.cache_state[set_index] = {}
            way = result_info.get('way', 0)
            if way not in self.cache_state[set_index]:
                self.cache_state[set_index][way] = {
                    'valid': False,
                    'tag': None,
                    'data': None,
                    'dirty': False,
                    'lru_counter': 0,
                    'access_time': 0,
                    'recently_accessed': False
                }
            block = self.cache_state[set_index][way]
            block['valid'] = True
            block['tag'] = tag
            block['recently_accessed'] = True
            block['lru_counter'] = self.total_accesses
            if result_info['operation'] == 'W' and 'data' in result_info:
                block['data'] = result_info['data']
                block['dirty'] = True
            elif result_info['operation'] == 'R':
                block['data'] = f"Data_{address:X}"
    def find_replacement_way(self, set_index):
        """Find the way to replace using the current replacement policy"""
        if set_index not in self.cache_state:
            return 0
        cache_set = self.cache_state[set_index]
        for way in range(self.associativity):
            if way not in cache_set or not cache_set[way]['valid']:
                return way
        if self.replacement_policy == "LRU":
            min_counter = float('inf')
            lru_way = 0
            for way in range(self.associativity):
                if way in cache_set:
                    counter = cache_set[way].get('lru_counter', 0)
                    if counter < min_counter:
                        min_counter = counter
                        lru_way = way
            return lru_way
        elif self.replacement_policy == "FIFO":
            min_time = float('inf')
            fifo_way = 0
            for way in range(self.associativity):
                if way in cache_set:
                    access_time = cache_set[way].get('access_time', 0)
                    if access_time < min_time:
                        min_time = access_time
                        fifo_way = way
            return fifo_way
        else:
            return random.randint(0, self.associativity - 1)
        """Find the way to replace using the current replacement policy"""
        cache_set = self.cache_state[set_index]
        for way in range(self.associativity):
            if not cache_set[way]['valid']:
                return way
        if self.replacement_policy == "LRU":
            min_counter = float('inf')
            lru_way = 0
            for way in range(self.associativity):
                if cache_set[way]['lru_counter'] < min_counter:
                    min_counter = cache_set[way]['lru_counter']
                    lru_way = way
            return lru_way
        elif self.replacement_policy == "FIFO":
            min_time = float('inf')
            fifo_way = 0
            for way in range(self.associativity):
                if cache_set[way]['access_time'] < min_time:
                    min_time = cache_set[way]['access_time']
                    fifo_way = way
            return fifo_way
        else:
            return random.randint(0, self.associativity - 1)
    def load_trace_file(self):
        """Load a trace file"""
        filename = filedialog.askopenfilename(
            title="Select Trace File",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'r') as f:
                    lines = f.readlines()
                self.current_trace = []
                for line_num, line in enumerate(lines, 1):
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    parts = line.split()
                    if len(parts) >= 2:
                        operation = parts[0].upper()
                        address = int(parts[1], 16)
                        data = int(parts[2]) if len(parts) > 2 else None
                        self.current_trace.append({
                            'operation': operation,
                            'address': address,
                            'data': data,
                            'line_num': line_num
                        })
                self.trace_index = 0
                self.trace_info_label.config(text=f"Loaded: {len(self.current_trace)} accesses")
                self.reset_simulation()
            except Exception as e:
                messagebox.showerror("Error", f"Could not load trace file: {e}")
    def generate_sample_trace(self):
        """Generate a sample trace for demonstration"""
        self.current_trace = []
        base_addresses = [0x1000, 0x1040, 0x1080, 0x10C0, 0x1100, 0x1140]
        for i in range(20):
            if i < 10:
                addr = base_addresses[i % len(base_addresses)]
            else:
                addr = random.choice(base_addresses) + random.randint(0, 3) * 0x40
            operation = 'W' if random.random() < 0.3 else 'R'
            data = random.randint(100, 999) if operation == 'W' else None
            self.current_trace.append({
                'operation': operation,
                'address': addr,
                'data': data,
                'line_num': i + 1
            })
        self.trace_index = 0
        self.trace_info_label.config(text=f"Generated: {len(self.current_trace)} accesses")
        self.reset_simulation()
    def manual_access_dialog(self):
        """Open dialog for manual cache access"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Manual Cache Access")
        dialog.geometry("300x200")
        dialog.transient(self.root)
        dialog.grab_set()
        ttk.Label(dialog, text="Operation:").grid(row=0, column=0, sticky=tk.W, padx=10, pady=5)
        op_var = tk.StringVar(value="R")
        ttk.Radiobutton(dialog, text="Read", variable=op_var, value="R").grid(row=0, column=1, sticky=tk.W)
        ttk.Radiobutton(dialog, text="Write", variable=op_var, value="W").grid(row=0, column=2, sticky=tk.W)
        ttk.Label(dialog, text="Address (hex):").grid(row=1, column=0, sticky=tk.W, padx=10, pady=5)
        addr_var = tk.StringVar(value="0x1000")
        ttk.Entry(dialog, textvariable=addr_var).grid(row=1, column=1, columnspan=2, sticky=tk.EW, padx=10)
        ttk.Label(dialog, text="Data (for writes):").grid(row=2, column=0, sticky=tk.W, padx=10, pady=5)
        data_var = tk.StringVar(value="100")
        ttk.Entry(dialog, textvariable=data_var).grid(row=2, column=1, columnspan=2, sticky=tk.EW, padx=10)
        def execute_access():
            try:
                operation = op_var.get()
                address = int(addr_var.get(), 16)
                data = int(data_var.get()) if operation == 'W' and data_var.get() else None
                result = self.simulate_cache_access(address, operation, data)
                self.display_access_result(result)
                self.draw_cache()
                self.update_metrics()
                dialog.destroy()
            except ValueError as e:
                messagebox.showerror("Error", f"Invalid input: {e}")
        ttk.Button(dialog, text="Execute", command=execute_access).grid(row=3, column=1, pady=20)
        ttk.Button(dialog, text="Cancel", command=dialog.destroy).grid(row=3, column=2, pady=20)
    def toggle_animation(self):
        """Toggle animation play/pause"""
        if not self.current_trace:
            messagebox.showwarning("Warning", "Please load or generate a trace first")
            return
        self.is_playing = not self.is_playing
        if self.is_playing:
            self.play_button.config(text="⏸ Pause")
            self.animate_step()
        else:
            self.play_button.config(text="▶ Play")
    def animate_step(self):
        """Perform one step of animation"""
        if not self.is_playing or self.trace_index >= len(self.current_trace):
            self.is_playing = False
            self.play_button.config(text="▶ Play")
            return
        self.step_animation()
        delay = int(1000 / self.speed_var.get())
        self.root.after(delay, self.animate_step)
    def step_animation(self):
        """Execute one step of the trace"""
        if self.trace_index >= len(self.current_trace):
            return
        access = self.current_trace[self.trace_index]
        result = self.simulate_cache_access(access['address'], access['operation'], access['data'])
        self.display_access_result(result)
        self.draw_cache()
        self.update_metrics()
        self.trace_index += 1
        progress = (self.trace_index / len(self.current_trace)) * 100
        self.progress_var.set(progress)
    def reset_simulation(self):
        """Reset the simulation to initial state"""
        self.is_playing = False
        self.play_button.config(text="▶ Play")
        self.trace_index = 0
        self.progress_var.set(0)
        if self.backend_ready and self.connector:
            try:
                self.connector.reset_simulator()
                print("✓ Backend reset")
            except Exception as e:
                print(f"⚠ Backend reset failed: {e}")
        self.initialize_cache()
        self.current_access_label.config(text="None")
        self.result_label.config(text="")
    def display_access_result(self, result):
        """Display the result of a cache access"""
        access_text = f"{result['operation']} {result['address']}"
        self.current_access_label.config(text=access_text)
        if result['hit']:
            result_text = f"✅ HIT (Way {result['way']})"
            self.result_label.config(text=result_text, foreground='green')
        else:
            result_text = f"❌ MISS (Way {result['way']})"
            if result.get('writeback'):
                result_text += " + Writeback"
            self.result_label.config(text=result_text, foreground='red')
    def update_metrics(self):
        """Update performance metrics display"""
        hit_rate = (self.hits / self.total_accesses * 100) if self.total_accesses > 0 else 0
        miss_rate = 100 - hit_rate
        metrics_text = f"""Performance Metrics:
Total Accesses: {self.total_accesses}
Cache Hits: {self.hits}
Cache Misses: {self.misses}
Hit Rate: {hit_rate:.1f}%
Miss Rate: {miss_rate:.1f}%
Cache Configuration:
Size: {self.cache_size} bytes
Block Size: {self.block_size} bytes
Associativity: {self.associativity}-way
Sets: {self.num_sets}
Policy: {self.replacement_policy}
Progress: {self.trace_index}/{len(self.current_trace) if self.current_trace else 0}
"""
        self.metrics_text.delete(1.0, tk.END)
        self.metrics_text.insert(tk.END, metrics_text)
def main():
    root = tk.Tk()
    CacheVisualizer(root)
    root.mainloop()
if __name__ == "__main__":
    main()