# Phase 11: Documentation - Research

**Researched:** 2026-02-01
**Domain:** Technical documentation for MCP testing and integration
**Confidence:** HIGH

## Summary

Phase 11 requires updating the README with MCP Inspector testing instructions and creating a testing guide for writing custom Inspector tests. Research reveals that **effective MCP documentation** follows specific patterns: quick start examples first, code-first explanations, and inline ASCII diagrams for protocol visualization.

The key insight is that mcpp's audience (intermediate+ C++ developers) needs **practical examples over theoretical explanations**. The CONTEXT.md decisions lock in specific documentation structures: linear narrative (What -> Why -> Quick start -> Build -> Testing -> API docs), code-first approach with FAQ-style troubleshooting, and module references rather than file paths.

Existing documentation patterns from the MCP ecosystem show that:
1. **README.md** serves as the entry point - must include quick start that works immediately
2. **Testing guides** work best as "learn by example" walkthroughs of actual test files
3. **ASCII diagrams** effectively explain the stdio transport and JSON-RPC message flow
4. **Inspector integration** requires both UI and CLI mode documentation

The project already has 7 BATS test files in `tests/cli/` that demonstrate the testing patterns, plus the `inspector_server.cpp` example showing a complete MCP server implementation.

**Primary recommendation:** Create two documents: (1) Update README.md with Inspector testing section before the current "Testing" section, (2) Create TESTING.md that walks through the existing BATS test files as examples, with FAQ-style troubleshooting.

## Standard Stack

### Core

| Tool/Library | Version | Purpose | Why Standard |
|-------------|---------|---------|--------------|
| Markdown | GitHub Flavored | Documentation format | Universal, renders on GitHub, supports code blocks |
| ASCII diagrams | text-only | Protocol/architecture visualization | Works everywhere, no image dependencies |
| jq examples | 1.6+ | JSON validation examples | Matches Phase 10 test infrastructure |

### Supporting

| Format | Purpose | When to Use |
|--------|---------|-------------|
| Code blocks with syntax highlighting | C++ examples | All code snippets |
| Bash session examples | CLI testing commands | Inspector usage |
| JSON-RPC snippets | Protocol examples | Request/response illustrations |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| README updates | Separate docs/ directory | Less discoverable, users expect README |
| ASCII diagrams | Image-based diagrams | Rendering issues, external dependencies |
| Inline code examples | External code links | Context switching, harder to follow |

**No installation required** - documentation is plain text/Markdown.

## Architecture Patterns

### Recommended Documentation Structure

The CONTEXT.md decisions lock in this README structure:

```
README.md:
├── Title + brief description (2-3 sentences on MCP)
├── Features (bullet list)
├── Requirements
├── Quick Start (working code example FIRST)
├── Building (full section, not just link)
├── Inspector Testing (NEW - Phase 11 addition)
│   ├── What is Inspector?
│   ├── UI mode setup
│   ├── CLI mode usage
│   └── Example session
├── Testing (existing - ctest instructions)
├── Examples (link to examples/)
├── Installation
├── Project Structure
├── Contributing
└── Resources (links to MCP spec, Inspector, JSON-RPC)

TESTING.md (NEW - Phase 11 creation):
├── Overview (code-first philosophy)
├── Test Infrastructure (BATS, helpers)
├── Reading Existing Tests
│   ├── 01-tools-list.bats walkthrough
│   ├── 02-tools-call.bats walkthrough
│   ├── Common patterns (setup/teardown, JSON validation)
├── Writing Custom Tests
│   ├── Test file template
│   ├── Helper functions available
│   ├── Best practices
├── CMake integration (CTest)
└── Troubleshooting FAQ (5-10 common issues)
```

### Pattern 1: Quick Start First

**What:** Working code example appears before any explanation, building user confidence immediately.

**When to use:** README must begin with code that runs successfully.

**Example:**
```markdown
## Quick Start

```bash
# Clone and build
git clone https://github.com/mcpp-project/mcpp.git
cd mcpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Test with MCP Inspector (see Inspector Testing section below)
./build/examples/inspector_server
```
```

### Pattern 2: ASCII Diagrams for Protocol Flow

**What:** Text-based diagrams showing JSON-RPC message flow over stdio.

**When to use:** Explaining MCP stdio transport and Inspector communication.

**Example:**
```text
MCP Inspector → Server Communication (stdio mode):

┌─────────────┐                    ┌──────────────┐
│  Inspector  │                    │  Your Server │
│   (Client)  │                    │ (stdio mode) │
└──────┬──────┘                    └──────┬───────┘
       │                                  │
       │  1. initialize request           │
       │ ───────────────────────────────>│
       │                                  │
       │  2. initialize response          │
       │ <───────────────────────────────│
       │                                  │
       │  3. tools/list request           │
       │ ───────────────────────────────>│
       │                                  │
       │  4. tools/list response          │
       │ <───────────────────────────────│
       │                                  │

All messages are JSON-RPC 2.0 formatted:
{"jsonrpc":"2.0","id":1,"method":"tools/list"}
```

### Pattern 3: Code-First Testing Guide

**What:** Present test code first, explain concepts after they've seen the pattern.

**When to use:** TESTING.md - show actual test file content, then explain what each part does.

**Example:**
```markdown
## Reading Existing Tests

The best way to learn is to examine real tests. Let's walk through
`tests/cli/01-tools-list.bats`:

```bash
#!/usr/bin/env bats
# Load common setup for test environment
load 'test_helper/common-setup'

setup() {
    _common_setup
}

@test "tools/list returns valid JSON-RPC response" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",...}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}"
    } | inspector_server 2>/dev/null | tail -1'

    assert_success
    assert_output --partial '"jsonrpc":"2.0"'
}
```

**What's happening:**
- `load 'test_helper/common-setup'` - loads shared test environment
- `setup()` - runs before each test, sets PATH and MCPP_DEBUG
- `run bash -c '...'` - executes the command, captures output
- `assert_success` - verifies exit code was 0
- `assert_output --partial` - checks output contains substring
```

### Pattern 4: FAQ-Style Troubleshooting

**What:** Common problems with symptoms and solutions.

**When to use:** TESTING.md troubleshooting section.

**Example:**
```markdown
## Troubleshooting

### Server not starting after code changes

**Symptoms:**
```
inspector_server: command not found
# OR tests fail with "connection refused"
```

**Solution:** Always rebuild after C++ code changes:
```bash
cmake --build build
```

### Tests timing out

**Symptoms:** BATS tests hang indefinitely.

**Cause:** Server not responding, stdio buffering issue.

**Solution:** Check MCPP_DEBUG output:
```bash
MCPP_DEBUG=1 bats tests/cli/01-tools-list.bats
```
```

### Anti-Patterns to Avoid

- **File paths in documentation:** Use module references ("See the server module") not paths ("See src/mcpp/server/")
- **External code links:** Keep examples inline, don't link to code that might change
- **Theoretical explanations:** Code-first, explain concepts as they appear
- **Assuming prior MCP knowledge:** Briefly explain MCP concepts as they relate to mcpp
- **Separate "concepts" docs:** Keep all docs in README and TESTING.md

## Don't Hand-Roll

Documentation problems that have established solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Protocol diagrams | Complex text descriptions | ASCII art with box characters | Universal rendering, no dependencies |
| JSON-RPC examples | Verbal descriptions | Actual JSON snippets | Copy-pasteable, unambiguous |
| Testing walkthroughs | Abstract test patterns | Real test files from tests/cli/ | Proven to work, immediate reference |
| Inspector setup | Custom instructions | Official `npx @modelcontextprotocol/inspector` command | Standard MCP workflow |

**Key insight:** Documentation should reference existing, working examples. The tests/cli/ directory already contains 7 working BATS test files covering all MCP endpoints - use these as the primary teaching material.

## Common Pitfalls

### Pitfall 1: Explanations Before Examples

**What goes wrong:** Readers see paragraphs of theory before any working code, lose interest.

**Why it happens:** Traditional documentation order (concepts -> examples).

**How to avoid:** Code first. Show working example, then explain what it does.

**Warning signs:** Documentation starts with "MCP is a protocol that..." before any code block.

### Pitfall 2: File Paths That Become Stale

**What goes wrong:** Documentation references "src/mcpp/server/mcp_server.h" but file moved during refactoring.

**Why it happens:** File paths change during development; docs aren't updated.

**How to avoid:** Use module references ("See the server module") or link to generated docs.

**Solution:**
```markdown
# BAD:
See src/mcpp/server/mcp_server.h for McpServer class

# GOOD:
See the McpServer class in the server module
```

### Pitfall 3: Inspector UI vs CLI Confusion

**What goes wrong:** User tries to use UI mode commands for CLI testing, gets confused.

**Why it happens:** Inspector has both UI (browser) and CLI modes with different invocation patterns.

**How to avoid:** Clearly separate UI mode instructions from CLI mode in documentation.

**Solution:**
```markdown
## Inspector Testing

### UI Mode (Interactive, Browser-based)

For manual testing and interactive exploration:

```bash
# UI mode opens browser automatically
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server
```

### CLI Mode (Automated, Scriptable)

For CI/CD and scripting:

```bash
# CLI mode returns JSON to stdout
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server --method tools/list
```
```

### Pitfall 4: Not Explaining the Initialize Handshake

**What goes wrong:** Users send tools/list without initialize first, get empty results.

**Why it happens:** MCP protocol requires initialize before most operations; not obvious to newcomers.

**How to avoid:** Document the initialize requirement prominently, show it in all examples.

**Solution:**
```markdown
**Important:** MCP requires initialize handshake before most operations:

```bash
# First: initialize
echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}' | server

# Then: actual request
echo '{"jsonrpc":"2.0","id":2,"method":"tools/list"}' | server
```

Both requests can be piped together:
```bash
{ echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}'
  echo '{"jsonrpc":"2.0","id":2","method":"tools/list"}'
} | server
```
```

## Code Examples

Verified patterns from official MCP documentation and existing tests:

### Inspector UI Mode

```markdown
## Testing with MCP Inspector (UI Mode)

MCP Inspector is an interactive testing tool. Start your server with Inspector:

```bash
# Build the example server first
cmake --build build --target inspector_server

# Connect via Inspector (opens browser)
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server
```

The Inspector will:
1. Open your browser to http://localhost:6274
2. Display available tools (calculate, echo, get_time, server_info)
3. Show resources and prompts
4. Allow interactive tool testing

**Source:** https://github.com/modelcontextprotocol/inspector
```

### Inspector CLI Mode

```markdown
## Testing with MCP Inspector (CLI Mode)

For automated testing or scripting, use CLI mode:

```bash
# List available tools
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server --method tools/list

# Call a tool
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
    --method tools/call --tool-name calculate --tool-arg operation=add --tool-arg a=5 --tool-arg b=3

# List resources
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server --method resources/list
```

**Source:** https://github.com/modelcontextprotocol/inspector#cli-mode
```

### BATS Test Template

```bash
#!/usr/bin/env bats
# mcpp - MCP C++ library tests
# https://github.com/mcpp-project/mcpp
#
# Test file for [feature description]
# Tests [what this test file covers]

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Optional: teardown for cleanup
teardown() {
    # Cleanup runs even on test failure
    # Common use: kill background processes
}

@test "[descriptive test name]" {
    # Send initialize + request
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},"clientInfo":{"name":"test","version":"1.0"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"methods/name\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    # Verify success
    assert_success

    # Verify response structure
    assert_output --partial '"jsonrpc"'
    assert_output --partial '"result"'

    # Verify specific content using jq
    echo "$output" | jq -e '.result.field == "expected"' >/dev/null
    assert_equal $? 0
}
```

### README Quick Start Section

```markdown
## Quick Start

```bash
# Clone the repository
git clone https://github.com/mcpp-project/mcpp.git
cd mcpp

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Test with MCP Inspector
cmake --build build --target inspector_server
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server
```

The inspector_server demonstrates:
- **Tools:** calculate, echo, get_time, server_info
- **Resources:** file reading, server information
- **Prompts:** code review template, greeting
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Separate concepts + examples docs | Code-first with inline explanation | ~2020+ documentation trends | Better engagement, faster onboarding |
| Image-based diagrams | ASCII art for protocol diagrams | Ongoing best practice | Universal rendering, version control friendly |
| External examples in separate repo | Inline examples referencing existing tests | Current MCP SDK pattern | Easier maintenance, always in sync |

**Deprecated/outdated:**
- **Separate "Getting Started" guide:** Merge into README quick start
- **File paths in docs:** Use module references to avoid stale links
- **PDF/doc based docs:** Plain Markdown is the standard

## Open Questions

1. **Inspector version pinning**
   - What we know: Official docs show `npx @modelcontextprotocol/inspector` without version
   - What's unclear: Should we recommend specific version for stability?
   - Recommendation: Use latest (no version pin) - Inspector is actively developed and backward compatible

2. **TESTING.md location**
   - What we know: Could be at root docs/TESTING.md or in examples/TESTING.md
   - What's unclear: Which location is more discoverable?
   - Recommendation: Root TESTING.md - applies to entire project, not just examples

3. **Diagram complexity**
   - What we know: ASCII diagrams help understand stdio transport
   - What's unclear: How detailed to make the protocol flow diagrams?
   - Recommendation: Keep diagrams simple - focus on message flow, not every field

## Sources

### Primary (HIGH confidence)
- [MCP Inspector Official Documentation](https://modelcontextprotocol.io/docs/tools/inspector/) - Installation, UI vs CLI modes, features
- [MCP Inspector GitHub Repository](https://github.com/modelcontextprotocol/inspector) - CLI usage, authentication, configuration
- [Existing mcpp tests/cli/ directory](/home/kotdath/omp/personal/cpp/mcpp/tests/cli/) - 7 working BATS test files demonstrating all patterns
- [Existing inspector_server.cpp](/home/kotdath/omp/personal/cpp/mcpp/examples/inspector_server.cpp) - Complete MCP server example

### Secondary (MEDIUM confidence)
- [Rust MCP SDK TESTING_GUIDE.md](/home/kotdath/omp/personal/cpp/mcpp/thirdparty/gopher-mcp/sdk/rust/TESTING_GUIDE.md) - Reference for testing guide structure
- [BATS-core documentation](https://bats-core.readthedocs.io/en/stable/) - Test structure and patterns (referenced from Phase 10 research)

### Tertiary (LOW confidence)
- [Community MCP testing blog posts](https://mydeveloperplanet.com/2025/12/01/testing-mcp-servers-with-mcp-inspector/) - Additional context on Inspector usage patterns

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Markdown and ASCII diagrams are universal standards
- Architecture: HIGH - Context.md decisions lock in specific structure; existing tests provide verified patterns
- Pitfalls: MEDIUM - Documentation pitfalls are well-known but specific MCP/MCPP patterns need validation

**Research date:** 2026-02-01
**Valid until:** 2026-03-01 (30 days - documentation patterns are stable)
