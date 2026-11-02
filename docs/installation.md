# Installation Guide

[← Back to README](https://github.com/FMazz97/CS123x/blob/main/README.md)

CS123x can be installed from the Arduino Library Manager, the ESP Component Registry and the PlatformIO Registry, or used from a local copy of the repository.

## Arduino IDE

[![Arduino Library Manager](https://www.ardu-badge.com/badge/CS123x.svg?)](https://www.ardu-badge.com/CS123x)

- **Via Library Manager (recommended):** Open the **Library Manager** (`Ctrl+Shift+I`), search for `CS123x`, and click **Install**.

- **Manual installation:** Download or clone this repository into your Arduino `libraries` folder (`Documents/Arduino/libraries/CS123x`), then restart the IDE.

## ESP Component Registry

[![Component Registry](https://components.espressif.com/components/fmazz97/cs123x/badge.svg)](https://components.espressif.com/components/fmazz97/cs123x)

The library is published on [ESP Component Registry](https://components.espressif.com/components/fmazz97/cs123x) as `cs123x`, without any Arduino dependency.

- **Via `idf.py` (recommended):** In your project directory, run:

  ```bash
  idf.py add-dependency "fmazz97/cs123x^2.0.0"
  ```

  This adds `cs123x` as a dependency of the `main` component.

- **Manual:** add directly to `main/idf_component.yml`:

  ```YAML
  dependencies:
    fmazz97/cs123x: "^2.0.0"
  ```

- **Local development:** To work with a local copy of the library, clone the repository into a folder named `cs123x` in all lowercase:

  ```bash
  git clone https://github.com/FMazz97/CS123x.git cs123x
  ```

  > ESP‑IDF uses the folder name as the component name. The directory must be named cs123x in lowercase, otherwise the component will not be detected.
  > For more details, see: [Local folder name vs. GitHub repository name](https://github.com/FMazz97/arduino-espidf-dual-library-guide/blob/main/chapters/02-repo-structure.md#22-local-folder-name-vs-github-repository-name) and [IDF Dependency Resolution](https://github.com/FMazz97/arduino-espidf-dual-library-guide/blob/main/appendix/A-esp-idf-dependency-resolution.md).

  Then reference the local component path directly in your project's `main/idf_component.yml` manifest:

  ```YAML
  dependencies:
    fmazz97/cs123x:
      version: "*"
      override_path: "../../path/to/cs123x"
  ```

## PlatformIO

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fmazz97/library/cs123x.svg)](https://registry.platformio.org/libraries/fmazz97/cs123x)

The library is published on the [PlatformIO Registry](https://registry.platformio.org/libraries/fmazz97/cs123x) and works with **both** `framework = arduino` and `framework = espidf`.

- **Via PlatformIO Registry (Arduino or ESP‑IDF — recommended):** Add it to your `platformio.ini` via `lib_deps`:

  ```ini
  ; Pin to a specific version for reproducible builds (recommended)
  lib_deps = fmazz97/cs123x@^2.0.0
  framework = ...
  ```

  This works for both frameworks without changes.

- **Using native ESP‑IDF inside PlatformIO:** in your `platformio.ini` declare:

  ```ini
  framework = espidf
  ```

  Then integrate the component using the native ESP‑IDF dependency system, exactly as described in the [ESP Component Registry](#esp-component-registry) section above.

> For version pinning, alternative sources (Git, local folder), and other options, see the official guide on [declaring dependencies](https://docs.platformio.org/en/latest/librarymanager/dependencies.html#declaring-dependencies).
  > For a comprehensive deep dive into how [PlatformIO, Component Manager, and CMake handle dependency resolution](https://github.com/FMazz97/arduino-espidf-dual-library-guide/blob/main/chapters/04-examples.md#43-platformio-component-manager-and-cmake-are-independent-dependency-resolution-systems), consult the [Arduino - ESP-IDF Dual Library Guide](https://github.com/FMazz97/arduino-espidf-dual-library-guide).
