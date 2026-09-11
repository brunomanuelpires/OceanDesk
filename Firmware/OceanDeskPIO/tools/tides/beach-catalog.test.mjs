import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import { validateBeachCatalog } from "./beach-catalog-lib.mjs";

const [catalog, stationDataset] = await Promise.all([
  readFile(new URL("./portugal-beach-catalog.json", import.meta.url), "utf8").then(JSON.parse),
  readFile(new URL("./generated/portugal-stations.json", import.meta.url), "utf8").then(JSON.parse),
]);

test("validates every v1 beach against an embedded, nearest tide model", () => {
  const validated = validateBeachCatalog(catalog, stationDataset);

  assert.equal(validated.beaches.length, 14);
  assert.deepEqual(validated.tideModels.map(({ id }) => id), ["nazare"]);
  assert.ok(validated.beaches.every(({ area }) => area === "mainland"));
  assert.ok(validated.beaches.every(({ tideModelId }) => tideModelId === "nazare"));
  assert.ok(validated.beaches.every(({ mappedDistanceKm }) => mappedDistanceKm <= 45));
});

test("keeps island areas valid without claiming unsupported beach mappings", () => {
  const validated = validateBeachCatalog(catalog, stationDataset);

  assert.ok(stationDataset.stations.some(({ region }) => region === "Madeira"));
  assert.ok(stationDataset.stations.some(({ region }) => region === "Azores"));
  assert.equal(validated.beaches.some(({ area }) => area === "madeira"), false);
  assert.equal(validated.beaches.some(({ area }) => area === "azores"), false);
});
