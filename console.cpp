#include "AVL/AVLTree.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>

namespace {
struct EndOfInput {};

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string text;
    if (!std::getline(std::cin, text)) throw EndOfInput{};
    return text;
}

template<class T>
T read(const std::string& prompt) {
    const auto text = readLine(prompt);
    if constexpr (std::is_same_v<T, std::string>) {
        return text;
    } else {
        if constexpr (std::is_unsigned_v<T>) {
            const auto first = text.find_first_not_of(" \t");
            if (first != std::string::npos && text[first] == '-')
                throw std::invalid_argument("Требуется неотрицательное число");
        }
        std::istringstream input(text);
        T value{};
        if (!(input >> value)) throw std::invalid_argument("Неверное число");
        input >> std::ws;
        if (!input.eof()) throw std::invalid_argument("Лишние символы после числа");
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(value)) throw std::invalid_argument("Требуется конечное число");
        }
        return value;
    }
}

template<class T>
void show(const AVLTree<T>& tree, const typename AVLTree<T>::OrderPolicy& policy) {
    std::cout << "Размер: " << tree.size() << "\nЭлементы:";
    auto e = tree.getEnumerator(policy);
    std::size_t displayed = 0;
    while (displayed < 100 && e->moveNext()) {
        std::cout << ' ';
        if constexpr (std::is_same_v<T, std::string>) std::cout << std::quoted(e->current());
        else std::cout << e->current();
        ++displayed;
    }
    if (tree.size() > displayed) std::cout << " ... (скрыто " << tree.size() - displayed << ')';
    if (tree.empty()) std::cout << " (пусто)";
    std::cout << '\n';
}

template<class T>
T addChecked(T value, T delta) {
    if constexpr (std::is_integral_v<T>) {
        if ((delta > 0 && value > std::numeric_limits<T>::max() - delta) ||
            (delta < 0 && value < std::numeric_limits<T>::min() - delta))
            throw std::overflow_error("Переполнение при преобразовании");
    }
    T result = value + delta;
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(result)) throw std::overflow_error("Переполнение при преобразовании");
    }
    return result;
}

template<class T>
void generate(AVLTree<T>& tree) {
    const auto count = read<std::size_t>("Количество попыток вставки (0..1000000): ");
    if (count > 1000000) throw std::invalid_argument("Для больших объёмов используйте avl_bench");
    AVLTree<T> generated;
    if constexpr (std::is_same_v<T, std::string>) {
        const auto length = read<std::size_t>("Длина строк (1..100): ");
        if (length == 0 || length > 100) throw std::invalid_argument("Неверная длина");
        const auto seed = read<std::uint32_t>("Seed: ");
        std::mt19937 engine(seed);
        std::uniform_int_distribution<int> letter(0, 25);
        for (std::size_t i = 0; i < count; ++i) {
            std::string value(length, 'a');
            for (char& ch : value) ch = static_cast<char>('a' + letter(engine));
            generated.insert(value);
        }
    } else {
        const T low = read<T>("Минимальное значение: ");
        const T high = read<T>("Максимальное значение: ");
        if (low > high) throw std::invalid_argument("Минимум больше максимума");
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(high - low)) throw std::invalid_argument("Слишком широкий диапазон");
        }
        const auto seed = read<std::uint32_t>("Seed: ");
        std::mt19937 engine(seed);
        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> distribution(low, high);
            for (std::size_t i = 0; i < count; ++i) generated.insert(distribution(engine));
        } else {
            std::uniform_real_distribution<T> distribution(low, high);
            for (std::size_t i = 0; i < count; ++i) generated.insert(distribution(engine));
        }
    }
    tree = std::move(generated);
    std::cout << "Генерация завершена. Уникальных элементов: " << tree.size()
              << ". Повторы не добавляются.\n";
}

template<class T>
void session() {
    using Tree = AVLTree<T>;
    Tree tree;
    auto policy = Tree::lkp();
    const char* orderName = "ЛКП";
    while (true) {
        std::cout << "\nAVL: " << tree.size() << " элементов; обход " << orderName << '\n'
                  << "1 Добавить     2 Удалить       3 Найти\n"
                  << "4 Показать     5 Сгенерировать 6 Очистить\n"
                  << "7 map         8 where         9 reduce\n"
                  << "10 Поддерево  11 Вхождение поддерева\n"
                  << "12 Сохранить в файл   13 Загрузить из файла\n"
                  << "14 Порядок обхода     15 Проверить перечислитель/reset\n"
                  << "16 Строка дерева      0 Выбор типа\n";
        try {
            const int choice = read<int>("> ");
            switch (choice) {
            case 0: return;
            case 1: {
                const T value = read<T>("Значение: ");
                std::cout << (tree.insert(value) ? "Добавлен\n" : "Уже существует\n");
                break;
            }
            case 2: {
                const T value = read<T>("Значение: ");
                std::cout << (tree.erase(value) ? "Удалён\n" : "Не найден\n");
                break;
            }
            case 3: {
                const T value = read<T>("Значение: ");
                std::cout << (tree.contains(value) ? "Найден\n" : "Не найден\n");
                break;
            }
            case 4: show(tree, policy); break;
            case 5: generate(tree); show(tree, policy); break;
            case 6: tree.clear(); std::cout << "Дерево очищено\n"; break;
            case 7: {
                std::cout << "Исходное дерево:\n";
                show(tree, policy);
                if constexpr (std::is_same_v<T, std::string>) {
                    const auto prefix = readLine("Префикс для каждого элемента: ");
                    auto mapped = tree.map([&](const T& value) { return prefix + value; });
                    std::cout << "Результат map (исходное дерево сохранено):\n";
                    show(mapped, policy);
                } else {
                    const T delta = read<T>("Прибавить к каждому элементу: ");
                    auto mapped = tree.map([&](T value) { return addChecked(value, delta); });
                    std::cout << "Результат map (исходное дерево сохранено):\n";
                    show(mapped, policy);
                }
                break;
            }
            case 8: {
                std::cout << "Исходное дерево:\n";
                show(tree, policy);
                if constexpr (std::is_same_v<T, std::string>) {
                    const auto pattern = readLine("Оставить строки с подстрокой: ");
                    auto filtered = tree.where([&](const T& v) { return v.find(pattern) != std::string::npos; });
                    std::cout << "Результат where:\n"; show(filtered, policy);
                } else {
                    const T threshold = read<T>("Оставить значения больше: ");
                    auto filtered = tree.where([&](T v) { return v > threshold; });
                    std::cout << "Результат where:\n"; show(filtered, policy);
                }
                break;
            }
            case 9:
                if constexpr (std::is_same_v<T, std::string>) {
                    if (tree.size() > 10000) throw std::invalid_argument("Для вывода reduce оставьте не более 10000 строк");
                    std::cout << "Склейка в порядке ЛКП: " << tree.reduce(
                        [](std::string acc, const T& v) { return acc + '[' + v + ']'; }, std::string{}) << '\n';
                } else {
                    const auto sum = tree.reduce([](long double acc, T v) { return acc + v; }, 0.0L);
                    std::cout << "Сумма в порядке ЛКП: " << sum << '\n';
                }
                break;
            case 10: {
                auto subtree = tree.getSubtree(read<T>("Корень поддерева: "));
                std::cout << "Независимое поддерево:\n"; show(subtree, policy);
                break;
            }
            case 11: {
                Tree candidate;
                candidate.deserialize(readLine("Строка образца, например {10}: "));
                std::cout << (tree.containsSubtree(candidate) ? "Поддерево входит\n" : "Поддерево не входит\n");
                break;
            }
            case 12: {
                const auto path = readLine("Путь файла (будет перезаписан): ");
                // Serialize before opening the file so a conversion error cannot erase it.
                const auto text = tree.serialize();
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                if (!output) throw std::runtime_error("Не удалось открыть файл");
                output << text;
                output.close();
                if (!output) throw std::runtime_error("Не удалось записать файл");
                std::cout << "Сохранено\n";
                break;
            }
            case 13: {
                const auto path = readLine("Путь файла: ");
                std::ifstream input(path, std::ios::binary | std::ios::ate);
                if (!input) throw std::runtime_error("Не удалось открыть файл");
                const auto size = input.tellg();
                if (size < 0 || size > 64 * 1024 * 1024) throw std::runtime_error("Размер файла недоступен или больше 64 МиБ");
                std::string text(static_cast<std::size_t>(size), '\0');
                input.seekg(0);
                if (!text.empty() && !input.read(text.data(), static_cast<std::streamsize>(text.size())))
                    throw std::runtime_error("Не удалось прочитать файл");
                tree.deserialize(text);
                std::cout << "Загружено\n"; show(tree, policy);
                break;
            }
            case 14: {
                const int order = read<int>("1 КЛП, 2 КПЛ, 3 ЛКП, 4 ЛПК, 5 ПКЛ, 6 ПЛК: ");
                switch (order) {
                case 1: policy = Tree::klp(); orderName = "КЛП"; break;
                case 2: policy = Tree::kpl(); orderName = "КПЛ"; break;
                case 3: policy = Tree::lkp(); orderName = "ЛКП"; break;
                case 4: policy = Tree::lpk(); orderName = "ЛПК"; break;
                case 5: policy = Tree::pkl(); orderName = "ПКЛ"; break;
                case 6: policy = Tree::plk(); orderName = "ПЛК"; break;
                default: throw std::invalid_argument("Неверный порядок обхода");
                }
                break;
            }
            case 15: {
                auto e = tree.getEnumerator(policy);
                std::size_t count = 0;
                while (e->moveNext()) ++count;
                e->reset();
                std::cout << "Перечислено: " << count << ". После reset: ";
                if (e->moveNext()) std::cout << e->current();
                else std::cout << "дерево пусто";
                std::cout << '\n';
                break;
            }
            case 16:
                if (tree.size() > 10000) throw std::invalid_argument("Большое дерево сохраните в файл");
                std::cout << "Строка дерева: " << tree.serialize() << '\n';
                break;
            default: throw std::invalid_argument("Нет такого пункта меню");
            }
        } catch (const std::exception& error) {
            std::cout << "Ошибка: " << error.what() << '\n';
        }
    }
}
}

int main() {
    std::cout << "Лабораторная работа: AVL-дерево\n"
              << "Строки вводятся целиком, без внешних кавычек.\n";
    try {
        while (true) {
            try {
                switch (read<int>("\nТип: 1 int, 2 double, 3 строка, 0 выход: ")) {
                case 0: return 0;
                case 1: session<int>(); break;
                case 2: session<double>(); break;
                case 3: session<std::string>(); break;
                default: throw std::invalid_argument("Нет такого типа");
                }
            } catch (const std::exception& error) {
                std::cout << "Ошибка: " << error.what() << '\n';
            }
        }
    } catch (const EndOfInput&) {
        std::cout << "\nВвод завершён\n";
        return 0;
    }
}