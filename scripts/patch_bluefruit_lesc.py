"""Make Bluefruit LESC support compile-time optional.

The bundled Bluefruit library initializes CryptoCell and generates a P-256
key pair whenever NRF_CRYPTOCELL is available, even when the application later
selects legacy static-passkey pairing.  Patch the downloaded framework package
before it is compiled so unused LESC/ECC code can be removed by the linker.

BLUEFRUIT_LESC_ENABLED defaults to 0.  A target that needs LESC can restore the
original behavior with -D BLUEFRUIT_LESC_ENABLED=1.
"""

from pathlib import Path

Import("env")


def replace_once(text, old, new, path):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            "Bluefruit LESC patch expected one matching block in {} but found {}"
            .format(path, count)
        )
    return text.replace(old, new, 1)


framework_dir = env.PioPlatform().get_package_dir(
    "framework-arduinoadafruitnrf52"
)
if not framework_dir:
    raise RuntimeError("Adafruit nRF52 framework package was not found")

bluefruit_dir = (
    Path(framework_dir) / "libraries" / "Bluefruit52Lib" / "src"
)
header_path = bluefruit_dir / "BLESecurity.h"
source_path = bluefruit_dir / "BLESecurity.cpp"

header = header_path.read_text(encoding="utf-8")
source = source_path.read_text(encoding="utf-8")

if "BLUEFRUIT_LESC_ENABLED" not in header:
    header = replace_once(
        header,
        '#include "utility/bonding.h"\n\n#ifdef NRF_CRYPTOCELL\n'
        '#include "Adafruit_nRFCrypto.h"\n#endif',
        '#include "utility/bonding.h"\n\n'
        '#ifndef BLUEFRUIT_LESC_ENABLED\n'
        '#define BLUEFRUIT_LESC_ENABLED 0\n'
        '#endif\n\n'
        '#if BLUEFRUIT_LESC_ENABLED && defined(NRF_CRYPTOCELL)\n'
        '#define BLUEFRUIT_LESC_SUPPORTED 1\n'
        '#include "Adafruit_nRFCrypto.h"\n'
        '#else\n'
        '#define BLUEFRUIT_LESC_SUPPORTED 0\n'
        '#endif',
        header_path,
    )

    private_guard = "#ifdef NRF_CRYPTOCELL\n    nRFCrypto_ECC_PrivateKey"
    header = replace_once(
        header,
        private_guard,
        "#if BLUEFRUIT_LESC_SUPPORTED\n    nRFCrypto_ECC_PrivateKey",
        header_path,
    )
    header_path.write_text(header, encoding="utf-8")

if "BLUEFRUIT_LESC_SUPPORTED" not in source:
    source = replace_once(
        source,
        "#ifdef NRF_CRYPTOCELL\n"
        "  #define LESC_SUPPORTED    1\n"
        "#else\n"
        "  #define LESC_SUPPORTED    0\n"
        "#endif\n\n",
        "",
        source_path,
    )
    source = replace_once(
        source,
        ".lesc         = LESC_SUPPORTED,",
        ".lesc         = BLUEFRUIT_LESC_SUPPORTED,",
        source_path,
    )

    guard_count = source.count("#ifdef NRF_CRYPTOCELL")
    if guard_count != 3:
        raise RuntimeError(
            "Bluefruit LESC patch expected three CryptoCell guards in {} but found {}"
            .format(source_path, guard_count)
        )
    source = source.replace(
        "#ifdef NRF_CRYPTOCELL", "#if BLUEFRUIT_LESC_SUPPORTED"
    )
    source_path.write_text(source, encoding="utf-8")

required_header_tokens = (
    "#define BLUEFRUIT_LESC_ENABLED 0",
    "#define BLUEFRUIT_LESC_SUPPORTED 1",
    "#if BLUEFRUIT_LESC_SUPPORTED",
)
required_source_tokens = (
    ".lesc         = BLUEFRUIT_LESC_SUPPORTED,",
    "#if BLUEFRUIT_LESC_SUPPORTED",
)

header = header_path.read_text(encoding="utf-8")
source = source_path.read_text(encoding="utf-8")
if not all(token in header for token in required_header_tokens):
    raise RuntimeError("Bluefruit LESC header patch validation failed")
if not all(token in source for token in required_source_tokens):
    raise RuntimeError("Bluefruit LESC source patch validation failed")
