# Testing mcpp with MCP Inspector

This guide explains how to test the mcpp example server using the MCP Inspector CLI tool.

## Prerequisites

- Node.js ^22.7.5 (required for MCP Inspector)
- Built mcpp library and examples
- npx (comes with Node.js)

## Building

From the project root:
```bash
mkdir -p build && cd build
cmake ..
make
```

Verify the example server exists:
```bash
ls -la build/examples/inspector_server
```

## UI Mode (Interactive Testing)

Start Inspector with the mcp.json config:
```bash
npx @modelcontextprotocol/inspector --config examples/mcp.json
```

Or directly with the executable:
```bash
npx @modelcontextprotocol/inspector node build/examples/inspector_server
```

The Inspector UI will open at http://localhost:6274

### Testing Tools

1. Click "Tools" in the sidebar
2. Click "calculate" tool
3. Enter arguments: `{"operation": "add", "a": 5, "b": 3}`
4. Click "Call Tool"
5. Expected result: `{"content": [{"type": "text", "text": "8"}]}`

### Testing Resources

1. Click "Resources" in the sidebar
2. Click "List Resources"
3. Expected: file://tmp/mcpp_test.txt and info://server

### Testing Prompts

1. Click "Prompts" in the sidebar
2. Click "List Prompts"
3. Expected: code_review prompt

## CLI Mode (Scriptable Testing)

### List Tools
```bash
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method tools/list
```

### Call a Tool
```bash
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method tools/call \
    --tool-name calculate \
    --tool-arg operation=add \
    --tool-arg a=42 \
    --tool-arg b=8
```

Expected output:
```json
{
  "content": [
    {"type": "text", "text": "50"}
  ]
}
```

### List Resources
```bash
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method resources/list
```

### List Prompts
```bash
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method prompts/list
```

## Manual Stdio Testing

For quick debugging without Inspector:
```bash
echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./build/examples/inspector_server
```

Expected: Valid JSON-RPC response with tools array.

## Troubleshooting

### "command not found: npx"
Install Node.js from https://nodejs.org/ (version 22.7.5 or later)

### "Cannot find module @modelcontextprotocol/inspector"
Wait for npx to download the package (first run may take a minute)

### Inspector shows "Connection failed"
- Verify server builds: `make inspector_server`
- Check executable exists: `ls build/examples/inspector_server`
- Try manual test: `echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' | ./build/examples/inspector_server`

### No tools/resources/prompts appear
- Check debug output (stderr) for validation errors
- Verify MCPP_DEBUG=1 is set in mcp.json env
- Test with manual stdio command above

### Parse error responses
After Phase 9, parse errors should include the request ID:
```json
{"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null}
```

## See Also

- MCP Inspector Documentation: https://modelcontextprotocol.io/docs/tools/inspector
- MCP Inspector GitHub: https://github.com/modelcontextprotocol/inspector
