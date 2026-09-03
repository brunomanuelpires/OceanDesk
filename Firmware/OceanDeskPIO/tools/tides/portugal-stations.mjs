#!/usr/bin/env node

import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { stations } from "@neaps/tide-database";
import {
  AGUA_DE_MADEIROS,
  createPortugalInventory,
  prepareEsp32Dataset,
} from "./portugal-stations-lib.mjs";

function usage() {
  console.log(`Usage: npm run tides:portugal -- [options]

Options:
  --lat <number>       Reference latitude (default: ${AGUA_DE_MADEIROS.latitude})
  --lon <number>       Reference longitude (default: ${AGUA_DE_MADEIROS.longitude})
  --nearest <number>   Number of nearby stations to highlight (default: 3)
  --output <path>      Write the normalized ESP32-ready JSON dataset
  --json               Print the normalized dataset instead of a table
  --help               Show this help
`);
}

function parseArguments(args) {
  const options = {
    latitude: AGUA_DE_MADEIROS.latitude,
    longitude: AGUA_DE_MADEIROS.longitude,
    nearest: 3,
    output: null,
    json: false,
  };

  for (let index = 0; index < args.length; index += 1) {
    const argument = args[index];
    if (argument === "--help") options.help = true;
    else if (argument === "--json") options.json = true;
    else if (["--lat", "--lon", "--nearest", "--output"].includes(argument)) {
      const value = args[++index];
      if (value === undefined) throw new Error(`Missing value for ${argument}`);
      if (argument === "--output") options.output = value;
      else if (argument === "--lat") options.latitude = Number(value);
      else if (argument === "--lon") options.longitude = Number(value);
      else options.nearest = Number(value);
    } else {
      throw new Error(`Unknown option: ${argument}`);
    }
  }

  if (!Number.isFinite(options.latitude) || options.latitude < -90 || options.latitude > 90) {
    throw new Error("--lat must be a number between -90 and 90");
  }
  if (!Number.isFinite(options.longitude) || options.longitude < -180 || options.longitude > 180) {
    throw new Error("--lon must be a number between -180 and 180");
  }
  if (!Number.isInteger(options.nearest) || options.nearest < 1) {
    throw new Error("--nearest must be a positive integer");
  }

  return options;
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (options.help) {
    usage();
    return;
  }

  const usesDefaultOrigin =
    options.latitude === AGUA_DE_MADEIROS.latitude &&
    options.longitude === AGUA_DE_MADEIROS.longitude;
  const origin = {
    name: usesDefaultOrigin ? AGUA_DE_MADEIROS.name : "Reference location",
    latitude: options.latitude,
    longitude: options.longitude,
  };
  const inventory = createPortugalInventory(stations, origin);
  const dataset = prepareEsp32Dataset(inventory, origin, options.nearest);

  if (options.json) {
    console.log(JSON.stringify(dataset, null, 2));
  } else {
    console.log(
      `Portugal: ${inventory.length} stations; distances from ${origin.latitude}, ${origin.longitude}\n`,
    );
    console.table(
      inventory.map((station, index) => ({
        near: index < options.nearest ? "★" : "",
        name: station.name,
        coordinates: `${station.latitude.toFixed(6)}, ${station.longitude.toFixed(6)}`,
        type: station.type,
        license: station.license ?? "unknown",
        constituents: station.constituentCount,
        distanceKm: station.distanceKm.toFixed(1),
      })),
    );
  }

  if (options.output) {
    const outputPath = resolve(options.output);
    await mkdir(dirname(outputPath), { recursive: true });
    await writeFile(outputPath, `${JSON.stringify(dataset, null, 2)}\n`, "utf8");
    console.error(`Wrote ${inventory.length} stations to ${outputPath}`);
  }
}

main().catch((error) => {
  console.error(`tides:portugal: ${error.message}`);
  process.exitCode = 1;
});
