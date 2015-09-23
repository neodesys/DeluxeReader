DeluxeReader
============

[![GPLv3 License](https://img.shields.io/badge/License-GPLv3-blue.svg?style=flat)](/LICENSE)

DeluxeReader is a slick comics and ebooks reader for Linux, Windows and Android.

**Contents**

1. [Quick start](#quick-start)
2. [How to contribute](#how-to-contribute)
3. [License](#license)

--------------------------------------------------------------------------------

Quick start
-----------

Download the
[latest release](https://github.com/neodesys/DeluxeReader/releases/latest) or
clone this repository.

```
$ git clone https://github.com/neodesys/DeluxeReader.git DeluxeReader
$ cd DeluxeReader
$ git submodule init
$ git submodule update
```

### Build the executable

```
$ make PLATFORM=linux ARCH=64 BUILD=release
$ make PLATFORM=linux ARCH=32 BUILD=release
$ make PLATFORM=mingw ARCH=64 BUILD=release
$ make PLATFORM=mingw ARCH=32 BUILD=release
```

### Build and run tests

```
$ make PLATFORM=linux ARCH=64 BUILD=release test && bin/Test_DeluxeReader_linux64_release
$ make PLATFORM=linux ARCH=32 BUILD=release test && bin/Test_DeluxeReader_linux32_release
$ make PLATFORM=mingw ARCH=64 BUILD=release test && bin/Test_DeluxeReader_mingw64_release.exe
$ make PLATFORM=mingw ARCH=32 BUILD=release test && bin/Test_DeluxeReader_mingw32_release.exe
```

### Generate source code documentation using [Doxygen](http://www.doxygen.org/)

```
$ make doc && xdg-open doc/generated/html/index.html
```

--------------------------------------------------------------------------------

How to contribute
-----------------

You are welcome to contribute to DeluxeReader.

If you find a bug, have an issue or great ideas to make it better, please
[post an issue](https://guides.github.com/features/issues/).

If you can fix a bug or want to add a feature yourself, please
[fork](https://guides.github.com/activities/forking/) the repository and post a
*Pull Request*.

You can find detailed information about how to contribute in
[GitHub Guides](https://guides.github.com/activities/contributing-to-open-source/).

--------------------------------------------------------------------------------

License
-------

DeluxeReader is released under the [GPLv3 License](/LICENSE).

```
DeluxeReader

Copyright (C) 2015, Loïc Le Page

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```

--------------------------------------------------------------------------------
