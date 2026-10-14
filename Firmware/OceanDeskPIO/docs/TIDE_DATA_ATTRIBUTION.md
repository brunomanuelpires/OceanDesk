# Tide data attribution and commercial-use record

This file is the product-facing attribution record for the harmonic data
embedded in OceanDesk firmware. Keep it with the product documentation and
release notes whenever the corresponding tide model is distributed.

| Model | Embedded source and version | Licence / commercial position | Required attribution / action |
| --- | --- | --- | --- |
| Nazaré | FES2022b ocean-tide-extrapolated harmonics, bilinearly interpolated at the firmware coordinate from AVISO FES2022 files | AVISO Licence. OceanDesk ships derivative coefficients, not the original grids; the AVISO terms permit use/share of adapted works subject to attribution. | Include: “The FES2022 Tide product was funded by CNES, produced by LEGOS, NOVELTIS and CLS and made freely available by AVISO.” |
| Peniche | FES2022b ocean-tide-extrapolated harmonics, bilinearly interpolated at the firmware coordinate from AVISO FES2022 files | Same as Nazaré. | Same FES2022/AVISO attribution. |
| Cascais | TICON-4 record `cascais-209-prt-uhslc_fd`, distributed through the pinned Neaps tide database | CC BY 4.0; commercial use is allowed. | Retain the CC BY 4.0 attribution and licence link with the release documentation. |
| Independent validation | Portuguese Hydrographic Institute (IH), *Tabela de Marés 2026* | Reference used only to validate predicted events; not embedded in firmware. | Cite IH as the validation reference; do not imply IH endorsement. |

The historical Nazaré and Peniche TICON-4 inputs were CC BY-NC 4.0 and are no
longer used as harmonic sources. They may appear only as nearest-station
inventory metadata for the beach catalogue.

## Reproducibility and validation

- Reproducible derived coefficients: `tools/tides/generated/fes2022-portugal-harmonics.json`.
- Generator: `tools/tides/generate-fes2022-tide-data.mjs`.
- Validation maximum errors against IH 2026 events: Nazaré 6.86 minutes / 0.07 m;
  Peniche 1.38 minutes / 0.07 m; Cascais 6.34 minutes / 0.09 m.
- Original FES files belong under `DOWNLOADS/` outside Git. Never add raw AVISO
  grids or source downloads to a release commit.

Primary references: [AVISO FES2022 release](https://www.aviso.altimetry.fr/en/data/products/auxiliary-products/global-tide-fes/release-fes22.html),
[AVISO Licence](https://www.aviso.altimetry.fr/fileadmin/documents/data/License_Aviso.pdf),
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), and the
[IH 2026 tide table](https://loja.hidrografico.pt/ln/web/wp-content/uploads/2023/11/TabelaMare_I_2026_signed.pdf).
