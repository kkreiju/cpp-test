#!/bin/bash
# ──────────────────────────────────────────────
# publish_to_aptly.sh - Push .deb package to local Aptly repository
# ──────────────────────────────────────────────
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION="1.0.1"
DEB_FILE="$PROJECT_DIR/nctv-player_${VERSION}_armhf.deb"
APTLY_REPO="nctv-releases"
APTLY_DISTRIBUTION="bullseye"

echo "═══════════════════════════════════════════"
echo "  NCTV Player - Aptly Publish"
echo "═══════════════════════════════════════════"

# Verify the .deb exists
if [ ! -f "$DEB_FILE" ]; then
    echo "ERROR: .deb file not found at $DEB_FILE"
    echo "Run build_pi_deb.sh first."
    exit 1
fi

echo "Package: $DEB_FILE"
echo "Repository: $APTLY_REPO"
echo "Distribution: $APTLY_DISTRIBUTION"
echo ""

# Create repo if it doesn't exist
if ! aptly repo show "$APTLY_REPO" > /dev/null 2>&1; then
    echo "Creating Aptly repository: $APTLY_REPO"
    aptly repo create -distribution="$APTLY_DISTRIBUTION" -component="main" "$APTLY_REPO"
fi

# Add package to repo
echo "Adding package to repository..."
aptly repo add "$APTLY_REPO" "$DEB_FILE"

# Publish (or update if already published)
if aptly publish show "$APTLY_DISTRIBUTION" > /dev/null 2>&1; then
    echo "Updating published repository..."
    aptly publish update "$APTLY_DISTRIBUTION"
else
    echo "Publishing repository for the first time..."
    aptly publish repo -distribution="$APTLY_DISTRIBUTION" "$APTLY_REPO"
fi

echo ""
echo "═══════════════════════════════════════════"
echo "  PUBLISH SUCCESSFUL"
echo "  Clients can update with: sudo apt update && sudo apt upgrade"
echo "═══════════════════════════════════════════"
