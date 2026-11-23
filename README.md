# dlopen() slows down with every library already loaded

Each `dlopen` that loads an image takes time proportional to the number of images
already loaded from outside the dyld shared cache, so loading N libraries one call
at a time takes O(N²).

## Reproduce

    ./run.sh 500

This builds 500 one-function dylibs, then times `dlopen` on each of them, and finally
loads the same 500 with a single `dlopen` of a dylib that links them all.

## Result (macOS 27.0 26A428, arm64, M4 Pro)

          call     ms per dlopen
       1..50               0.489
      51..100              0.742
     101..150              0.891
     151..200              1.254
     201..250              1.570
     251..300              2.069
     301..350              2.384
     351..400              2.675
     401..450              3.127
     451..500              4.731
    500 dlopen calls loading 500 images: 996.6 ms
    1 dlopen call loading 501 images: 234.8 ms

The first calls take about 0.5 ms and the last about 4.7 ms for identical libraries.
Loading the same images in one call costs 0.47 ms per image, so the growth is per
`dlopen` call, not per image mapped.

Doubling the count roughly quadruples the total (`./run.sh 1000`):

    1000 dlopen calls loading 1000 images: 3840.0 ms   (last 100 calls: 8.1 ms each)
    1 dlopen call loading 1001 images: 306.2 ms

## Cause

`ExternallyViewableState::addImages` (dyld/ExternallyViewableState.cpp) calls
`generateAtlas` on every load. `generateAtlas` walks all loaded images and, for each
one that is not in the shared cache, rebuilds a property-list dictionary (path,
install name, UUID, one dictionary per segment) in `atlasAddImage`, then encodes the
whole list and swaps it in. A time profile of the loop puts about three quarters of
`dlopen` in `generateAtlas`.

Attaching `sample` or Instruments makes this worse again, because dyld then also
sends a synchronous notification to the monitor on every load.

## Expected

The cost of a `dlopen` should not depend on how many other libraries are loaded,
for example by appending the new images to the existing snapshot.

## Impact

Processes that open hundreds of libraries individually: language runtimes with one
shared library per precompiled package (Julia), Python with many extension modules,
plugin hosts. In Julia, loading a plotting package with 250 package images spent
about 700 ms of 1.7 s in `dlopen`.
