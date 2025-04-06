# blackpill-usb-sha256

`blackpill-usb-sha256` &mdash; учебный проект, который демонстрирует подход к написанию
приложений для контроллера
[STM32F411CEU6 (WeAct Black Pill V2.0)](https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0.html)
использующих криптографические функции над данными, получаемыми по USB.
В то же время, `blackpill-usb-sha256` можно использовать в качестве шаблона для
написания таких приложений.

Для реализации криптографических функций и алгоритмов в проект используется библиотека
[WolfSSL](https://www.wolfssl.com/).

Приложение из проекта работает следующим образом:
1. после подачи питания контроллер инициализирует необходимую периферию и библиотеку WolfSSL;
2. контроллер ожидает получения данных по USB;
3. полученные данные накапливаются во внутреннем буфере;
4. последний байт очередного пакета данных сравнивается с символом переноса каретки
    (`\r`, 0xD в таблице ASCII);
5. после успешного сравнения контроллер рассчитывает хэш полученных данных (без
    учета символа переноса каретки) по алгоритму
    [SHA256](https://cryptoteh.ru/sha-256/)
    с использованием WolfSSL;
6. полученный хэш преобразуется в hex-строку (строку, в котором каждому байту хэша
    соответствует 2 символа шестандцатеричного представления числа, т.е. `0xFF` &rarr; `"FF"`);
7. hex-строка отправляется по USB ответным пакетом.

Для сборки этого проекта **не** требуется среда разработки
[STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
и встроенный в нее генератор кода STM32CubeMX.
Код библиотек HAL и CMSIS включен в проект в подмодулем Git, а код инициализации
контроллера предварительно сгенерирован и помещен в статическую библиотеку, которая
собирается в проекте.

## Компиляция проекта

Компиляция проекта состоит из подготовки рабочего окружения и, собственно, компиляции.
Подготовка рабочего окружения происходит один раз.
В нее входит установка кросс-компилятора для процессоров семейства ARM и сборка главной
зависимости проекта &mdash; библиотеки WolfSSL.
Все инструкции написаны для Debian-based операционных систем Linux (Debain, Ubuntu и др.).

### Установка нужных системных пакетов и приложений

1. Установите Python:
    ```console
    $ sudo apt install python3-full python3-pip python-is-python3 pipx
    ```

2. Установите пакеты `build-essential` и CMake:
    ```console
    $ sudo apt install build-essential cmake
    ```

3. Установите Git:
    ```console
    $ sudo apt install git
    ```

### Установка Conan

[Conan](https://docs.conan.io/2/) является пакетным менеджером C и C++.
В этом проекте он используется для установки кросс-компилятора и библиотеки WolfSSL.

> [!NOTE]
> Пакеты Conan устанавливаются в директорию `${HOME}/.conan2` и не мешают системным
> пакетам, установленным с помощью `apt`.
> Для их установки не требуется прав суперпользователя.

Установите Conan:

```console
$ pipx install conan
$ conan --version
Conan version 2.15.0
```

Conan поддерживает кросс-платформенность (возможность скомпилировать программу не для
того же окружения, в котором происходит компиляция).
Для этого он использует файлы, которые называются *профилями*.
Для корректной работы пакетного менеджера необходимо сгенерировать профиль по-умолчанию:

```console
$ conan profile detect --force
$ conan profile list
Profiles found in the cache:
default
```

### Установка кросс-компилятора и профиля Conan для Cortex-M4

Наше приложение будет выполняться на процессоре контроллера
[ARM Cortex-M4](https://en.wikipedia.org/wiki/ARM_Cortex-M).
Нам необходимо получить компилятор, который умеет компилировать код для этого типа
процессора, а также установить профиль Conan для этого процессора.

Склонируйте репозиторий https://github.com/czertyaka/arm-gnu-toolchain:

```console
$ cd $(mktemp -d)
$ git clone https://github.com/czertyaka/arm-gnu-toolchain.git
$ cd arm-gnu-toolchain/
```

Установите профиль `cortex-m4`

```console
$ conan config install conan/
$ conan profile list
Profiles found in the cache:
cortex-m4
default
```

Установите компилятор arm-none-eabi-g++:

```console
$ conan create . -pr:b=default -pr:h=cortex-m4 --build-require
$ conan list "arm-gnu-toolchain/14.2"
Found 1 pkg/version recipes matching arm-gnu-toolchain/14.2 in local cache
Local Cache
  arm-gnu-toolchain
    arm-gnu-toolchain/14.2
```

> [!NOTE]
> Компилятор не будет немедленно доступен в командной строке после установки с
> помощью Conan.
> Это нормально, в большинстве случаев нам не придется вызывать его вручную.

### Установка WolfSSL

Пакет WolfSSL есть в глобальном репозитории Conan пакетов
[Conan Center](https://conan.io/center).
Однако на данный момент его компиляция для Cortex-M4 без операционной системы
не поддерживается.
Я создал
[пулл-реквест](https://github.com/conan-io/conan-center-index/pull/26597)
, в котором добавил такую возможность, но он еще проходит ревью.
Поэтому нам потребуется скомпилировать пакет вручную.

Склонируйте репозиторий https://github.com/czertyaka/conan-center-index:

```console
$ cd $(mktemp -d)
$ git clone -b 'wolfssl/5.7.2' https://github.com/czertyaka/conan-center-index.git
$ cd conan-center-index/recipes/wolfssl/
```

> [!WARNING]
> Клонирование этого репозитория может занять продолжительное время!

Теперь нам потребуется скомпилировать WolfSSL.
Разумно сразу скомпилировать его в трех конфигурациях:

1. Release (умолчательная конфигурация из профиля cortex-m4).
2. RelWithDebInfo &mdash; сборка с оптимизациями и отладочной информацией.
3. MinSizeRel &mdash; сборка с минимальным размером библиотеки.
    Для нас очень важен размер, ведь на контроллере всего 512KiB Flash-памяти!

Создайте пакеты WolfSSL для Cortex-M4:

```console
$ conan create ./all/ -pr:h cortex-m4 --version 5.7.2 --build=missing
$ conan create ./all/ -pr:h cortex-m4 --version 5.7.2 --build=missing -s build_type=RelWithDebInfo
$ conan create ./all/ -pr:h cortex-m4 --version 5.7.2 --build=missing -s build_type=MinSizeRel
```

<details>
<summary>Проверка установленных пакетов:</summary>

```console
$ conan list "wolfssl/5.7.2"
Local Cache
  wolfssl
    wolfssl/5.7.2
      revisions
        0ca6a9d2a5d8006ac8480ab9fae988ba (2025-04-06 12:23:26 UTC)
          packages
            80360945a9c858d98da83c0762cae9f333851dfa
              info
                settings
                  arch: armv7
                  build_type: RelWithDebInfo
                  compiler: gcc
                  compiler.version: 14.2
                  os: baremetal
                options
                  alpn: False
                  certgen: False
                  des3: False
                  dsa: False
                  fPIC: True
                  opensslall: False
                  opensslextra: False
                  ripemd: False
                  sessioncerts: False
                  sni: False
                  sslv3: False
                  testcert: False
                  tls13: False
                  with_curl: False
                  with_experimental: False
                  with_quic: False
                  with_rpk: False
            84d6bbcff874f071643a9b90499ff0694fd64ace
              info
                settings
                  arch: armv7
                  build_type: MinSizeRel
                  compiler: gcc
                  compiler.version: 14.2
                  os: baremetal
                options
                  alpn: False
                  certgen: False
                  des3: False
                  dsa: False
                  fPIC: True
                  opensslall: False
                  opensslextra: False
                  ripemd: False
                  sessioncerts: False
                  sni: False
                  sslv3: False
                  testcert: False
                  tls13: False
                  with_curl: False
                  with_experimental: False
                  with_quic: False
                  with_rpk: False
            927f2de093a0ddacd58e0137e337437548c915aa
              info
                settings
                  arch: armv7
                  build_type: Release
                  compiler: gcc
                  compiler.version: 14.2
                  os: baremetal
                options
                  alpn: False
                  certgen: False
                  des3: False
                  dsa: False
                  fPIC: True
                  opensslall: False
                  opensslextra: False
                  ripemd: False
                  sessioncerts: False
                  sni: False
                  sslv3: False
                  testcert: False
                  tls13: False
                  with_curl: False
                  with_experimental: False
                  with_quic: False
                  with_rpk: False
```
</details>

### Компиляция `blackpill-usb-sha256`

Теперь можно скомпилировать учебный проект.
Склонируйте его:

```console
$ cd ~
$ cd blackpill-usb-sha256
$ git clone --recursive https://github.com/czertyaka/blackpill-usb-sha256.git
$ cd blackpill-usb-sha256
```

> [!WARNING]
> Клонирование этого репозитория может занять продолжительное время!

Соберите проект:

```console
$ conan build . -pr:h cortex-m4 -s build_type=MinSizeRel
```

В проекте должна появиться директория `build/MinSizeRel/`, а в ней &mdash; файлы `main.bin`
и `main.elf`:

```console
$ ls -1 build/MinSizeRel/
CMakeCache.txt
CMakeFiles
Makefile
board
cmake_install.cmake
generators
main.bin
main.elf
metadata
os
stripped.elf
usb-sha256
$ file build/Release/main.bin
build/Release/main.bin: data
$ file build/Release/main.elf
build/Release/main.elf: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), statically linked, not stripped
```

## Прошивка контроллера

На прошлом этапе мы получили файл `main.bin`, который и является прошивкой контроллера.
Он должен быть скопирован с файловой системы компьютера в Flash-память устройства.

Копирование происходит c помощью утилиты `st-flash` из пакета `stlink-tools`, установим его:

```console
$ sudo apt install stlink-tools
```

Далее необходимо подключить программатор ST-LINK V2 Clone к компьютеры, соединить его
выводы с помощью перемычек с SWD-интерфейсом контроллера.

![connect-stlink](docs/connect-debugger.webp)

Для удобства прошивки в CMake была добавлена специальная цель `flash`.
Чтобы ее выполнить, необходимо активировать виртуальное окружение, которое
Conan создал для сборки проекта:

```console
$ cd build/MinSizeRel/
$ source generators/conanbuild.sh
```

После активации окружения станут работать команды компиляции и будет доступен
компилятор:

```console
$ arm-none-eabi-g++ --version
arm-none-eabi-g++ (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

Выполните цель `flash`:

```console
$ make flash
[  5%] Built target startup
[ 70%] Built target board
[ 79%] Built target usb-sha256
[ 88%] Built target os
[ 94%] Built target main
st-flash 1.8.0
2025-04-06T18:13:13 INFO common.c: STM32F411xC_xE: 128 KiB SRAM, 512 KiB flash in at least 16 KiB pages.
file main.bin md5 checksum: 7cf27f10551d5be84c441f1839c6f49d, stlink checksum: 0x01ef4133
2025-04-06T18:13:13 INFO common_flash.c: Attempting to write 299464 (0x491c8) bytes to stm32 address: 134217728 (0x8000000)
EraseFlash - Sector:0x0 Size:0x4000 -> Flash page at 0x8000000 erased (size: 0x4000)
EraseFlash - Sector:0x1 Size:0x4000 -> Flash page at 0x8004000 erased (size: 0x4000)
EraseFlash - Sector:0x2 Size:0x4000 -> Flash page at 0x8008000 erased (size: 0x4000)
EraseFlash - Sector:0x3 Size:0x4000 -> Flash page at 0x800c000 erased (size: 0x4000)
EraseFlash - Sector:0x4 Size:0x10000 -> Flash page at 0x8010000 erased (size: 0x10000)
EraseFlash - Sector:0x5 Size:0x20000 -> Flash page at 0x8020000 erased (size: 0x20000)
EraseFlash - Sector:0x6 Size:0x20000 -> Flash page at 0x8040000 erased (size: 0x20000)

2025-04-06T18:13:20 INFO flash_loader.c: Starting Flash write for F2/F4/F7/L4
2025-04-06T18:13:20 INFO flash_loader.c: Successfully loaded flash loader in sram
2025-04-06T18:13:20 INFO flash_loader.c: Clear DFSR
2025-04-06T18:13:20 INFO flash_loader.c: enabling 32-bit flash writes
2025-04-06T18:13:24 INFO common_flash.c: Starting verification of write complete
2025-04-06T18:13:27 INFO common_flash.c: Flash written and verified! jolly good!
2025-04-06T18:13:27 INFO common.c: Go to Thumb mode
[100%] Built target flash
```

Готово!
Прошивка попала на контроллер.

## Тестирование прошивки

Теперь можно попробовать что-нибудь отправить на контроллер и посмотреть, какую
хэш-сумму он посчитает.
Отсоедините контроллер от программатора и с помощью Type-C соедините с USB-портом
комьютера.

> [!WARNING]
> Если не отсоединить контроллер от программатора и сразу подключить к USB через Type-C,
> то контроллер может сломаться.

После подключения контроллера по USB на компьютере должно появиться устройство
с именем `/dev/ttyUSB0` или `/dev/ttyACM0`:

 ```console
$ ls /dev/ttyACM0
/dev/ttyACM0
```

Подключаться к нему будем с помощью picocom.
Установим его:

 ```console
$ sudo apt install picocom
```

Запустим picocom:

```console
$ picocom --echo /dev/ttyACM0
picocom v2024-07

port is        : /dev/ttyACM0
flowcontrol    : none
baudrate is    : 9600
parity is      : none
databits are   : 8
stopbits are   : 1
txdelay is     : 0 ns
escape is      : C-a
local echo is  : yes
noinit is      : no
noreset is     : no
hangup is      : no
nolock is      : no
send_cmd is    : sz -vv
receive_cmd is : rz -vv -E
imap is        :
omap is        :
emap is        : crcrlf,delbs,
logfile is     : none
initstring     : none
exit_after is  : not set
exit is        : no
minimal cmds is: no

Type [C-a] [C-h] to see available commands
Terminal ready
```

Посла запуска picocom все печатаемые символы будут отправлены на контроллер и будут
им обрабатываться.
Для выхода из picocom нужно нажать последовательность символов Ctrl+a и Ctrl-x.

Наберем `hello` и нажем Enter:

```console
hello
957d8a8404a598792d5a7b0ae06bad7e61b14ee26780928b966819b9a4ee784f
```

Контроллер ответил последовательностью символов
`957d8a8404a598792d5a7b0ae06bad7e61b14ee26780928b966819b9a4ee784f`,
которая является хэш-суммой строки `hello`.
Проверим это:

```console
$ printf "hello" | sha256sum
2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824  -
```

Хэш-сумма, вычисленная на компьютере, совпадает с хэш-суммой, вычисленной
на контроллере.

## Отладка

Иногда для поиска ошибок в коде бывает полезно исследовать код с помощью
отладчика.
В нашем проекте для отладки будем использовать `arm-none-eabi-gdb`.
Он работает точно так же как и обычный GDB, про который можно почитать
[тут](https://losst.pro/kak-polzovatsya-gdb).

Соберите проект с отладочной информацией:

```console
$ conan build . -pr:h cortex-m4 -s build_type=RelWithDebInfo
```

Обратите внимание, как вырос размер прошивки:

```console
$ du -sh build/RelWithDebInfo/main.bin
332K    build/RelWithDebInfo/main.bin
$ du -sh build/MinSizeRel/main.bin
296K    build/MinSizeRel/main.bin
```

Зайдите в директорию `build/RelWithDebInfo`, активируйте виртуальное окружение
и залейте отладочную прошивку на контроллер:

```console
$ cd build/RelWithDebInfo/
$ source generators/conanbuild.sh
$ make flash
```

В отдельном окне терминала выполните:

```console
$ st-util --connect-under-reset
```

Из директории `build/RelWithDebInfo` выполните подключение к контроллеру:

```console
$ arm-none-eabi-gdb --commands=../../gdb.txt
```

После этого вы окажетесь в интерактивной консоли GDB и можно отлаживать прошивку.

> [!WARNING]
> Если хотите отладить прошивку с подключенным Type-C, то отсоедините от контроллера
перемычку программатора 3.3V.

## Структура проекта

В проекте собирается несколько статичеких библиотек, которые затем линкуются
в ELF-файл, из которого, в свою очередь, получается прошивка `main.bin`.
Разберемся, что это за библиотеки и зачем они нужны.

### Библиотека board

В эту библиотеку попадает все, что связано с инициализацией контроллера и работа
с периферией.

Библиотека предоставляет функцию `void board_main(void)`, объявление которой
находится в `board/include/board/main.h`.
Эта была сгенерирована с помощью STM32CubeMX и переименована из `main` в
`board_main`.
В ней происходит инициализация HAL, GPIO, USB и др.

Еще одна полезная функция из этой библиотеки:

```c
typedef void (*usb_receive_callback_t)(const uint8_t *const, const uint32_t);
void board_set_usb_receive_callback(usb_receive_callback_t callback);
```

из файла `board/include/board/usbd_cdc_if.h`.
Эта функция совместно с псевдонимом типа позволяет во время выполнения задать собственный
обработчик прерывания получения данных по USB.

В библиотеке также реализована следующая функциональность: по получению данных по USB
загорается светодиод контроллера.
Сразу после этого стартует таймер TIM2, по истечении которого светодиод гаснет.

### Библиотека os

Эта библиотека эмулирует работу операционной системы функциями-"заглушками" в файле
`os/src/syscalls.c`.
Большинство из этих функций не выполняют никакую работу и нужны только для того, что
проект собирался.

В файле `os/src/sysmem.c` определена функция `void *_sbrk(ptrdiff_t incr)`.
Эта функция вызывается каждый раз, когда происходит аллокация памяти.
Прочитав внимательно её код, можно понять, что злоупотреблять динамической памятью
в проекте не стоит :upside_down_face:

### Библиотека usb-sha256

В этой библиотеке определяются классы прикладного уровня &mdash; механизмы, которые
относятся к логике нашего приложения и не зависят напрямую от аппаратной части.
В отличие от библиотек board и os, эта библиотека написана на C++.

#### Класс `FastBuffer`

`usb-sha256/include/usb-sha256/fast_buffer.hpp`

Шаблонный класс `FastBuffer` позволяет сохранять данные в один и тот же участок памяти
без предварительного очистки этого участка.
Он всегда хранит актуальный размер полезных данных в буфере вне зависимости от его
содержимого.

#### Класс `Sha256Sum`

`usb-sha256/include/usb-sha256/sha256sum.hpp`
`usb-sha256/src/sha256sum.cpp`

Этот класс позволяет посчитать хэш-сумму произвольного массива байт.
Он инкапсулирует работу с библиотекой WolfSSL.

#### Класс `HexString`

`usb-sha256/include/usb-sha256/hex_string.hpp`

Класс `HexSrting` позволяет получить hex-строку произвольного массива байт без
динамической аллокации памяти.

#### Класс `Usb`

`usb-sha256/include/usb-sha256/usb.hpp`
`usb-sha256/src/usb.cpp`

Класс `Usb` является, пожалуй, самым сложным из перечисленных.
Он инкапсулирует работу с устройством USB и является потребителем функции
`board_set_usb_receive_callback` библиотеки board.

Метод `instance` возвращает ссылку на объект `Usb`, который нельзя создать вызовом
конструктора (такой подход называется
[Singletone](https://en.wikipedia.org/wiki/Singleton_pattern)).
На контроллере есть только одно USB-устройство, поэтому объект такого класса
тоже всегда один.

Метод `WaitReceiving` блокирует управление в ожидании приема нового пакета данных
по USB.
Получить принятые данные можно с помощью метода `GetBuffer`, а очистить его
&mdash; с помощью метода `ClearBuffer`.
Размер внутреннего буфера равен 1KiB.

Перегруженные методы `Transmit` нужны для передачи в USB ответных пакетов.
Один из них принимает массив байт, а второй &mdash; строку.

#### `main.cpp`

В этом файле определена логика приложения.
Сначала инициализируется периферия:

```c++
    board_main();
```

а затем &mdash; библиотека WolfSSL:

```c++
    wolfSSL_Init();
```

После этого получаем Singleton-объект класса `Usb`:

```c++
    Usb& usb = Usb::instance();
```

Далее в бесконечном циле ожидаем приема нового сообщения:

```c++
    while (true) {
        usb.WaitReceiving();
```

и сравниваем последний байт в буфере с символом переноса каретки:

```c++
        if (*data.rbegin() == `\r`) {
```

Далее в теле условия формируем хэш-сумму, из нее hex-строку и отправляем обратно
по USB:

```c++
            usb.Transmit(hex.String());
```
