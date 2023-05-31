#!/usr/bin/env bash

set -ex

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
source "$SCRIPT_DIR"/../lib.sh

function main {
  local -r version=$1 variant=$2
  if ! image_exists datadog/dd-appsec-php-ci:php-$version-$variant; then
    "$SCRIPT_DIR"/../php/build_image.sh $version $variant
  fi

  major=$(cut -d '.' -f 1 <<< "$version")
  minor=$(cut -d '.' -f 2 <<< "$version")

  DOCKER_BUILDKIT=0 docker build -t dd-appsec-php-trace:$version \
    --build-arg PHP_MAJOR_VERSION=$major PHP_MINOR_VERSION=$minor \
    -f "$SCRIPT_DIR/Dockerfile" "$SCRIPT_DIR/../../../../dd-trace-php/"
}

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <php_version_no_minor> <variant>" >&2
  exit 1
fi

main "$@"
