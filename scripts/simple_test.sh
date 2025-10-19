#!/bin/bash

# Определяем корневую директорию проекта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Простое тестирование MCP Echo Server ==="

# Создаем временный файл с одним запросом
cat > /tmp/single_request.json << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"ping"}
EOF

echo "Отправляем один ping запрос:"
echo "--- Запрос ---"
cat /tmp/single_request.json

echo
echo "--- Ответ сервера ---"
# Отправляем запрос и ловим только JSON ответ, игнорируя логи
cat /tmp/single_request.json | "$PROJECT_ROOT/build/examples/echo_server" 2>/dev/null | grep '{' || echo "Ответ не получен"

# Удаляем временный файл
rm -f /tmp/single_request.json