# InterpolationLab: Лабораторна робота №7 (ТПП), варіант 3 «Інтерполяція»

Модернізація статичної бібліотеки інтерполяції з лабораторної №4: обробка винятків, RAII,
гарантії безпеки винятків, модульні тести Google Test. C++20, CMake, vcpkg, Visual Studio.

## Структура проєкту

| Файл | Призначення |
|---|---|
| `include/Interpolation.h`, `src/Interpolation.cpp` | бібліотека з лаби №4 без змін (лінійна інтерполяція та Лагранж, коди `INTERP_ERR_*`) |
| `include/InterpolationTask.h`, `src/InterpolationTask.cpp` | клас `InterpolationTask`: стан, інваріанти, перетворення кодів помилок на винятки |
| `include/InterpolationExceptions.h` | ієрархія власних винятків |
| `include/ScopeLogger.h` | RAII-логер для дослідження розкручування стека |
| `demo/main.cpp` | демонстрація помилок, сильної гарантії, ланцюжка з 3 функцій |
| `antipattern/main.cpp` | `throw int`, `throw std::string`, `catch (...)`, `throw ex;` |
| `tests/InterpolationTests.cpp` | модульні тести Google Test |

## Збирання та запуск

1. Відкрити папку проєкту у Visual Studio (File → Open → Folder).
2. У `vcpkg.json` має бути `builtin-baseline`. У `CMakePresets.json` перевірити шлях до `vcpkg.cmake`.
3. Вибрати конфігурацію `x64 Debug (vcpkg)`, виконати Project → Delete Cache and Reconfigure.
4. Build → Build All.
5. Цілі: `InterpDemo`, `AntipatternDemo`, `InterpTests`.
6. Тести: Test Explorer або `out\build\x64-debug\InterpTests.exe`.

## Інваріанти класу InterpolationTask

1. `2 <= size() <= maxNodes() <= INTERP_MAX_N` (10000).
2. Усі `x` та `y` скінченні (не NaN/Inf).
3. Вузли впорядковані за `x` строго зростаюче (дублікатів немає).

Некоректний стан недоступний через публічний інтерфейс: вузли змінюються лише через конструктор,
`replaceNodes()` та `addNode()`, які спершу перевіряють новий набір у тимчасовому об'єкті.
Вузли, подані у довільному порядку, клас упорядковує сам.

## Ієрархія винятків

```
std::runtime_error
 └─ InterpolationException
     ├─ InvalidNodeException        (index, parameter, value)
     ├─ DuplicateAbscissaException  (x, firstIndex, secondIndex)
     ├─ ExtrapolationException      (x, minX, maxX)
     └─ NodeCountException          (limit, actual)
```

Використовуються також стандартні винятки: `std::invalid_argument`, `std::out_of_range`,
`std::overflow_error`.

## Сценарії помилок

Алгоритми лаби №4 збережено, вони повертають коди помилок. Клас `InterpolationTask` перевіряє набір
вузлів сам (щоб знати індекси) і перетворює коди, що повернула бібліотека, на винятки.

| Операція | Умова | Код бібліотеки | Виняток |
|---|---|---|---|
| конструктор, `replaceNodes`, `addNode` | порожній набір | - | `std::invalid_argument` |
| конструктор | `maxNodes` не в `[1, INTERP_MAX_N]` | - | `std::invalid_argument` |
| конструктор, `replaceNodes`, `addNode` | NaN/Inf у вузлі | `INTERP_ERR_NOT_FINITE` | `InvalidNodeException` |
| конструктор, `replaceNodes`, `addNode` | однакові абсциси | `INTERP_ERR_DUPLICATE_X` | `DuplicateAbscissaException` |
| конструктор, `replaceNodes`, `addNode` | вузлів більше за `maxNodes` | `INTERP_ERR_SIZE` | `NodeCountException` |
| конструктор, `replaceNodes`, `addNode` | вузлів менше 2 | `INTERP_ERR_SIZE` | `NodeCountException` |
| `evaluate(.., Linear)`, `computeAll` | `xq` поза `[minX, maxX]` | `INTERP_ERR_OUT_OF_RANGE` | `ExtrapolationException` |
| `evaluate`, `approximationError` | `xq` або `exactValue` = NaN/Inf | `INTERP_ERR_NOT_FINITE` | `std::invalid_argument` |
| `evaluate`, `approximationError` | невідомий метод | `INTERP_ERR_BAD_METHOD` | `std::invalid_argument` |
| `at(i)` | `i >= size()` | - | `std::out_of_range` |
| `evaluate` | нескінченний результат | `INTERP_ERR_OVERFLOW` | `std::overflow_error` |

Метод Лагранжа (як у лабі №4) допускає екстраполяцію, лінійна інтерполяція ні.

## Гарантії безпеки винятків

| Метод | Гарантія |
|---|---|
| конструктор | strong (при помилці об'єкт не створюється) |
| `replaceNodes()` | strong: підготовка у тимчасовому об'єкті, потім `swap` (noexcept) |
| `addNode()` | strong: копія, перевірка, `swap` |
| `computeAll()` | strong: обидва методи обчислюються в локальній змінній, збереження лише після успіху обох |
| `evaluate()`, `approximationError()`, `at()` | не змінюють стан |
| `size()`, `maxNodes()`, `minX()`, `maxX()`, `lastResults()` | no-throw (`noexcept`) |
| деструктор `ScopeLogger` | `noexcept` |

Ключовий фрагмент (`computeAll`):

```cpp
InterpolationResults r{};
r.x = xq;
r.lagrange = evaluate(xq, Method::Lagrange);  // поза межами порахується (екстраполяція)
r.linear   = evaluate(xq, Method::Linear);    // тут може бути ExtrapolationException,
                                              // стан об'єкта ще не змінено
last_ = r;                                    // фіксація (тривіальне присвоєння, noexcept)
```

## Розкручування стека (ScopeLogger)

Очікувана послідовність до запуску (`level3` кидає `DuplicateAbscissaException`):

```
[ENTER] level1
[ENTER] level2
[ENTER] level3
Creating task with duplicate abscissas...
[EXIT ] level3
[LOG] level2 caught: Duplicate abscissa in nodes: x = 2.000000
[EXIT ] level2
[EXIT ] level1
[MAIN] Exception: Duplicate abscissa in nodes: x = 2.000000
Program continues normally.
```

Фактичний результат `InterpDemo.exe` збігається з очікуваним: деструктори викликаються у зворотному
порядку конструювання, `throw;` зберігає тип винятку.

## Антипатерни

`AntipatternDemo` показує:
- `catch (const std::exception&)` не перехоплює винятки типів `int` та `std::string`;
- `catch (...)` перехоплює все, але без доступу до типу та даних про помилку;
- `throw ex;` призводить до object slicing (втрата похідного типу).

Висновок: стандартні й користувацькі класи винятків (похідні від `std::exception`) дають єдиний
інтерфейс `what()`, ієрархію та централізовану обробку, а також несуть діагностичні дані.
Примітивні типи цього не забезпечують, тому використовувати їх не слід.

## Модульні тести

37 тестів Google Test (1 додатково вимкнений):
- бібліотека з лаби №4 не змінилась (коди помилок);
- основні операції та точність обчислень (`EXPECT_NEAR`);
- кожен сценарій помилки з перевіркою типу винятку (`EXPECT_THROW`) та діагностичних даних;
- граничні значення;
- інваріанти класу;
- сильна гарантія: після невдалої операції стан об'єкта не змінюється.

## Негативний самоконтроль

Метод `replaceNodesUnsafe()` навмисно очищає вузли ще до перевірок. Тест
`NegativeSelfCheck.DISABLED_UnsafeReplaceViolatesStrongGuarantee` вимкнений у звичайному запуску.

Запуск:

```
InterpTests.exe --gtest_also_run_disabled_tests --gtest_filter=*Unsafe*
```

Результат: FAILED, `t.size()` дорівнює 0 замість 4. Виняток виникає правильного типу, але початковий
стан втрачено, тому порушена сильна гарантія (базова зберігається). Метод `replaceNodes()` проходить
той самий сценарій успішно.