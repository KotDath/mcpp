#!/bin/bash

# Скрипт для обновления путей во всех тестовых скриптах

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Обновляем пути в тестовых скриптах..."

# Обновляем comprehensive_test.sh
sed -i "s|\.\./build/examples/echo_server|\"\$PROJECT_ROOT/build/examples/echo_server\"|g" comprehensive_test.sh

# Обновляем test_server.sh, test_fifo.sh, test_nc.sh
sed -i "s|\.\./build/examples/echo_server|\"\$PROJECT_ROOT/build/examples/echo_server\"|g" test_server.sh test_fifo.sh test_nc.sh

# Добавляем определение PROJECT_ROOT в начало каждого файла
for script in comprehensive_test.sh test_server.sh test_fifo.sh test_nc.sh; do
    if ! grep -q "PROJECT_ROOT=" "$script"; then
        # Создаем временный файл с новым началом
        cat > "${script}.tmp" << EOF
#!/bin/bash

# Определяем корневую директорию проекта
SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="\$(dirname "\$SCRIPT_DIR")"

EOF
        # Добавляем остальное содержимое файла
        tail -n +2 "$script" >> "${script}.tmp"
        # Заменяем оригинал
        mv "${script}.tmp" "$script"
        chmod +x "$script"
    fi
done

echo "Пути обновлены!"