const EARTH_RADIUS_KM = 6371.0088;

export const AGUA_DE_MADEIROS = Object.freeze({
  name: "Praia da Água de Madeiros",
  latitude: 39.740084,
  longitude: -9.040124,
});

// Geographic guardrails avoid historical records whose country field is Portugal
// but whose coordinates are in Macau or Cape Verde.
export const PORTUGAL_AREAS = Object.freeze([
  { name: "mainland", minLat: 36.8, maxLat: 42.2, minLon: -9.6, maxLon: -6.0 },
  { name: "azores", minLat: 36.8, maxLat: 40.1, minLon: -31.5, maxLon: -24.5 },
  { name: "madeira", minLat: 32.4, maxLat: 33.2, minLon: -17.5, maxLon: -15.5 },
]);

function isFiniteCoordinate(value) {
  return typeof value === "number" && Number.isFinite(value);
}

export function haversineDistanceKm(from, to) {
  if (![from.latitude, from.longitude, to.latitude, to.longitude].every(isFiniteCoordinate)) {
    throw new TypeError("Latitude and longitude must be finite numbers");
  }

  const radians = (degrees) => (degrees * Math.PI) / 180;
  const latitudeDelta = radians(to.latitude - from.latitude);
  const longitudeDelta = radians(to.longitude - from.longitude);
  const fromLatitude = radians(from.latitude);
  const toLatitude = radians(to.latitude);

  const a =
    Math.sin(latitudeDelta / 2) ** 2 +
    Math.cos(fromLatitude) * Math.cos(toLatitude) * Math.sin(longitudeDelta / 2) ** 2;

  return EARTH_RADIUS_KM * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}

export function isPortugalStation(station) {
  if (
    station?.country !== "Portugal" ||
    !isFiniteCoordinate(station.latitude) ||
    !isFiniteCoordinate(station.longitude)
  ) {
    return false;
  }

  return PORTUGAL_AREAS.some(
    (area) =>
      station.latitude >= area.minLat &&
      station.latitude <= area.maxLat &&
      station.longitude >= area.minLon &&
      station.longitude <= area.maxLon,
  );
}

export function createPortugalInventory(stations, origin = AGUA_DE_MADEIROS) {
  return stations
    .filter(isPortugalStation)
    .map((station) => ({
      id: station.id,
      name: station.name,
      region: station.region ?? null,
      latitude: station.latitude,
      longitude: station.longitude,
      type: station.type,
      license: station.license?.type ?? null,
      constituentCount: station.harmonic_constituents?.length ?? 0,
      distanceKm: haversineDistanceKm(origin, station),
      station,
    }))
    .sort((left, right) => left.distanceKm - right.distanceKm || left.name.localeCompare(right.name));
}

export function prepareEsp32Dataset(inventory, origin = AGUA_DE_MADEIROS, nearestCount = 3) {
  return {
    schemaVersion: 1,
    source: {
      name: "Neaps Tide Database",
      package: "@neaps/tide-database",
      attribution: "Tide harmonic constituents from the Neaps tide database",
      url: "https://github.com/openwatersio/tide-database",
    },
    origin,
    stationCount: inventory.length,
    nearestStationIds: inventory.slice(0, nearestCount).map(({ id }) => id),
    stations: inventory.map(({ station, distanceKm }) => ({
      id: station.id,
      name: station.name,
      region: station.region ?? null,
      latitude: station.latitude,
      longitude: station.longitude,
      timezone: station.timezone ?? null,
      type: station.type,
      distanceKm: Number(distanceKm.toFixed(3)),
      chartDatum: station.chart_datum ?? null,
      license: station.license
        ? {
            type: station.license.type,
            url: station.license.url ?? null,
            commercialUse: station.license.commercial_use ?? null,
          }
        : null,
      source: station.source
        ? {
            name: station.source.name ?? null,
            id: station.source.id ?? null,
            url: station.source.url ?? null,
          }
        : null,
      harmonicConstituents: (station.harmonic_constituents ?? []).map(
        ({ name, amplitude, phase }) => ({ name, amplitude, phase }),
      ),
    })),
  };
}
