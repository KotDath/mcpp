#!/bin/bash

# Определяем корневую директорию проекта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"


echo "=== Комплексное тестирование MCP Echo Server ==="

# Тестируем разные типы запросов
REQUESTS=(
    '{"jsonrpc":"2.0","id":1,"method":"ping"}'
    '{"jsonrpc":"2.0","id":2,"method":"echo","params":{"message":"Hello World"}}'
    '{"jsonrpc":"2.0","id":3,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{"tools":{}},"clientInfo":{"name":"test-client","version":"1.0.0"}}}'
    '{"jsonrpc":"2.0","method":"notifications/initialized"}'
)

echo "Тестируем запросы:"

for i in "${!REQUESTS[@]}"; do
    REQUEST="${REQUESTS[$i]}"
    echo
    echo "Тест $((i+1)): $REQUEST"
    echo "--- Ответ ---"

    # Отправляем запрос и получаем ответ (только JSON, без логов)
    echo "$REQUEST" | timeout 5 "$PROJECT_ROOT/build/examples/echo_server" 2>/dev/null | grep -E '^{' || echo "Ответ не получен или неверный формат"
done

echo
echo "=== Тестирование завершено ==="