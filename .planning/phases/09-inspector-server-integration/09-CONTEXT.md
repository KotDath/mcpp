# Phase 9: Inspector Server Integration - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Update the example server to use the new JsonRpcRequest::from_json() validation layer and ensure compatibility with the MCP Inspector CLI tool. Create mcp.json configuration and verify manual testing workflow.

</domain>

<decisions>
## Implementation Decisions

### Example server structure
- Modify the existing stdio_example.cpp (do NOT create a separate example)
- Replace ALL raw nlohmann::json parsing with from_json() throughout
- Keep the example simple/monolithic (single file structure)
- Update ALL examples (stdio, HTTP, SSE) for consistency

### Parse error handling
- Pre-scan for 'id' field in raw JSON before full validation (best-effort extraction)
- Add ID extraction helper to JsonRpcRequest class (not separate utility, not inline)
- Return null id if the id field exists but is malformed (wrong type)
- Extracted id is used in error responses when from_json() fails

### mcp.json configuration
- Command structure: command with args (paths relative to mcp.json location)
- File location: examples directory (not project root)
- mcp.json should be committed to the repo (not CMake-generated)
- References the built example server executable

### Testing workflow
- Required Inspector commands: tools/list, tools/call, prompts/list, resources/list
- Documentation: Separate TESTING.md file in examples directory
- Local testing only (no CI integration for manual Inspector tests)
- Assume user has MCP Inspector installed locally

### Claude's Discretion
- Exact format of TESTING.md (what sections, how much detail)
- Specific example tool/prompt/resource for testing
- Error message wording in testing documentation

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard MCP Inspector testing practices.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 09-inspector-server-integration*
*Context gathered: 2026-02-01*
