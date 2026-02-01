# calc

CLI-калькулятор на C++ (CMake), используется библиотека `mathlib` для выполнения арифметических операций.

## Build

```bash
cmake -B build
cmake --build build
```

Бинарник появится здесь: build/calc.

## Install

```bash
sudo cmake --build build --target install
```

## Example

```bash
./build/calc -o add -a 2 -b 3
```
