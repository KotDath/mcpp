# MCP C++ Test Scripts

This directory contains test scripts for the MCP C++ library.

## Available Scripts

### 🧪 Core Testing Scripts

- **`comprehensive_test.sh`** - Complete testing of all MCP methods
  - Tests ping, echo, initialize requests
  - Tests notifications
  - Shows JSON responses

- **`simple_test.sh`** - Quick ping test
  - Sends a single ping request
  - Shows basic server response

### 🔧 Advanced Testing Scripts

- **`test_fifo.sh`** - Testing with named pipes (FIFO)
  - Creates bidirectional communication
  - Tests concurrent request handling

- **`test_server.sh`** - Basic server testing
  - Sends multiple test requests
  - Shows full server logs

- **`test_nc.sh`** - Testing with netcat
  - Tests network-like communication
  - Uses TCP sockets for testing

## Usage

All scripts assume they are run from the `scripts/` directory:

```bash
cd scripts/
./comprehensive_test.sh
./simple_test.sh
```

## Requirements

- MCP library must be built (`make` in root directory)
- `build/examples/echo_server` must exist
- Standard Unix tools: `timeout`, `grep`, `mkfifo`, `nc`

## Script Details

### comprehensive_test.sh
Tests:
1. `ping` request
2. `echo` request with custom message
3. `initialize` request with full parameters
4. `notifications/initialized` (no response expected)

### simple_test.sh
- Single ping request
- Filters logs to show only JSON response
- Good for quick validation

### test_fifo.sh
- Creates named pipes for communication
- Tests server with simultaneous requests
- More realistic communication pattern

### test_nc.sh
- Uses netcat for TCP-like testing
- Tests network transport scenarios
- Good for debugging transport issues

## Output

Scripts show:
- Request being sent
- Server response (JSON only)
- Success/failure status

Server logs are redirected to `/dev/null` to focus on protocol testing.