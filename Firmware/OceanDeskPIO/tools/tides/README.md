# Portugal tide-station inventory

This development-only tool reads the pinned `@neaps/tide-database` package,
selects stations in mainland Portugal, the Azores and Madeira, and sorts them by
great-circle distance from Praia da Água de Madeiros.

The default reference point is `39.740084, -9.040124` (WGS84). It can be replaced
with `--lat` and `--lon` without changing the source.

## Usage

```sh
npm run tides:portugal
npm run tides:portugal -- --nearest 5
npm run tides:portugal -- --json
npm run tides:portugal -- --output tools/tides/generated/portugal-stations.json
npm run tides:generate-nazare
npm run tides:generate-beach-catalog
npm run test:tides
```

The table shows station name, coordinates, type, licence, constituent count and
distance. A star marks the nearest stations. The JSON output retains the harmonic
constituents and attribution needed by a later ESP32-specific compression step,
while dropping fields that are not needed for tide calculation or provenance.

The country field alone is not sufficient: the upstream data currently labels
historical records in Macau and Cape Verde as Portugal. Geographic bounds are
therefore applied after checking `country === "Portugal"`.

## Data and safety

Source: [Neaps Tide Database](https://github.com/openwatersio/tide-database).
Code is MIT licensed; each station carries its own data licence, which is retained
in generated output. Keep the attribution when distributing derived data.

These are astronomical tide harmonics. They do not account for weather, waves or
storm surge and must not be used for navigation or safety-critical decisions.

## Beach Catalog v1

`portugal-beach-catalog.json` is the editable source for the locations shown in
the ESP settings page. Each beach has its own stable id, display and short names,
region, country, WGS84 coordinates, geographic area and tide-model id. Tide
models separately identify the generated station record, firmware availability
and POSIX timezone.

`generate-beach-catalog.mjs` validates every mapping against the normalized
Portuguese station inventory. A v1 beach is accepted only when its model is
compiled into the firmware, the mapped station is the nearest inventory station
in the same geographic area, and it is no more than 45 km away. The generated
`src/locations/PortugalBeachCatalogData.inc` is checked by `npm test`.

The catalog types already distinguish mainland Portugal, the Azores and Madeira.
V1 intentionally contains no island beaches because those station models are not
yet compiled and validated in `TideService`.

## ESP32 development engine

`src/tides/data/NazareTideData.h/.cpp` are generated from the normalized JSON by
`npm run tides:generate-nazare`. The generator embeds all 50 Nazaré constituents,
resolves their Doodson coefficients through the pinned `@neaps/tide-predictor`
definitions, and keeps the original eight major constituents first to support a
host-side 8-versus-50 regression comparison. `npm test` also checks that the
generated C++ files are current.

Nazaré is strictly a development station:
its TICON-4/GESLA data is licensed **CC BY-NC 4.0**, which forbids commercial
use. It must be replaced by commercially distributable station data before any
commercial firmware or product distribution.

The engine returns the astronomical height anomaly in metres relative to mean
sea level (MSL). `TidePrediction` derives the current trend and the surrounding
high- and low-water events without changing `TideEngine`'s public API.
`TideService` converts those predictions into the application's tide snapshot
after NTP synchronization and refreshes it every 15 minutes. `HomeScreen` shows
the current trend, predicted height and the next high- and low-water times.

For the Nazaré development station, the displayed and event heights add the
Instituto Hidrografico's published mean level of 2.00 m above Zero Hidrografico
(ZH). This is a station-specific MSL-anomaly-to-ZH alignment, not a general
datum conversion.

Amplitude (`f`) and phase (`u`) nodal corrections reproduce the default IHO
fundamentals and compound-constituent composition used by the pinned
`@neaps/tide-predictor@0.11.0`. The generator resolves each Neaps constituent
into at most two fundamental correction terms, so the generated table remains
compact and the C++ correction calculation can be tested separately from the
harmonic sum. `predictHeight()` applies nodal corrections;
`predictHeightWithoutNodalCorrections()` retains the original calculation for
regression and effect measurements. Corrections are evaluated at the requested
UTC timestamp. Neaps evaluates them at the midpoint of each daily prediction
chunk, so long generated timelines can differ very slightly within a day even
though both use the same IHO formulas and composition.

General datum conversion (including MSL to LAT) and validation against
published predictions beyond the limited check below remain unimplemented.
Meteorological effects, waves and storm surge are also outside the harmonic
model. Predictions are not suitable for navigation or safety decisions.

The host regression test checks Neaps-derived `f`/`u` values and corrected
heights at fixed UTC timestamps, verifies continuity, and prints the 50-term
height with and without nodal corrections. These deltas measure the mathematical
effect of nodal correction; they are not an accuracy measurement against real
water levels. The historical eight-constituent baseline remains covered too.

Run its native sanity checks with:

```sh
npm run test:tide-engine
```

## External validation against Instituto Hidrografico

`npm run test:tide-engine-external` performs a reproducible, development-only
comparison against the official *Tabela de Mares 2026 - Volume I* published by
the Portuguese Instituto Hidrografico:

https://loja.hidrografico.pt/ln/web/wp-content/uploads/2023/11/TabelaMare_I_2026_signed.pdf

Retrieved 2026-09-03; SHA-256:
`e0c6512e2df1e940f982dadeb353c5dd6d70dfd262a4c79230c2f29a4051d589`.

Nazaré is not a primary port in that publication. The reference fixture is
therefore derived exactly as the publication instructs:

- Peniche predictions for 1-2 January 2026 come from page 2-43 (PDF page 65).
- Nazaré's concordance with Peniche comes from page 3-2 (PDF page 194).
- The height and time interpolation methods come from sections 111.1-111.4
  (PDF pages 18-20).
- All times are `Fuso 0 (TU)`, equivalent to UTC for these fixed timestamps.
- Published heights use Portugal's Zero Hidrografico (ZH). Peniche and Nazaré
  both list an adopted mean level of 2.00 m above ZH, so the test compares
  `OceanDesk MSL anomaly + 2.00 m` with the concordance-derived ZH height.

The upstream TICON-4/Neaps station metadata names `LAT` as its chart datum and
contains observed datum levels, but the generated firmware deliberately embeds
only harmonic amplitudes and phases. Their sum is a zero-mean astronomical
anomaly, not an absolute LAT height. Adding the independently published IH
Nazaré mean level aligns that anomaly with the table's ZH solely for this
comparison; it is not a general MSL/LAT/ZH conversion and is not fitted to the
validation results. The 50-constituent development dataset is the TICON-4
`nazaretg-naz-prt-cmems` record (observation epoch 2014-09-20 to 2025-06-30),
distributed through the pinned Neaps database under CC BY-NC 4.0.

The test keeps eight derived events (four high waters and four low waters) over
48 hours. It locates the nearby OceanDesk extremum on a one-minute grid and
refines it parabolically, but does not add extreme searching to `TideEngine`.
It reports signed mean error, maximum absolute error and RMSE for event time and
height. The primary-port heights in the IH table are truncated to 0.1 m, so this
is a useful independent regression check, not evidence of centimetric accuracy.
The Nazaré high-water time corrections also require linear interpolation; the
fixture retains the result to the nearest second.

Baseline result for the current engine (eight events):

| Quantity | Signed mean error | Maximum absolute error | RMSE |
| --- | ---: | ---: | ---: |
| Event time | +7.26 min | 9.49 min | 7.50 min |
| Height relative to ZH | -0.07 m | 0.11 m | 0.07 m |

Positive time error means OceanDesk is later; negative height error means it is
lower. The regression gates are deliberately visible: maximum timing error no
greater than 15 minutes, maximum height error no greater than 0.15 m, and height
RMSE no greater than 0.10 m. These gates detect material regressions but do not
erase or reinterpret the systematic offsets above.

This check does not compare continuous hourly curves, observations, weather,
waves or storm surge. The embedded Nazaré harmonics and this fixture remain
development-only; the harmonic source licence is CC BY-NC 4.0.
