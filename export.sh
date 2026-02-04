#!/bin/bash
# Run from firmwork repo root (where lib/ is)
# Requires: jq
# Prerequisites: pio account login (once globally)
# This script:
# - Sets clean name (folder name, with - allowed)
# - Adds/corrects dependencies (full owner/name format)
# - Bumps version to 1.0.1 (change if needed)
# - Publishes with --owner drewmutt
# Safe: backs up original library.json as library.json.bak

set -e

owner="drewmutt"

libs=(
  "Firmwork-Common"
  "Firmwork-Graphics-Core"
  "Firmwork-Graphics-GrayU8G2"
  "Firmwork-Graphics-M5"
  "Firmwork-Mesh"
  "Firmwork-Motion"
)

for lib in "${libs[@]}"; do
  path="lib/$lib"
  if [ ! -d "$path" ]; then
    echo "Skipping missing: $path"
    continue
  fi

  cd "$path"

  cp library.json library.json.bak

  # Set name (just the lib folder name, no owner)
  jq --arg name "$lib" '
    .name = $name |
    .version = "1.0.1"
  ' library.json > tmp.json && mv tmp.json library.json

  # Add/fix dependencies (full owner/name)
  case "$lib" in
    "Firmwork-Motion")
      jq '.dependencies = [{"name": "waspinator/AccelStepper", "version": "*"}]' library.json > tmp.json && mv tmp.json library.json
      ;;
    "Firmwork-Graphics-GrayU8G2")
      jq '.dependencies = [{"name": "olikraus/U8g2", "version": "*"}]' library.json > tmp.json && mv tmp.json library.json
      ;;
    "Firmwork-Graphics-M5")
      jq '.dependencies = [
        {"name": "m5stack/M5GFX", "version": "*"},
        {"name": "m5stack/M5Unified", "version": "*"}
      ]' library.json > tmp.json && mv tmp.json library.json
      ;;
    *)
      jq 'if .dependencies == null then .dependencies = [] else . end' library.json > tmp.json && mv tmp.json library.json
      ;;
  esac

  echo "Publishing $lib as $owner/$lib ..."
  pio pkg publish --owner "$owner"

  cd ../..
done

echo "All done. Original library.json saved as .bak in each folder."