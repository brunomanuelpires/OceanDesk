import { haversineDistanceKm, PORTUGAL_AREAS } from "./portugal-stations-lib.mjs";

const VALID_AREAS = new Set(PORTUGAL_AREAS.map(({ name }) => name));
const POSIX_TIMEZONES = Object.freeze({
  mainland: "WET0WEST,M3.5.0/1,M10.5.0",
  azores: "AZOT1AZOST0,M3.5.0/0,M10.5.0/1",
  madeira: "WET0WEST,M3.5.0/1,M10.5.0",
});

function requireText(value, field) {
  if (typeof value !== "string" || value.trim() === "") throw new Error(`${field} must be non-empty`);
}

function requireCoordinates(item, label) {
  if (!Number.isFinite(item.latitude) || item.latitude < -90 || item.latitude > 90 ||
      !Number.isFinite(item.longitude) || item.longitude < -180 || item.longitude > 180) {
    throw new Error(`${label} has invalid coordinates`);
  }
}

function requireUniqueIds(items, label) {
  const ids = new Set();
  for (const item of items) {
    requireText(item.id, `${label}.id`);
    if (ids.has(item.id)) throw new Error(`Duplicate ${label} id: ${item.id}`);
    ids.add(item.id);
  }
}

export function validateBeachCatalog(catalog, stationDataset) {
  if (catalog?.schemaVersion !== 1) throw new Error("Unsupported beach catalog schema");
  if (!Array.isArray(catalog.tideModels) || catalog.tideModels.length === 0) {
    throw new Error("Beach catalog must contain tide models");
  }
  if (!Array.isArray(catalog.beaches) || catalog.beaches.length === 0) {
    throw new Error("Beach catalog must contain beaches");
  }
  if (!Number.isFinite(catalog.maximumStationDistanceKm) || catalog.maximumStationDistanceKm <= 0) {
    throw new Error("maximumStationDistanceKm must be positive");
  }

  requireUniqueIds(catalog.tideModels, "tide model");
  requireUniqueIds(catalog.beaches, "beach");
  const stationsById = new Map(stationDataset.stations.map((station) => [station.id, station]));
  const modelsById = new Map();

  const tideModels = catalog.tideModels.map((model) => {
    requireText(model.stationId, `${model.id}.stationId`);
    requireText(model.displayName, `${model.id}.displayName`);
    if (!VALID_AREAS.has(model.area)) throw new Error(`${model.id} has invalid area ${model.area}`);
    if (model.firmwareSupported !== true) {
      throw new Error(`${model.id} is catalogued but not supported by the firmware`);
    }
    const station = stationsById.get(model.stationId);
    if (!station) throw new Error(`${model.id} references unknown station ${model.stationId}`);
    requireCoordinates(station, `station ${model.stationId}`);
    if (model.timezone !== POSIX_TIMEZONES[model.area]) {
      throw new Error(`${model.id} has a timezone that does not match ${model.area}`);
    }
    const normalized = { ...model, latitude: station.latitude, longitude: station.longitude };
    modelsById.set(model.id, normalized);
    return normalized;
  });

  const beaches = catalog.beaches.map((beach) => {
    for (const field of ["displayName", "region", "country", "area", "tideModelId"]) {
      requireText(beach[field], `${beach.id}.${field}`);
    }
    requireCoordinates(beach, `beach ${beach.id}`);
    if (beach.country !== "Portugal" || !VALID_AREAS.has(beach.area)) {
      throw new Error(`${beach.id} must identify a supported Portuguese area`);
    }
    const model = modelsById.get(beach.tideModelId);
    if (!model) throw new Error(`${beach.id} references unsupported model ${beach.tideModelId}`);
    if (model.area !== beach.area) throw new Error(`${beach.id} and ${model.id} are in different areas`);

    const stationsInArea = stationDataset.stations.filter((station) => {
      const bounds = PORTUGAL_AREAS.find(({ name }) => name === beach.area);
      return station.latitude >= bounds.minLat && station.latitude <= bounds.maxLat &&
             station.longitude >= bounds.minLon && station.longitude <= bounds.maxLon;
    });
    const nearest = stationsInArea
      .map((station) => ({ station, distanceKm: haversineDistanceKm(beach, station) }))
      .sort((left, right) => left.distanceKm - right.distanceKm)[0];
    const mappedDistanceKm = haversineDistanceKm(beach, model);
    if (nearest?.station.id !== model.stationId) {
      throw new Error(`${beach.id} maps to ${model.stationId}, but ${nearest?.station.id} is nearer`);
    }
    if (mappedDistanceKm > catalog.maximumStationDistanceKm) {
      throw new Error(`${beach.id} is ${mappedDistanceKm.toFixed(1)} km from ${model.displayName}`);
    }

    return { ...beach, shortName: beach.shortName ?? null, timezone: model.timezone, mappedDistanceKm };
  });

  return { tideModels, beaches };
}

export const catalogAreaCppNames = Object.freeze({
  mainland: "GeographicArea::Mainland",
  azores: "GeographicArea::Azores",
  madeira: "GeographicArea::Madeira",
});
