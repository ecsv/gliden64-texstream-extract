.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Sven Eckelmann <sven@narfation.org>

======================
gliden64_texstream_extract
======================

gliden64_texstream_extract is a debugging tool used to extract the content of a
GLideN64 texture stream cache and to show extra information about the content.

USAGE
=====

The texture stream cache has to be given to gliden64_texstream_extract via the
``--input`` argument. The compressed files are usually named ``*.hts``. The
result is a v7 tarball written to stdout.

The output file can also be specified using ``--output``.

Extra information about the content and errors are printed on stdout.::

  $ gliden64_texstream_extract --input MUPEN64PLUS.hts -vv --bitmapv5 \
    --prefix MUPEN64PLUS | tar x
  $ for i in *.bmp; do convert -strip -define png:format=png32 \
    -define png:compression-level=9  "${i}" "${i%.bmp}.png"; done
  $ rm *.bmp

The output files don't follow the Rice hires texture naming scheme correctly.
But they should be compatible with Glide64/GLideN64.

More information about the parameters can be requested using::

  $ gliden64_texstream_extract --help

CONTRIBUTING
============

Patches can be sent to the author. Please follow the Linux CodingStyle. A quick
check can be done using::

  $ ./scripts/checkpatch.pl --strict --ignore LONG_LINE,NEW_TYPEDEFS,CAMELCASE \
    -q $PATCH
  $ make CC=cgcc
  $ cppcheck --enable=all .
