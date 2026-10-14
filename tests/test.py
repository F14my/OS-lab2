from pathlib import Path
import subprocess
import sys
import tempfile


parent = Path(sys.argv[1]).resolve()

lines = ["Hello", "Привет", "abcdef", "12345"]
input_text = "first.txt\nsecond.txt\n" + "\n".join(lines) + "\n"

with tempfile.TemporaryDirectory() as temp_folder:
    folder = Path(temp_folder)

    result = subprocess.run(
        [str(parent)],
        input=input_text,
        text=True,
        encoding="utf-8",
        cwd=folder,
        timeout=20,
    )

    if result.returncode != 0:
        raise SystemExit("Ошибка: родительская программа завершилась с ошибкой")

    first = (folder / "first.txt").read_text(encoding="utf-8").splitlines()
    second = (folder / "second.txt").read_text(encoding="utf-8").splitlines()

    actual = first + second
    expected = [line[::-1] for line in lines]

    if sorted(actual) != sorted(expected):
        raise SystemExit(f"Ошибка: ожидалось {expected}, получено {actual}")

    print("Тест пройден: все строки правильно записаны и перевёрнуты")
    print("Содержимое first.txt:")
    print(first)
    print("Содержимое second.txt:")
    print(second)
