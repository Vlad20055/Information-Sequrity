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

  к лабораторной работе №6

  на тему

  #v(1.15em)

  #upper(
    [*ЦИФРОВАЯ ПОДПИСЬ*]

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

В данной лабораторной работе рассматривается электронная цифровая подпись (ЭЦП) на основе алгоритма ГОСТ Р 34.10-2012. Цифровая подпись – это реквизит электронного документа, полученный в результате криптографического преобразования и позволяющий проверить, что документ подписан владельцем определённого ключа и не изменялся после подписания. В отличие от хеш-значения, которое может вычислить любой, подпись может сформировать только тот, кто знает закрытый ключ.

Схемы цифровой подписи являются асимметричными. Владелец формирует пару ключей: закрытый ключ хранится в секрете и используется для формирования подписи, а открытый ключ распространяется свободно и используется для её проверки. Подпись вычисляется не от самого документа, а от его хеш-значения, поэтому любое изменение документа делает подпись недействительной. Таким образом, цифровая подпись обеспечивает контроль целостности документа, подтверждение его авторства и невозможность отказа от авторства.

ГОСТ Р 34.10-2012 – стандарт Российской Федерации, описывающий процессы формирования и проверки цифровой подписи. Он пришёл на смену стандарту ГОСТ Р 34.10-2001 и принят также в качестве межгосударственного стандарта ГОСТ 34.10-2018. Алгоритм работает в группе точек эллиптической кривой над конечным простым полем. Стандарт предусматривает две длины ключа – 256 и 512 бит, а для хеширования используется функция ГОСТ Р 34.11-2012 с длиной выхода 256 или 512 бит соответственно. Стойкость алгоритма основана на сложности задачи дискретного логарифмирования в группе точек эллиптической кривой: зная точки $P$ и $Q = d P$, вычислительно невозможно найти число $d$.

Целью лабораторной работы является разработка программного средства на языке #box[C++], выполняющего генерацию ключей, формирование и проверку цифровой подписи файлов на основе алгоритма ГОСТ Р 34.10-2012.

#pagebreak(weak: true)

= Выполнение работы
В ходе выполнения лабораторной работы было разработано программное средство на языке #box[C++] для формирования и проверки цифровой подписи. Все алгоритмы реализованы без использования сторонних библиотек: арифметика больших чисел, операции с точками эллиптической кривой и хеш-функция ГОСТ Р 34.11-2012, реализованная в предыдущей лабораторной работе. Программа поддерживает генерацию ключевой пары длиной 256 или 512 бит, подпись файла закрытым ключом, проверку подписи открытым ключом и проверку реализации на контрольном примере из стандарта.

Программа разделена на несколько модулей. Модуль bigint реализует арифметику больших целых чисел, модуль ec – операции с точками эллиптической кривой и наборы параметров, модуль random – генерацию случайных чисел, модуль streebog – хеш-функцию, модуль gost3410 – генерацию ключей, формирование и проверку подписи, а главный модуль main.cpp отвечает за разбор аргументов командной строки и вывод результатов.

В модуле bigint большое число хранится как массив 32-битных слов, начиная с младшего. Сложение, вычитание и умножение выполняются «столбиком» с переносом в 64-битной переменной, а деление с остатком – по алгоритму Кнута. На их основе реализованы операции по модулю: сложение, вычитание, умножение и возведение в степень методом «возведения в квадрат и умножения». Модули $p$ и $q$ являются простыми числами, поэтому обратный элемент вычисляется по малой теореме Ферма:

$a^(-1) = a^(p-2) mod p.$

В стандарте используется эллиптическая кривая в форме Вейерштрасса над простым полем $"GF"(p)$:

$y^2 = x^3 + a x + b mod p.$

Параметрами схемы являются модуль $p$, коэффициенты $a$ и $b$, простой порядок $q$ циклической подгруппы точек кривой и точка $P = (x_P, y_P)$ порядка $q$, порождающая эту подгруппу. Точки кривой образуют группу, в которой нейтральным элементом является бесконечно удалённая точка $O$. Сумма точек $(x_1, y_1)$ и $(x_2, y_2)$ вычисляется по формулам

$x_3 = lambda^2 - x_1 - x_2, quad y_3 = lambda (x_1 - x_3) - y_1,$

где $lambda = (y_2 - y_1) / (x_2 - x_1)$ при сложении различных точек и $lambda = (3 x_1^2 + a) / (2 y_1)$ при удвоении точки. В этих формулах на каждом шаге требуется вычисление обратного элемента, поэтому в программе точки представляются в якобиевых координатах $(X, Y, Z)$, где $x = X / Z^2$, $y = Y / Z^3$. В таком представлении сложение и удвоение выполняются только умножениями, а обратный элемент вычисляется один раз при переводе результата обратно в обычные координаты. Произведение $k P$ вычисляется методом «удвоения и сложения» по двоичной записи числа $k$.

В программе используются два набора параметров: для ключей длиной 256 бит – набор id-GostR3410-2001-CryptoPro-A-ParamSet с модулем $p = 2^(256) - 617$, для ключей длиной 512 бит – набор id-tc26-gost-3410-12-512-paramSetA с модулем $p = 2^(512) - 569$. Кроме того, для проверки реализации используются параметры контрольного примера из приложения стандарта.

Генерация ключей выполняется функцией generateKeys. Закрытым ключом является случайное число $d$, $0 < d < q$, а открытым – точка кривой

$Q = d P.$

Для получения случайных чисел используется криптостойкий генератор операционной системы. Ключи сохраняются в текстовые файлы: открытый ключ содержит длину ключа и координаты точки $Q$, закрытый – длину ключа и число $d$.

Формирование подписи сообщения $M$ выполняется функцией sign по алгоритму стандарта:

+ Вычислить хеш-значение $overline(h) = H(M)$ по ГОСТ Р 34.11-2012 с длиной выхода, равной длине ключа.
+ Вычислить число $alpha$, двоичным представлением которого является $overline(h)$, и определить $e = alpha mod q$. Если $e = 0$, то принять $e = 1$.
+ Сгенерировать случайное число $k$, $0 < k < q$.
+ Вычислить точку $C = k P$ и определить $r = x_C mod q$. Если $r = 0$, вернуться к шагу 3.
+ Вычислить $s = (r d + k e) mod q$. Если $s = 0$, вернуться к шагу 3.
+ Подписью сообщения является пара чисел $(r, s)$.

Проверка подписи выполняется функцией verify. Сначала проверяется, что $0 < r < q$ и $0 < s < q$, затем по сообщению вычисляется число $e$ так же, как при подписи, и выполняются вычисления

$v = e^(-1) mod q, quad z_1 = s v mod q, quad z_2 = -r v mod q,$

$C = z_1 P + z_2 Q, quad R = x_C mod q.$

Подпись признаётся верной, если $R = r$. Правильность проверки следует из того, что для верной подписи

$z_1 P + z_2 Q = (s v - r v d) P = v (s - r d) P = v k e P = k P.$

Если сообщение было изменено, то изменится число $e$, и полученная точка $C$ не совпадёт с точкой $k P$, использованной при подписи.

Для запуска программы предусмотрены следующие операции:

```
gost3410 keygen [-b 256|512] public.key private.key
gost3410 sign private.key input.txt input.sig
gost3410 verify public.key input.txt input.sig
gost3410 test
```

Первая команда создаёт ключевую пару, вторая формирует подпись файла, третья проверяет подпись, а четвёртая выполняет проверку реализации. Для этого используется контрольный пример из приложения ГОСТ Р 34.10-2012: по известным параметрам кривой, закрытому ключу $d$, числу $e$ и случайному числу $k$ вычисляются открытый ключ $Q$ и подпись $(r, s)$, которые сравниваются с приведёнными в стандарте, после чего проверяется, что эталонная подпись принимается, а подпись с изменённым значением $s$ отклоняется. Затем для обоих наборов параметров генерируются случайные ключи, подписывается сообщение и проверяется, что подпись верна для исходного сообщения и неверна для изменённого. Результат проверки продемонстрирован на рисунке 1.1.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/test.png"),
    caption: [Проверка реализации]
  )
]

Все вычисленные значения совпали с приведёнными в стандарте, что подтверждает правильность реализации. Генерация ключевой пары и содержимое файла открытого ключа продемонстрированы на рисунке 1.2.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/keygen.png"),
    caption: [Генерация ключевой пары]
  )
]

Далее тестовый файл input.txt был подписан закрытым ключом, что продемонстрировано на рисунке 1.3. Поскольку число $k$ выбирается случайно, при повторном подписании того же файла получается другая подпись, которая также является верной.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/sign.png"),
    caption: [Формирование подписи файла]
  )
]

Проверка подписи исходного файла открытым ключом завершилась успешно, что продемонстрировано на рисунке 1.4.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/verify_ok.png"),
    caption: [Проверка подписи исходного файла]
  )
]

Затем был создан файл modified.txt, в котором по сравнению с исходным изменена сумма в последней строке. Как видно на рисунке 1.5, подпись для изменённого файла признана неверной.

#block(breakable: false, above: 1.65em, below: 2.3em)[
  #figure(
    image("assets/verify_fail.png"),
    caption: [Проверка подписи изменённого файла]
  )
]

Таким образом, в ходе работы была реализована схема цифровой подписи ГОСТ Р 34.10-2012 на эллиптических кривых с длиной ключа 256 и 512 бит с поддержкой работы с файлами.

#pagebreak(weak: true)

#stp2024.heading_unnumbered[Вывод]

В ходе выполнения лабораторной работы был изучен алгоритм цифровой подписи ГОСТ Р 34.10-2012 и разработано программное средство на языке #box[C++], реализующее генерацию ключевой пары, формирование и проверку цифровой подписи файлов.

В программе самостоятельно реализованы арифметика больших целых чисел, операции сложения, удвоения и умножения точек эллиптической кривой в якобиевых координатах, а также алгоритмы формирования и проверки подписи с использованием хеш-функции ГОСТ Р 34.11-2012. Корректность реализации подтверждена совпадением результатов с контрольным примером из стандарта.

Проведённое тестирование показало, что подпись исходного файла успешно проходит проверку, а изменение содержимого файла или использование другого открытого ключа приводит к тому, что подпись признаётся неверной.

В результате выполнения лабораторной работы были получены практические навыки реализации асимметричных криптографических алгоритмов на эллиптических кривых, работы с большими целыми числами и организации формирования и проверки цифровой подписи электронных документов.

#stp2024.appendix(type:[обязательное], title:[Листинг программного кода])[

#stp2024.listing[gost3410.h][
    ```
#pragma once

#include "bigint.h"
#include "ec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gost {

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

struct Signature {
    BigInt r, s;

    void save(const std::string& path, int bits) const;
    static Signature load(const std::string& path);
};

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub);

BigInt messageToE(const std::vector<uint8_t>& msg, const Curve& c);
bool signWithK(const Curve& c, const BigInt& d, const BigInt& e,
               const BigInt& k, Signature& sig);
bool verifyE(const Curve& c, const Point& Q, const BigInt& e,
             const Signature& sig);

Signature sign(const PrivateKey& key, const std::vector<uint8_t>& msg);
bool verify(const PublicKey& key, const std::vector<uint8_t>& msg,
            const Signature& sig);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}
    ```
  ]

#stp2024.listing[gost3410.cpp][
    ```
#include "gost3410.h"

#include "random.h"
#include "streebog.h"

#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>

namespace gost {

namespace {

using Fields = std::map<std::string, std::string>;

Fields readFields(const std::string& path) {
    const auto data = readFile(path);
    std::istringstream in(std::string(data.begin(), data.end()));
    Fields f;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const size_t colon = line.find(": ");
        if (colon == std::string::npos)
            throw std::runtime_error(path + ": неверная строка \"" + line +
                                     "\"");
        f[line.substr(0, colon)] = line.substr(colon + 2);
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

void writeText(const std::string& path, const std::string& text) {
    writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
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
    const Curve& c = curveFor(k.bits);
    if (k.d.isZero() || !(k.d < c.q))
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

void Signature::save(const std::string& path, int bits) const {
    std::ostringstream out;
    out << "r: " << r.toHex(bits / 4) << "\n";
    out << "s: " << s.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

Signature Signature::load(const std::string& path) {
    const Fields f = readFields(path);
    return {BigInt::fromHex(field(f, "r", path)),
            BigInt::fromHex(field(f, "s", path))};
}

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub) {
    const Curve& c = curveFor(bits);
    priv.bits = pub.bits = bits;
    priv.d = randomBelow(c.q);
    pub.Q = c.multiply(priv.d, c.P);
}

BigInt messageToE(const std::vector<uint8_t>& msg, const Curve& c) {
    const BigInt alpha = BigInt::fromBytesLE(Streebog::hash(msg, c.bits));
    const BigInt e = alpha % c.q;
    return e.isZero() ? BigInt(1) : e;
}

bool signWithK(const Curve& c, const BigInt& d, const BigInt& e,
               const BigInt& k, Signature& sig) {
    const Point C = c.multiply(k, c.P);
    sig.r = C.x % c.q;
    if (sig.r.isZero()) return false;
    sig.s = addMod(mulMod(sig.r, d, c.q), mulMod(k, e, c.q), c.q);
    return !sig.s.isZero();
}

bool verifyE(const Curve& c, const Point& Q, const BigInt& e,
             const Signature& sig) {
    if (sig.r.isZero() || sig.s.isZero() || !(sig.r < c.q) ||
        !(sig.s < c.q))
        return false;
    const BigInt v = invMod(e, c.q);
    const BigInt z1 = mulMod(sig.s, v, c.q);
    const BigInt z2 = subMod(0, mulMod(sig.r, v, c.q), c.q);
    const Point C = c.add(c.multiply(z1, c.P), c.multiply(z2, Q));
    if (C.infinity) return false;
    return C.x % c.q == sig.r;
}

Signature sign(const PrivateKey& key, const std::vector<uint8_t>& msg) {
    const Curve& c = curveFor(key.bits);
    const BigInt e = messageToE(msg, c);
    Signature sig;
    while (!signWithK(c, key.d, e, randomBelow(c.q), sig)) {
    }
    return sig;
}

bool verify(const PublicKey& key, const std::vector<uint8_t>& msg,
            const Signature& sig) {
    const Curve& c = curveFor(key.bits);
    return verifyE(c, key.Q, messageToE(msg, c), sig);
}

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("не удаётся открыть файл: " + path);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("не удаётся создать файл: " + path);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out) throw std::runtime_error("ошибка записи в файл: " + path);
}

}
    ```
  ]
]
