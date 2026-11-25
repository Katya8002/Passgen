# Passgen — Сейф паролів

## Про проєкт
Passgen — програма на C, яка генерує пароль із файлу та ключового слова. Той самий файл і слово завжди дають однаковий пароль, тому паролі ніде не зберігаються. Файл програма тільки читає.

SHA-256 і генератор випадкових чисел написані самі.

## Збірка
```
make
```

## Запуск
```
./passgen -i samples/photo.jpg -k github
```

З довжиною 16:
```
./passgen -i samples/photo.jpg -k github -l 16
```

З майстер-паролем:
```
./passgen -i samples/photo.jpg -k github -p
```

## Тести
```
make test
./run_tests.sh
```

## Структура
- include/ — заголовочні файли
- src/ — код програми
- tests/ — тести
- samples/ — тестові файли
- run_tests.sh — тести програми

## Команда
- Галич Катерина: cli, fileio, secret, main, тести
- Покатілова Дарʼя: sha256, hash, prng, charset, entropy, generator
