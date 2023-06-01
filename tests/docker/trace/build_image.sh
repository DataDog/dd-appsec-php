#!/usr/bin/env bash

set -ex

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
source "$SCRIPT_DIR"/../lib.sh

function main {
  local -r version=$1 variant=$2 arch=$3
  docker_file="$SCRIPT_DIR/Dockerfile"
  tag="dd-appsec-php-trace:$version"
  if ! image_exists datadog/dd-appsec-php-ci:php-$version-$variant; then
    "$SCRIPT_DIR"/../php/build_image.sh $version $variant
  fi

  major=$(cut -d '.' -f 1 <<< "$version")
  minor=$(cut -d '.' -f 2 <<< "$version")

  if [ "$arch" = "alpine" ]; then
    tag="$tag-alpine"
    docker_file="$SCRIPT_DIR/Dockerfile.alpine"
  fi

  DOCKER_BUILDKIT=0 docker build -t $tag \
    --build-arg PHP_MAJOR_VERSION=$major --build-arg PHP_MINOR_VERSION=$minor \
    -f "$docker_file" "$SCRIPT_DIR/../../../../dd-trace-php/"
}

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <php_version_no_minor> <variant> <arch>" >&2
  exit 1
fi

main "$@"
