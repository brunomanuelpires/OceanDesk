import assert from "node:assert/strict";
import test from "node:test";
import { stations } from "@neaps/tide-database";
import {
  AGUA_DE_MADEIROS,
  createPortugalInventory,
  haversineDistanceKm,
  isPortugalStation,
  prepareEsp32Dataset,
} from "./portugal-stations-lib.mjs";

test("calculates great-circle distances in kilometres", () => {
  assert.equal(haversineDistanceKm(AGUA_DE_MADEIROS, AGUA_DE_MADEIROS), 0);
  const oneDegreeNorth = haversineDistanceKm(
    { latitude: 0, longitude: 0 },
    { latitude: 1, longitude: 0 },
  );
  assert.ok(Math.abs(oneDegreeNorth - 111.195) < 0.001);
});

test("keeps mainland, Azores and Madeira while rejecting historical false positives", () => {
  assert.equal(isPortugalStation({ country: "Portugal", latitude: 39.5, longitude: -9.1 }), true);
  assert.equal(isPortugalStation({ country: "Portugal", latitude: 38.6, longitude: -28.6 }), true);
  assert.equal(isPortugalStation({ country: "Portugal", latitude: 32.6, longitude: -16.9 }), true);
  assert.equal(isPortugalStation({ country: "Portugal", latitude: 22.167, longitude: 113.55 }), false);
  assert.equal(isPortugalStation({ country: "Portugal", latitude: 16.867, longitude: -24.983 }), false);
  assert.equal(isPortugalStation({ country: "Spain", latitude: 39.5, longitude: -9.1 }), false);
});

test("sorts stations by distance and preserves export fields", () => {
  const sample = [
    {
      id: "far",
      name: "Far",
      country: "Portugal",
      latitude: 40,
      longitude: -8,
      type: "reference",
      license: { type: "cc-by-4.0", commercial_use: true, url: "https://example.test/license" },
      harmonic_constituents: [{ name: "M2", amplitude: 1, phase: 2 }],
    },
    {
      id: "near",
      name: "Near",
      country: "Portugal",
      latitude: 39.74,
      longitude: -9.04,
      type: "reference",
      harmonic_constituents: [],
    },
  ];
  const inventory = createPortugalInventory(sample);
  const dataset = prepareEsp32Dataset(inventory);

  assert.deepEqual(inventory.map(({ id }) => id), ["near", "far"]);
  assert.equal(dataset.stationCount, 2);
  assert.equal(dataset.stations[1].license.type, "cc-by-4.0");
  assert.deepEqual(dataset.stations[1].harmonicConstituents, [
    { name: "M2", amplitude: 1, phase: 2 },
  ]);
});

test("finds the expected Portuguese records in the pinned Neaps database", () => {
  const inventory = createPortugalInventory(stations);

  assert.ok(inventory.length >= 10);
  assert.equal(inventory[0].name, "Nazare");
  assert.ok(inventory.every(({ station }) => !["Macau", "Porto Grande"].includes(station.name)));
  assert.ok(inventory.every(({ constituentCount }) => constituentCount > 0));
});
