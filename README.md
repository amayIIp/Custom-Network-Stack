# Custom Network Stack & High-Performance Financial Parsers

Welcome to the **Custom Network Stack** repository. This project is a suite of low-latency networking and trading infrastructure components designed for high-frequency trading (HFT) and quantitative engineering. 

The primary component in this repository is an **ultra-low latency, zero-copy Nasdaq TotalView-ITCH 5.0 Parser** implemented in C++20. It achieves parsing speeds of **under 10 nanoseconds per message** (>100 million messages/sec) through aggressive hardware and compiler-level optimizations.

---

## Repository Structure

```
Custom-Network-Stack/
├── .gitignore
├── README.md                  # Main project guide
└── itch-parser/               # Nasdaq ITCH 5.0 parser project
    ├── CMakeLists.txt         # Build configuration
    ├── README.md              # Detailed parser design & cycle-level profiling documentation
    ├── include/               # Header-only high-performance library
    │   ├── byteswap.hpp       # Fast big-endian to host byte swapping
    │   ├── itch_messages.hpp  # Packed C++ structs matching the ITCH 5.0 binary protocol
    │   ├── itch_parser.hpp    # Switch-based and Jump-Table-based parser dispatchers
    │   ├── mmap_file.hpp      # Portable Windows and POSIX memory-mapped file wrapper
    │   ├── order_book.hpp     # Fast order book with price-level and bid/ask cross tracking
    │   └── parser.hpp         # High-level runner declarations
    ├── src/                   # Source files
    │   ├── main.cpp           # CLI parser harness
    │   └── parser.cpp         # Memory-mapped file runners
    ├── bench/                 # Benchmark suite
    │   ├── bench_parser.cpp   # Warmup + 10 timed runs benchmark harness
    │   ├── naive_parser.cpp   # Slower baseline parser (ifstream, heap copies, map lookup)
    │   └── naive_parser.hpp   # Naive parser declarations
    └── tests/                 # Unit tests & verification
        ├── test_parser.cpp    # C++ unit tests verifying bytes, fields, and bounds safety
        └── verify.py          # Reference Python verification script using struct.unpack
```

---

## ITCH 5.0 Parser Performance Summary

When tested against a **65MB raw binary Nasdaq ITCH 5.0 feed** containing **2,243,750 messages**, the C++20 implementation achieved the following median parsing latencies (compiled using Clang with `-O3 -march=native -flto` on Raptor Lake hardware):

| Parser Implementation | Throughput (Million msgs/sec) | Avg Latency (ns/msg) | Speedup vs Naive | Memory Allocation | Dispatch Mechanism |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Naive Baseline** | 4.68 M/s | 213.54 ns | *Baseline* | Heap (`std::vector`) | `std::unordered_map` + `std::function` |
| **Optimized Switch** | 103.21 M/s | 9.69 ns | **22.0x** | **Zero-Copy** (mmap) | `switch` statement (compiler branches) |
| **Optimized Jump Table** | **108.33 M/s** | **9.23 ns** | **23.1x** | **Zero-Copy** (mmap) | **Branchless Jump-Table** |
| **Order Book Building** | 6.44 M/s | 155.35 ns | **1.37x** (with state) | Map/Set Overhead | Branchless Jump-Table |

*Note: The optimized parsers parse in-memory once the memory-mapped file pages are fully warmed up in cache, bypassing cold disk I/O faults.*

---

## Key Optimization Techniques

1. **Memory-Mapped Files (`mmap` / `MapViewOfFile`):** Avoids user-space buffering and system call read overhead by mapping the binary file directly to virtual memory.
2. **Zero-Copy Pointer Overlays:** Structs are marked with `#pragma pack(push, 1)` to match the Nasdaq binary layout exactly. The parser casts pointers directly into the mapped region, creating zero heap allocations or structure copies.
3. **Branchless Jump-Table Dispatch:** Bypasses conditional branches by executing callbacks using an array-indexed jump table (`m_table[msg[0]]`), completely eliminating branch misprediction penalties.
4. **Data Prefetching:** Utilizes cache line prefetching (`__builtin_prefetch`) to load future messages into L1/L2 caches while the CPU is busy parsing the current one.
5. **Template-Based Inline Callbacks:** Avoids runtime polymorphism (virtual functions) by using compile-time template arguments to inline handlers directly into the hot parsing loops.

---

## Getting Started

### Prerequisites
* A C++20 compatible compiler (e.g., Clang 16+, GCC 11+, or MSVC 2022+)
* CMake (Version 3.15+)
* Build tool (e.g., Ninja or Make)

### How to Build (CMake)

1. Clone this repository and navigate to the project directory:
   ```bash
   git clone https://github.com/amayIIp/Custom-Network-Stack.git
   cd Custom-Network-Stack
   ```

2. Generate build files and compile the project (using Clang & Make in this example):
   ```bash
   cmake -S itch-parser -B itch-parser/build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=clang++
   cmake --build itch-parser/build
   ```

This will produce three executables under `itch-parser/build/`:
* `test_parser.exe` (Unit tests)
* `bench_parser.exe` (Benchmark suite)
* `itch_parser.exe` (CLI parser tool)

---

## How to Run

### 1. Run Unit Tests
Verifies that message layouts, byteswapping, and bounds-checking parse exactly as specified:
```bash
./itch-parser/build/test_parser.exe
```

### 2. Run the Parser CLI
Processes a raw ITCH 5.0 binary file using a specified mode (`naive`, `switch`, `table`, or `book`):
```bash
./itch-parser/build/itch_parser.exe table path/to/data.bin
```

### 3. Run the Benchmark Suite
Executes 2 warmup runs and 10 timed runs for all three parser implementations and prints the median latency metrics:
```bash
./itch-parser/build/bench_parser.exe path/to/data.bin
```

---

## Future Goals / Roadmap

* [ ] **MoldUDP64 Multicast Receiver:** Implement a socket-level UDP multicast receiver to parse ITCH packets off the wire in real-time.
* [ ] **SIMD FIX Parser:** Utilize SIMD vector instructions (`_mm_cmpeq_epi8` / `_mm_movemask_epi8`) to perform ultra-fast tag-value scanning for the FIX protocol.
* [ ] **AF_XDP/DPDK Integration:** Direct network card buffer access bypasses the OS kernel completely, bringing wire-to-application parser latencies to the sub-microsecond level.
