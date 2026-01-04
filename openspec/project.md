# Project Context

## Purpose
LocalPhotoSync is a LAN-based photo synchronization daemon/service written in C++. It enables clients to discover and sync photos to a local server using UDP broadcasts for discovery and TCP for reliable file transfer with deduplication via CRC32 checksums.

## Tech Stack
- **Language**: C++17/C++20
- **Networking**: Boost.Asio (async I/O)
- **Serialization**: Protobuf
- **Storage**: SQLite3 (per-directory metadata)
- **Protocol**: Custom length-prefixed Protobuf framing (4-byte big-endian length + payload)
- **Build**: CMake 3.15+
- **Testing**: GoogleTest (gtest)
- **Dependencies**: Boost (system), zlib, gflags

## Project Conventions

### Code Style
- **Formatting**: clang-format with .clang-format config at repo root
- **Naming**: snake_case for variables/functions, PascalCase for classes
- **Indentation**: 2 spaces (configurable via .clang-format)
- **Headers**: Guard with `#pragma once`
- **Includes**: Sort system → local; separate with blank line

### Architecture Patterns
- **Async I/O**: Boost.Asio-based event-driven architecture
- **Session-based**: Each TCP connection = one `Session` object managing device binding and photo sync
- **Per-directory DB**: Each sync path has independent `index.db` (SQLite) storing filename→CRC32 mappings
- **Message framing**: Length-prefixed Protobuf (4-byte big-endian + payload)
- **Singleton services**: `ServiceContainer` manages UDP broadcaster, TCP server, shared resources

### Testing Strategy
- **Framework**: GoogleTest (gtest)
- **Location**: `tests/` directory, organized by component
- **Coverage**: Minimum 90%
- **Build flag**: `-DENABLE_TESTS` for selective test building
- **Running**: `build/lps_unit_tests` or individual test binaries
- **Convention**: Test file = `test_<component>.cpp`; test class = `<Component>Test`

### Git Workflow
- **Branching**: Feature branches for major work; main branch protected
- **Commits**: Conventional Commits (feat:, fix:, refactor:, test:, docs:)
- **CI/CD**: Run `build.sh` + tests before push
- **Code review**: OpenSpec proposal for breaking changes, feature additions, architecture shifts

## Domain Context

### Network Protocol
- **UDP Broadcast**: Server broadcasts `CSNtyServerInfo` every second (TCP port, name, root path, OS)
- **TCP Messages**: 
  - `CSReqDeviceInfo`: Client registers device and sync path
  - `CSReqSyncPhoto`: Client uploads photo chunk with CRC32, supports multi-part
  - Server deduplicates by matching file-level CRC32 against `index.db`
  
### Storage Model
- **Root**: Configurable via `LPS_ROOT` (default: `./data`)
- **Paths**: `LPS_ROOT/<client-provided-path>/` (auto-created multi-level)
- **Metadata DB**: Each directory has `index.db` with schema `photos(filename PRIMARY KEY, crc32 INTEGER)`
- **Dedup**: If file CRC matches DB record, skip write; if missing or CRC differs, write new file

### Configuration
- Environment variables: `LPS_TCP_PORT`, `LPS_UDP_PORT`, `LPS_NAME`, `LPS_ROOT`, `LPS_OS`, `LPS_BROADCAST`
- Defaults reasonable for single-server deployment
- No config file persistence (env-var only)

## Important Constraints
- **Concurrency**: Same file concurrent writes must be serialized by client; server does NOT implement file-level locks
- **Error handling**: Parse failures, invalid frames, I/O errors close session (no reconnect logic in protocol)
- **SQLite safety**: Each `Session` opens dedicated DB handle; no cross-session DB locking
- **Prototype**: CRC-based dedup is simple; production may need stronger content-hash or chunking
- **Single-server assumption**: Broadcast/sync assumes one server; no cluster/replication

## External Dependencies
- **Boost**: System library only (asio header-only)
- **Protobuf**: `libprotobuf` + `protoc` compiler
- **SQLite3**: C library
- **zlib**: Compression (optional but included in build)
