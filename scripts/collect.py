#!/usr/bin/env python3
"""
Скрипт для сбора содержимого файлов по glob-шаблонам
из указанной директории и всех поддиректорий.
По умолчанию выводит результат в stdout.
"""

import os
import sys
import argparse
import fnmatch
from pathlib import Path

__version__ = "2.0.0"


def collect_files(root_dir: Path, patterns: list) -> list:
    """
    Рекурсивно обходит root_dir и возвращает отсортированный список
    путей к файлам, имя которых совпадает хотя бы с одним из шаблонов.
    Шаблоны - shell-globs (*.cpp, file_?_*.java и т.п.).
    """
    collected = set()
    for dirpath, _, filenames in os.walk(root_dir):
        for fname in filenames:
            if any(fnmatch.fnmatch(fname, pat) for pat in patterns):
                collected.add(Path(dirpath) / fname)
    return sorted(collected)


def write_collected_content(
    file_paths: list,
    out_f,
    root_dir: Path,
    *,
    encoding: str = 'utf-8',
    errors: str = 'replace'
) -> None:
    """
    Записывает в открытый файловый объект out_f содержимое всех файлов.
    Перед каждым файлом - заголовок с относительным путём от root_dir.
    """
    for src_path in file_paths:
        try:
            rel_path = src_path.relative_to(root_dir)
        except ValueError:
            rel_path = src_path
        out_f.write(f"{rel_path}\n")
        out_f.write("=" * 60 + "\n")
        try:
            with open(src_path, 'r', encoding=encoding, errors=errors) as in_f:
                content = in_f.read()
            out_f.write(content)
            if content and not content.endswith('\n'):
                out_f.write('\n')
        except Exception as e:
            out_f.write(f"[Ошибка чтения файла: {e}]\n")
        out_f.write("\n" + "=" * 60 + "\n\n")


def main():
    parser = argparse.ArgumentParser(
        description="Собрать содержимое файлов по glob-шаблонам "
                    "из директории (рекурсивно).",
        epilog='Примеры:\n'
               '  %(prog)s "*.cpp"\n'
               '  %(prog)s "*.cpp" "*.h" -d ./myproject\n'
               '  %(prog)s "file_?_*.java" -o output.txt',
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        'patterns',
        nargs='+',
        help='Один или несколько glob-шаблонов (например "*.cpp" "*.h"). '
             'Кавычки обязательны, иначе шелл раскроет шаблон сам.'
    )
    parser.add_argument(
        '-d', '--dir',
        default='.',
        help="Директория для обхода (по умолчанию текущая)"
    )
    parser.add_argument(
        '-o', '--output',
        help="Выходной файл (по умолчанию вывод в stdout)"
    )
    parser.add_argument(
        '--version',
        action='version',
        version=f"%(prog)s {__version__}"
    )
    args = parser.parse_args()

    root_dir = Path(args.dir).expanduser().resolve()
    if not root_dir.is_dir():
        print(f"Ошибка: '{root_dir}' не является директорией.", file=sys.stderr)
        sys.exit(1)

    print(f"Поиск файлов по шаблонам {args.patterns} в {root_dir}...",
          file=sys.stderr)
    files = collect_files(root_dir, args.patterns)
    if not files:
        print("Файлы по указанным шаблонам не найдены.", file=sys.stderr)
        sys.exit(0)

    if args.output:
        print(f"Найдено {len(files)} файлов. Запись в '{args.output}'...",
              file=sys.stderr)
        with open(args.output, 'w', encoding='utf-8') as out_f:
            write_collected_content(files, out_f, root_dir)
        print("Готово.", file=sys.stderr)
    else:
        print(f"Найдено {len(files)} файлов. Вывод в stdout.", file=sys.stderr)
        write_collected_content(files, sys.stdout, root_dir)


if __name__ == "__main__":
    main()
