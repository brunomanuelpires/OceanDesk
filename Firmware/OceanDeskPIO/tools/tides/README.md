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

## ESP32 development engine

`src/tides/data/NazareTideData.cpp` embeds eight major Nazaré constituents as a
small first dataset for `TideEngine`. Nazaré is strictly a development station:
its TICON-4/GESLA data is licensed **CC BY-NC 4.0**, which forbids commercial
use. It must be replaced by commercially distributable station data before any
commercial firmware or product distribution.

The first engine version returns the astronomical height anomaly in metres
relative to mean sea level (MSL). It is intentionally not connected to
`TideService` or `HomeScreen` yet. It currently omits nodal amplitude/phase
corrections, minor and compound constituents, datum conversion, and extreme
searching. Predictions are not suitable for navigation or safety decisions.

Run its native sanity checks with:

```sh
npm run test:tide-engine
```
