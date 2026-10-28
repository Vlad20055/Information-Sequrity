#import "stp2024.typ"

#show: stp2024.template

#let CHANGE_ME(arg) = {
  text(fill: red, arg)
}

#align(center)[

  Министерство образования Республики Беларусь

  #v(1.15em)

  Учреждение образования

  #v(1.15em)

  БЕЛОРУССКИЙ ГОСУДАРСТВЕННЫЙ УНИВЕРСИТЕТ \ ИНФОРМАТИКИ И РАДИОЭЛЕКТРОНИКИ

  #v(1.15em)

]

#align(center)[

  Факультет компьютерных систем и сетей

  #v(1.15em)

  Кафедра информатики

  #v(1.15em)

  Дисциплина  "Методы защиты информации"

]



#v(1fr)

#v(4em)

#align(center)[

  ОТЧЁТ

  к лабораторной работе №7

  на тему

  #v(1.15em)

  #upper(
    [*КРИПТОГРАФИЯ С ИСПОЛЬЗОВАНИЕМ \ ЭЛЛИПТИЧЕСКИХ КРИВЫХ*]

  )

  #v(1.15em)

  БГУИР КП 6-05-0612-02 015 ПЗ

]

#v(1fr)

#grid(

  columns: (1fr, 1fr),

  align: center,

  [],

  [

    #block[

      Выполнил студент гр. 353502 \ МАКСИМЕНКОВ \ Владислав Александрович

      #v(0.3em)

      #line(length: 9cm)

      #v(-0.5em)

      #text(size: 9pt)[(дата, подпись студента)]

    ]

    #v(1.15em)

    #block[

      Проверил ассистент каф.Информатики ГЕРЧИК Артём Вадимович

      #v(0.3em)

      #line(length: 9cm)

      #v(-0.5em)

      #text(size: 9pt)[(дата, подпись преподавателя)]

    ]

  ]

)

#v(1fr)

#align(center)[

  Минск 2026

]

#pagebreak(weak: true)

#stp2024.full_outline()

#stp2024.heading_unnumbered[Введение]

В данной лабораторной работе рассматривается асимметричная схема шифрования, являющаяся аналогом алгоритма Эль-Гамаля на основе эллиптических кривых. Алгоритм Эль-Гамаля был предложен Тахером Эль-Гамалем в 1985 году и в классическом виде работает в мультипликативной группе вычетов по модулю простого числа $p$. Получатель выбирает закрытый ключ $x$ и публикует открытый ключ $y = g^x mod p$. Для зашифрования сообщения $m$ отправитель выбирает случайное число $k$ и передаёт пару

$c_1 = g^k mod p, quad c_2 = m dot y^k mod p.$

Получатель, зная $x$, вычисляет $y^k = c_1^x$ и восстанавливает $m = c_2 dot (c_1^x)^(-1) mod p$. Стойкость схемы основана на сложности задачи дискретного логарифмирования: по известным $g$ и $y$ невозможно за приемлемое время найти $x$.

В том же 1985 году Н. Коблиц и В. Миллер независимо предложили использовать в криптографии группу точек эллиптической кривой над конечным полем. Любой алгоритм, построенный на задаче дискретного логарифмирования, можно перенести в эту группу: умножение элементов заменяется сложением точек, а возведение в степень – умножением точки на число. Для группы точек эллиптической кривой не известны субэкспоненциальные алгоритмы дискретного логарифмирования, поэтому при той же стойкости ключи получаются значительно короче: ключ длиной 256 бит на эллиптической кривой соответствует ключу RSA длиной около 3072 бит.

В эллиптическом аналоге алгоритма Эль-Гамаля сообщение представляется точкой кривой $M$, а шифртекстом является пара точек. Особенностью схемы является вероятностный характер шифрования: из-за случайного числа $k$ один и тот же открытый текст при каждом зашифровании даёт разный шифртекст.

Целью лабораторной работы является разработка программного средства на языке #box[C++], реализующего генерацию ключей, зашифрование и расшифрование текстовых файлов по схеме, аналогичной алгоритму Эль-Гамаля, на основе эллиптических кривых.

#pagebreak(weak: true)

= Выполнение работы
В ходе выполнения лабораторной работы было разработано программное средство на языке #box[C++] для шифрования файлов на эллиптических кривых. Все алгоритмы реализованы без использования сторонних библиотек. Программа поддерживает генерацию ключевой пары длиной 256 или 512 бит, зашифрование файла открытым ключом, расшифрование закрытым ключом и проверку реализации на контрольном примере.

Программа разделена на несколько модулей. Модуль bigint реализует арифметику больших целых чисел, модуль ec – операции с точками эллиптической кривой и наборы параметров, модуль random – генерацию случайных чисел, модуль elgamal – представление сообщения точками кривой, зашифрование и расшифрование, а главный модуль main.cpp отвечает за разбор аргументов командной строки и вывод результатов.

В модуле bigint большое число хранится как массив 32-битных слов. Реализованы сложение, вычитание, умножение, деление с остатком по алгоритму Кнута, а также операции по модулю: сложение, вычитание, умножение, возведение в степень и вычисление обратного элемента по малой теореме Ферма $a^(-1) = a^(p-2) mod p$.

Используется эллиптическая кривая в форме Вейерштрасса над простым полем $"GF"(p)$:

$y^2 = x^3 + a x + b mod p.$

Точки кривой вместе с бесконечно удалённой точкой $O$ образуют группу. Сумма точек $(x_1, y_1)$ и $(x_2, y_2)$ вычисляется по формулам

$x_3 = lambda^2 - x_1 - x_2, quad y_3 = lambda (x_1 - x_3) - y_1,$

где $lambda = (y_2 - y_1) / (x_2 - x_1)$ при сложении различных точек и $lambda = (3 x_1^2 + a) / (2 y_1)$ при удвоении. Противоположной к точке $(x, y)$ является точка $(x, -y)$, поэтому вычитание точек сводится к сложению с противоположной точкой. Для ускорения вычислений точки представляются в якобиевых координатах $(X, Y, Z)$, где $x = X / Z^2$, $y = Y / Z^3$, а произведение $k P$ вычисляется методом «удвоения и сложения» по двоичной записи числа $k$.

В программе используются стандартизованные наборы параметров: для ключей длиной 256 бит – id-GostR3410-2001-CryptoPro-A-ParamSet с модулем $p = 2^(256) - 617$, для ключей длиной 512 бит – id-tc26-gost-3410-12-512-paramSetA с модулем $p = 2^(512) - 569$. Каждый набор задаёт коэффициенты $a$ и $b$, простой порядок $q$ подгруппы и базовую точку $P$ порядка $q$.

Закрытым ключом получателя является случайное число $d$, $0 < d < q$, а открытым ключом – точка

$Q = d P.$

Для получения случайных чисел используется криптостойкий генератор операционной системы. Ключи сохраняются в текстовые файлы: открытый ключ содержит длину ключа и координаты точки $Q$, закрытый – длину ключа и число $d$.

Перед зашифрованием сообщение необходимо представить точками кривой. Для этого используется вероятностный метод Коблица. Файл разбивается на блоки длиной $B$ байт: $B = 30$ для ключа 256 бит и $B = 62$ для ключа 512 бит. Блок рассматривается как целое число $m$, и для $j = 0, 1, ..., 255$ проверяется значение

$x = 256 m + j.$

Для первого $j$, при котором правая часть уравнения кривой $f(x) = x^3 + a x + b$ является квадратичным вычетом по модулю $p$, находится координата $y$, и точка $M = (x, y)$ представляет блок. Для обоих наборов параметров выполняется $p equiv 3 mod 4$, поэтому квадратный корень вычисляется по формуле

$y = f(x)^((p + 1) / 4) mod p,$

после чего проверяется равенство $y^2 = f(x)$. Квадратичным вычетом является примерно половина чисел, поэтому вероятность того, что ни одно из 256 значений не подойдёт, составляет около $2^(-256)$. Обратное преобразование выполняется отбрасыванием младшего байта: $m = floor(x / 256)$.

Зашифрование каждого блока выполняется следующим образом:

+ Представить блок точкой кривой $M$.
+ Сгенерировать случайное число $k$, $0 < k < q$.
+ Вычислить точки $C_1 = k P$ и $C_2 = M + k Q$.
+ Шифртекстом блока является пара точек $(C_1, C_2)$.

Для каждого блока выбирается новое число $k$. Если бы одно и то же $k$ использовалось для двух блоков, то разность $C_2 - C_2' = M - M'$ не зависела бы от ключа, и знание одного открытого текста раскрывало бы другой.

Получатель расшифровывает блок с помощью закрытого ключа:

$M = C_2 - d C_1.$

Правильность расшифрования следует из равенства

$C_2 - d C_1 = M + k Q - d k P = M + k d P - d k P = M.$

Противник знает точки $P$, $Q$ и $C_1 = k P$, но для вычисления $k Q = d k P$ ему необходимо найти $d$ или $k$, то есть решить задачу дискретного логарифмирования на эллиптической кривой.

Зашифрованный файл является текстовым. В его начале записываются длина ключа, исходный размер файла и число блоков, а затем для каждого блока – четыре координаты точек $C_1$ и $C_2$ в шестнадцатеричном виде. Каждые 30 байт открытого текста превращаются в две точки, то есть в четыре числа по 256 бит, поэтому размер данных при шифровании заметно увеличивается. При расшифровании программа проверяет, что точки лежат на кривой, а восстановленное число $m$ помещается в блок. При использовании чужого закрытого ключа получаются случайные точки, и программа сообщает об ошибке.

Для запуска программы предусмотрены следующие операции:

```
elgamal keygen [-b 256|512] public.key private.key
elgamal encrypt public.key input.txt secret.enc
elgamal decrypt private.key secret.enc output.txt
elgamal test
```

Первая команда создаёт ключевую пару, вторая выполняет зашифрование файла, третья – его расшифрование, а четвёртая – проверку реализации. В качестве контрольного примера используется пример из учебника У. Столлингса «Криптография и защита сетей»: кривая $y^2 = x^3 - x + 188 mod 751$, базовая точка $G = (0, 376)$ порядка 727, открытый ключ получателя $P_B = (201, 5)$, сообщение $P_m = (562, 201)$ и число $k = 386$. По учебнику шифртекстом должна быть пара точек $C_1 = (676, 558)$ и $C_2 = (385, 328)$. Расшифрование проверяется закрытым ключом $d = 58$, для которого $P_B = 58 G$. Кроме того, проверяются представление блока точкой и обратное преобразование, зашифрование и расшифрование для обоих наборов параметров, различие шифртекстов одного и того же сообщения и отказ при расшифровании чужим ключом. Результат проверки продемонстрирован на рисунке 1.1.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/test.png"),
    caption: [Проверка реализации]
  )
]

Все вычисленные значения совпали с приведёнными в учебнике, что подтверждает правильность реализации. Генерация ключевой пары и содержимое файла открытого ключа продемонстрированы на рисунке 1.2.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/keygen.png"),
    caption: [Генерация ключевой пары]
  )
]

Далее тестовый файл input.txt был зашифрован открытым ключом. Начало зашифрованного файла продемонстрировано на рисунке 1.3: служебные данные и координаты точек $C_1$ и $C_2$ первых двух блоков.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/encrypt.png"),
    caption: [Зашифрование файла]
  )
]

Расшифрование файла закрытым ключом продемонстрировано на рисунке 1.4. Текст в файле output.txt совпадает с текстом в исходном файле input.txt, что подтверждает утилита cmp.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/decrypt.png"),
    caption: [Расшифрование файла]
  )
]

Попытка расшифровать файл закрытым ключом другой ключевой пары завершается ошибкой, что продемонстрировано на рисунке 1.5.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/wrong_key.png"),
    caption: [Расшифрование чужим ключом]
  )
]

Таким образом, в ходе работы была реализована схема шифрования, аналогичная алгоритму Эль-Гамаля, на эллиптических кривых с длиной ключа 256 и 512 бит с поддержкой работы с файлами.

#pagebreak(weak: true)

#stp2024.heading_unnumbered[Вывод]

В ходе выполнения лабораторной работы был изучен эллиптический аналог алгоритма Эль-Гамаля и разработано программное средство на языке #box[C++], реализующее генерацию ключевой пары, зашифрование и расшифрование текстовых файлов.

В программе самостоятельно реализованы арифметика больших целых чисел, операции сложения, вычитания и умножения точек эллиптической кривой, представление блоков сообщения точками кривой методом Коблица с вычислением квадратного корня по модулю $p$, а также зашифрование $(C_1, C_2) = (k P, M + k Q)$ и расшифрование $M = C_2 - d C_1$. Корректность реализации подтверждена совпадением результатов с контрольным примером из учебника.

Проведённое тестирование показало, что после последовательного зашифрования и расшифрования исходный файл восстанавливается полностью, повторное зашифрование одного и того же текста даёт другой шифртекст, а попытка расшифрования чужим ключом завершается ошибкой. Подтверждён и недостаток схемы: шифртекст заметно больше открытого текста, так как каждый блок превращается в две точки кривой.

В результате выполнения лабораторной работы были получены практические навыки реализации асимметричных криптосистем на эллиптических кривых, работы с большими целыми числами и представления произвольных данных точками эллиптической кривой.

#stp2024.appendix(type:[обязательное], title:[Листинг программного кода])[


#stp2024.listing[elgamal.h][
    ```
#pragma once

#include "bigint.h"
#include "ec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace elgamal {

struct PrivateKey {
    int bits = 256;
    BigInt d;

    void save(const std::string& path) const;
    static PrivateKey load(const std::string& path);
};

struct PublicKey {
    int bits = 256;
    Point Q;

    void save(const std::string& path) const;
    static PublicKey load(const std::string& path);
};

struct CipherBlock {
    Point C1, C2;
};

size_t blockBytes(const Curve& c);
Point encodeBlock(const Curve& c, const std::vector<uint8_t>& block);
std::vector<uint8_t> decodeBlock(const Curve& c, const Point& M);

CipherBlock encryptPoint(const Curve& c, const Point& Q, const Point& M,
                         const BigInt& k);
Point decryptPoint(const Curve& c, const BigInt& d, const CipherBlock& cb);

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub);
std::string encrypt(const PublicKey& key, const std::vector<uint8_t>& data);
std::vector<uint8_t> decrypt(const PrivateKey& key,
                             const std::string& cipher);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}
    ```
  ]

#stp2024.listing[elgamal.cpp][
    ```
#include "elgamal.h"

#include "random.h"

#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>

namespace elgamal {

namespace {

using Fields = std::map<std::string, std::string>;

std::string readText(const std::string& path) {
    const auto data = readFile(path);
    return std::string(data.begin(), data.end());
}

void writeText(const std::string& path, const std::string& text) {
    writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
}

bool splitField(std::string line, std::string& key, std::string& value) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const size_t colon = line.find(": ");
    if (colon == std::string::npos) return false;
    key = line.substr(0, colon);
    value = line.substr(colon + 2);
    return true;
}

Fields readFields(const std::string& path) {
    std::istringstream in(readText(path));
    Fields f;
    std::string line, key, value;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        if (!splitField(line, key, value))
            throw std::runtime_error(path + ": неверная строка");
        f[key] = value;
    }
    return f;
}

const std::string& field(const Fields& f, const std::string& key,
                         const std::string& path) {
    const auto it = f.find(key);
    if (it == f.end())
        throw std::runtime_error(path + ": нет поля \"" + key + "\"");
    return it->second;
}

[[noreturn]] void badCipher() {
    throw std::runtime_error("неверный ключ или повреждённый шифртекст");
}

}

void PrivateKey::save(const std::string& path) const {
    std::ostringstream out;
    out << "bits: " << bits << "\n";
    out << "d: " << d.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

PrivateKey PrivateKey::load(const std::string& path) {
    const Fields f = readFields(path);
    PrivateKey k;
    k.bits = std::stoi(field(f, "bits", path));
    k.d = BigInt::fromHex(field(f, "d", path));
    if (k.d.isZero() || !(k.d < curveFor(k.bits).q))
        throw std::runtime_error(path + ": недопустимый закрытый ключ");
    return k;
}

void PublicKey::save(const std::string& path) const {
    std::ostringstream out;
    out << "bits: " << bits << "\n";
    out << "Qx: " << Q.x.toHex(bits / 4) << "\n";
    out << "Qy: " << Q.y.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

PublicKey PublicKey::load(const std::string& path) {
    const Fields f = readFields(path);
    PublicKey k;
    k.bits = std::stoi(field(f, "bits", path));
    k.Q = {BigInt::fromHex(field(f, "Qx", path)),
           BigInt::fromHex(field(f, "Qy", path)), false};
    if (!curveFor(k.bits).contains(k.Q))
        throw std::runtime_error(path + ": точка Q не лежит на кривой");
    return k;
}

size_t blockBytes(const Curve& c) {
    const size_t bits = c.p.bitLength() - 1;
    return bits > 8 ? (bits - 8) / 8 : 0;
}

Point encodeBlock(const Curve& c, const std::vector<uint8_t>& block) {
    const BigInt m = BigInt::fromBytesLE(block);
    for (uint32_t j = 0; j < 256; ++j) {
        Point M;
        if (c.liftX(m * BigInt(256) + BigInt(j), M)) return M;
    }
    throw std::runtime_error("не удалось представить блок точкой кривой");
}

std::vector<uint8_t> decodeBlock(const Curve& c, const Point& M) {
    const size_t n = blockBytes(c);
    const BigInt m = M.x / BigInt(256);
    if (M.infinity || m.bitLength() > 8 * n) badCipher();
    return m.toBytesLE(n);
}

CipherBlock encryptPoint(const Curve& c, const Point& Q, const Point& M,
                         const BigInt& k) {
    return {c.multiply(k, c.P), c.add(M, c.multiply(k, Q))};
}

Point decryptPoint(const Curve& c, const BigInt& d, const CipherBlock& cb) {
    return c.subtract(cb.C2, c.multiply(d, cb.C1));
}

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub) {
    const Curve& c = curveFor(bits);
    priv.bits = pub.bits = bits;
    priv.d = randomBelow(c.q);
    pub.Q = c.multiply(priv.d, c.P);
}

std::string encrypt(const PublicKey& key,
                    const std::vector<uint8_t>& data) {
    const Curve& c = curveFor(key.bits);
    const size_t n = blockBytes(c), w = size_t(key.bits) / 4;
    const size_t blocks = (data.size() + n - 1) / n;

    std::ostringstream out;
    out << "bits: " << key.bits << "\n";
    out << "size: " << data.size() << "\n";
    out << "blocks: " << blocks << "\n";
    for (size_t b = 0; b < blocks; ++b) {
        std::vector<uint8_t> block(n, 0);
        for (size_t i = 0; i < n && b * n + i < data.size(); ++i)
            block[i] = data[b * n + i];
        CipherBlock cb;
        do {
            cb = encryptPoint(c, key.Q, encodeBlock(c, block),
                              randomBelow(c.q));
        } while (cb.C2.infinity);
        out << cb.C1.x.toHex(w) << ' ' << cb.C1.y.toHex(w) << ' '
            << cb.C2.x.toHex(w) << ' ' << cb.C2.y.toHex(w) << "\n";
    }
    return out.str();
}

std::vector<uint8_t> decrypt(const PrivateKey& key,
                             const std::string& cipher) {
    const Curve& c = curveFor(key.bits);
    std::istringstream in(cipher);
    std::string line, k, v;
    size_t size = 0, blocks = 0;
    for (const char* name : {"bits", "size", "blocks"}) {
        if (!std::getline(in, line) || !splitField(line, k, v) || k != name)
            badCipher();
        if (k == "bits" && std::stoi(v) != key.bits)
            throw std::runtime_error(
                "длина ключа не совпадает с шифртекстом");
        if (k == "size") size = std::stoull(v);
        if (k == "blocks") blocks = std::stoull(v);
    }
    const size_t n = blockBytes(c);
    if (blocks != (size + n - 1) / n) badCipher();

    std::vector<uint8_t> out;
    for (size_t b = 0; b < blocks; ++b) {
        std::string x1, y1, x2, y2;
        if (!(in >> x1 >> y1 >> x2 >> y2)) badCipher();
        const CipherBlock cb{
            {BigInt::fromHex(x1), BigInt::fromHex(y1), false},
            {BigInt::fromHex(x2), BigInt::fromHex(y2), false}};
        if (!c.contains(cb.C1) || !c.contains(cb.C2)) badCipher();
        const auto part = decodeBlock(c, decryptPoint(c, key.d, cb));
        out.insert(out.end(), part.begin(), part.end());
    }
    out.resize(size);
    return out;
}
    ```
  ]
]
