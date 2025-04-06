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
    Для нас очень важне размер, ведь на контроллере всего 512KiB Flash-памяти!

Создайте пакет WolfSSL для Cortex-M4:

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
$ conan build . -pr:h cortex-m4
```

В проекте должна появиться директория `build/Release/`, а в ней &mdash; файлы `main.bin`
и `main.elf`:

```console
$ ls -1 build/Release/
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
