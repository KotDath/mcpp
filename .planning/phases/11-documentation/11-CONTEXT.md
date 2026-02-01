# Phase 11: Documentation - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can run Inspector tests and write custom integration tests. This phase creates documentation that enables users to:
1. Test their MCP servers using MCP Inspector CLI
2. Understand the existing BATS test infrastructure
3. Write their own integration tests

</domain>

<decisions>
## Implementation Decisions

### README structure
- Quick start example appears first (minimal working code)
- Multiple examples: simple hello-world start, then link to examples/ directory for fuller patterns
- Full build section included in README (not just brief + link)
- Linear narrative structure: What → Why → Quick start → Build → Testing → API docs
- Inspector testing instructions as top-level README section (not separate doc)
- Inline reference for key classes and methods (not just link to generated docs)
- Concise overview: 2-3 sentences on what MCP is and why C++ developers might need it

### Testing guide depth
- Primary purpose: Learn by example (walk through existing tests to understand the library)
- Code-first approach: start with test example, explain concepts as they appear
- Common issues troubleshooting: FAQ-style with 5-10 common problems (Server not starting, Tests timing out, etc.)
- Covers both reading existing tests AND writing new ones (read and write)

### Audience assumptions
- C++ experience: Intermediate+ (comfortable with templates, smart pointers, async patterns)
- MCP protocol: Explain briefly as concepts relate to using mcpp (tools, resources, prompts)
- Build system: Assume readers know CMake
- Dedicated pitfalls section for common C++ gotchas specific to mcpp (async, callbacks, thread safety)

### Documentation format
- ASCII diagrams for architecture/data flow (simple text-based inline with Markdown)
- Inline with prose: code examples mixed directly into explanation text
- Module references: "See the handler module" rather than specific file paths
- Real test files: reference actual tests/cli tests; readers can open and follow along

### Claude's Discretion
- Exact wording and tone of documentation
- Specific ASCII diagram designs
- Which examples/ files to reference from examples/ directory
- Ordering within the linear narrative sections

</decisions>

<specifics>
## Specific Ideas

- README should start with code that works immediately — seeing it run first builds confidence
- Testing guide focuses on real tests from the codebase so readers can follow along in their editor
- Keep explanations brief and practical; readers are working developers who can look up concepts themselves

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 11-documentation*
*Context gathered: 2026-02-01*
