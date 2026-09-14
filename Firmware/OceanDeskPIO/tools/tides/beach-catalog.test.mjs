import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import { validateBeachCatalog } from "./beach-catalog-lib.mjs";

const [catalog, stationDataset] = await Promise.all([
  readFile(new URL("./portugal-beach-catalog.json", import.meta.url), "utf8").then(JSON.parse),
  readFile(new URL("./generated/portugal-stations.json", import.meta.url), "utf8").then(JSON.parse),
]);

test("validates every catalog beach against an embedded, nearest tide model", () => {
  const validated = validateBeachCatalog(catalog, stationDataset);

  assert.equal(validated.beaches.length, 22);
  assert.deepEqual(validated.tideModels.map(({ id }) => id), ["nazare", "peniche"]);
  assert.ok(validated.beaches.every(({ area }) => area === "mainland"));
  assert.ok(validated.beaches.every(({ mappedDistanceKm }) => mappedDistanceKm <= 45));
});

test("unlocks the reviewed Peniche beaches after model validation", () => {
  const validated = validateBeachCatalog(catalog, stationDataset);
  const peniche = validated.tideModels.find(({ id }) => id === "peniche");
  const penicheBeaches = validated.beaches.filter(({ tideModelId }) => tideModelId === "peniche");

  assert.equal(peniche?.stationId, "ticon/penichetg-pen-prt-cmems");
  assert.equal(peniche?.firmwareSupported, true);
  assert.deepEqual(
    penicheBeaches.map(({ id }) => id),
    [
      "peniche-de-cima",
      "baleal-sul",
      "baleal-norte",
      "gamboa",
      "cova-da-alfarroba",
      "molhe-leste",
      "medao-supertubos",
      "consolacao",
    ],
  );
  assert.ok(penicheBeaches.every(({ region }) => region === "Peniche"));
  assert.ok(penicheBeaches.every(({ mappedDistanceKm }) => mappedDistanceKm <= 3.4));
  assert.equal(validated.beaches.filter(({ tideModelId }) => tideModelId === "nazare").length, 14);
});

test("keeps island areas valid without claiming unsupported beach mappings", () => {
  const validated = validateBeachCatalog(catalog, stationDataset);

  assert.ok(stationDataset.stations.some(({ region }) => region === "Madeira"));
  assert.ok(stationDataset.stations.some(({ region }) => region === "Azores"));
  assert.equal(validated.beaches.some(({ area }) => area === "madeira"), false);
  assert.equal(validated.beaches.some(({ area }) => area === "azores"), false);
});
