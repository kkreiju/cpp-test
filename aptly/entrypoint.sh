#!/bin/bash
set -e

# The folder mapped to your Windows machine
BACKUP_DIR="/app_keys"

echo ">>> Starting Entropy Generator..."
rngd -r /dev/urandom

# ==============================================================================
#  STEP 1: KEY MANAGEMENT (Restore or Generate)
# ==============================================================================
echo ">>> Checking GPG Keys..."

# Check if we have a backup in the Windows folder
if [ -f "$BACKUP_DIR/private.key" ]; then
    echo ">>> ♻️  Found existing keys in 'exported_keys'. Importing..."
    gpg --import "$BACKUP_DIR/private.key"
else
    echo ">>> 🆕 No backup found. Generating NEW GPG Key..."
    # Generate new key
    gpg --batch --passphrase '' --quick-gen-key "NTV360 <admin@ntv360.com>" default default 0
fi

# Extract the Key ID
KEY_ID=$(gpg --list-keys --keyid-format SHORT 2>/dev/null | grep "^pub" | head -n 1 | awk -F'/' '{print $2}' | awk '{print $1}' | xargs)
echo ">>> Using Key ID: $KEY_ID"

if [ -z "$KEY_ID" ]; then
    echo "ERROR: Could not extract Key ID. Exiting."
    exit 1
fi

# ==============================================================================
#  STEP 2: BACKUP KEYS TO WINDOWS
# ==============================================================================
echo ">>> 💾 Backing up keys to Windows folder..."
# We export the SECRET key so we can restore it later (Persistence!)
gpg --export-secret-keys --armor "$KEY_ID" > "$BACKUP_DIR/private.key"
# We export the PUBLIC key for clients
gpg --export --armor "$KEY_ID" > "$BACKUP_DIR/public.key"

# ==============================================================================
#  STEP 3: REPO MANAGEMENT
# ==============================================================================
echo ">>> Initializing Repo..."
rm -rf ~/.aptly

# Create Repo (Supporting both 32-bit and 64-bit ARM)
aptly repo create -architectures="armhf,arm64" -comment="NCTV Player Repo" -component=main -distribution=stable nctv-repo

# Add packages
aptly repo add nctv-repo /packages/

# Publish (Include both architectures)
aptly publish repo -architectures="armhf,arm64" -batch -gpg-key="$KEY_ID" -passphrase="" nctv-repo

# Copy public key to web root for 'wget' access
cp "$BACKUP_DIR/public.key" ~/.aptly/public/public.key

echo "=================================================="
echo "   ✅ REPO DEPLOYED SUCCESSFULLY"
echo "   🔑 Key ID: $KEY_ID"
echo "   📂 Keys saved to: ./exported_keys/"
echo "   🌐 URL: http://localhost:8080"
echo "=================================================="

aptly serve -listen=:8080