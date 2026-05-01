# Codebase Improvement Plan

## Phase 1: Code Quality & Structure (Weeks 1-3)

### 1.1 Architecture & Organization
```
Current:  flat structure, apps call classes directly
Target:   layered architecture with clear separation
```

| Task | Description | Files |
|------|-------------|-------|
| Create `include/` directory | Public headers | Move .h files |
| Create `lib/` directory | Core library code | src/*.cpp → lib/ |
| Create `cli/` directory | CLI utilities | Argument parsing |
| Refactor apps to use library | Apps become thin wrappers | All app/*.cpp |

### 1.2 Error Handling
```
Current:  return 1 on failure, minimal error messages
Target:   proper error codes, logging, recovery
```

| Task | Description |
|------|-------------|
| Create `Result<T>` type | Handle success/failure uniformly |
| Add logging system | Configurable log levels (debug, info, warn, error) |
| Add validation functions | Input mesh validation, parameter bounds checking |
| Create exception hierarchy | Specific exception types for different failures |

### 1.3 Configuration
```
Current:  hardcoded constants throughout
Target:   configuration system with defaults and overrides
```

| Task | Description |
|------|-------------|
| Add argparse library | C++ header-only CLI parser (IMPLEMENTED) |
| Create config files | JSON for complex settings |
| Add --help to all apps | Document all options |
| Environment variables | Override config values |

---

## Phase 2: Testing & Reliability (Weeks 4-6)

### 2.1 Unit Tests
```
Current:  basic assert-based tests
Target:   comprehensive test suite with coverage
```

| Task | Description | Target |
|------|-------------|--------|
| Add Google Test | Test framework | 100% |
| Add coverage reporting | Track line/branch coverage | >80% |
| Add memory sanitizers | Detect leaks | CI runs |
| Test mesh validation | Invalid input handling | All edge cases |
| Test algorithms | Correctness, not just "runs" | Key algorithms |

### 2.2 Integration Tests
| Task | Description |
|------|-------------|
| Multi-mesh tests | assembly_clearance, extraction_path, etc. |
| Round-trip tests | Load → process → save → load |
| Performance tests | Regression detection |

---

## Phase 3: Performance (Weeks 7-9)

### 3.1 Profiling & Optimization
| Task | Description |
|------|-------------|
| Add timing utilities | Measure component performance |
| Identify bottlenecks | Profile with Instruments/Valgrind |
| Optimize hot paths | Critical ray casting loops |
| Add caching | ReuseEmbree scenes where possible |

### 3.2 Memory Management
| Task | Description |
|------|-------------|
| Add memory tracking | Leak detection |
| Use smart pointers | RAII for Embree resources |
| Object pooling | Reduce allocation overhead |

---

## Phase 4: Documentation (Weeks 10-12)

### 4.1 API Documentation
| Task | Description |
|------|-------------|
| Add Doxygen | Auto-generate API docs |
| Document public API | All headers in include/ |
| Add design docs | Architecture decisions |

### 4.2 User Documentation
| Task | Description |
|------|-------------|
| Improve UserGuide.md | More examples, troubleshooting |
| Add tutorials | Common workflows |
| Create website | Host docs online |

---

## Phase 5: Engineering (Weeks 13-16)

### 5.1 Build System
| Task | Description |
|------|-------------|
| Add CI/CD | GitHub Actions |
| Add package managers | Conan/vcpkg support |
| Version management | Semantic versioning |
| Add pre-commit hooks | Format, lint |

### 5.2 Code Standards
| Task | Description |
|------|-------------|
| Add clang-format | Code style enforcement |
| Add clang-tidy | Lint rules |
| Add cppcheck | Static analysis |
| Create style guide | Project conventions |

---

## Implementation Order

```
Phase 1 (Weeks 1-3):
├── 1.1.1 Create include/ and move headers
├── 1.1.2 Create lib/ and restructure
├── 1.1.3 Add CLI11 and refactor apps
├── 1.2.1 Create Result<T> type
├── 1.2.2 Add logging system
└── 1.3.1 Add CLI argument parsing

Phase 2 (Weeks 4-6):
├── 2.1.1 Add Google Test
├── 2.1.2 Add coverage reporting
├── 2.1.3 Write algorithm correctness tests
└── 2.2.1 Add integration tests

Phase 3 (Weeks 7-9):
├── 3.1.1 Add timing utilities
├── 3.1.2 Profile and optimize
├── 3.2.1 Add memory tracking
└── 3.2.2 Use smart pointers

Phase 4 (Weeks 10-12):
├── 4.1.1 Add Doxygen
├── 4.1.2 Document public API
├── 4.2.1 Improve UserGuide
└── 4.2.2 Add tutorials

Phase 5 (Weeks 13-16):
├── 5.1.1 Add CI/CD
├── 5.1.2 Add package support
├── 5.2.1 Add format/lint
└── 5.2.2 Create style guide
```

---

## Quick Wins (Do First)

1. **Add argparse** - ✅ IMPLEMENTED (header-only library in include/argparse/)
2. **Add logging** - Helps debugging immediately  
3. **Add --help to all apps** - Low effort, high value (mass_properties done as example)
4. **Add Google Test** - Enables proper testing
5. **Add clang-format** - Keeps code consistent

---

## Estimated Timeline

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Phase 1 | 3 weeks | Structured, configurable codebase |
| Phase 2 | 3 weeks | Comprehensive test suite |
| Phase 3 | 3 weeks | Optimized performance |
| Phase 4 | 3 weeks | Complete documentation |
| Phase 5 | 4 weeks | Production-ready engineering |

**Total: ~16 weeks** for full production-grade quality

---

## Progress Tracking

| Metric | Current | Target |
|--------|---------|--------|
| Test coverage | ~80% (shell only) | >80% (unit tests) |
| Apps with --help | 1 (example) | 100% |
| Error handling | Minimal | Complete |
| Configuration | Hardcoded | Configurable |
| Documentation | Basic | Complete |
| CLI parsing | ✅ argparse (1 app) | All apps |

---

## Implementation Notes

### argparse Library
- Location: `include/argparse/argparse.h`
- Header-only, no external dependencies
- Supports positional and optional arguments
- Supports short (-r) and long (--resolution) flags
- Usage example in `app/mass_properties.cpp`

### Next Steps
1. Refactor more apps to use argparse
2. Add logging system
3. Continue with Phase 2 (Testing)