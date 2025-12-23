# Aaru.Checksums.Native

This repository contains the Aaru.Checksums.Native library for [Aaru](https://www.aaru.app).

The purpose of this library is to provide checksums and hashing algorithms for Aaru.

No archiver processing code should fall here, those go in [Aaru.Checksums](https://github.com/aaru-dps/Aaru.Checksums).

To build you just need Docker on Linux and run `build.sh`, that will generate a NuGet package for use with
Aaru.Checksums.

Currently implemented algorithms are:

- Adler-32
- CRC-16 (CCITT and IBM polynomials)
- CRC-32 (ISO polynomial)
- CRC-64 (ECMA polynomial)
- Fletcher-16
- Fletcher-32
- SpamSum

Each of these algorithms have a corresponding license, that can be found in their corresponding file header.

The resulting output of `build.sh` falls under the LGPL 2.1 license as stated in the [LICENSE file](LICENSE).

Any new algorithm added should be under a license compatible with the LGPL 2.1 license to be accepted.

© 2021-2026 Natalia Portillo