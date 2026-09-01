#!/usr/bin/env bash
set -e

KEYSTORE_DIR="android"
KEYSTORE_FILE="${KEYSTORE_DIR}/qanvas.keystore"
KEY_ALIAS="qanvas_key"
KEY_VALIDITY_DAYS=10000
IMAGE_TAG=qanvas-qt6-android:6.10-api36

echo "🔐 Generating new keystore for Android signing..."

if [ -f "$KEYSTORE_FILE" ]; then
    echo "⚠️  Keystore already exists at: $KEYSTORE_FILE"
    read -p "Do you want to overwrite it? (yes/no): " CONFIRM
    if [ "$CONFIRM" != "yes" ]; then
        echo "❌ Aborted."
        exit 1
    fi
    rm -f "$KEYSTORE_FILE"
fi

mkdir -p "$KEYSTORE_DIR"

while true; do
    read -s -p "Enter keystore password: " KEYSTORE_PASS
    echo ""
    if [ -z "$KEYSTORE_PASS" ]; then
        echo "❌ Password cannot be empty."
        continue
    fi
    read -s -p "Confirm keystore password: " KEYSTORE_PASS_CONFIRM
    echo ""
    if [ "$KEYSTORE_PASS" != "$KEYSTORE_PASS_CONFIRM" ]; then
        echo "❌ Passwords do not match. Try again."
        continue
    fi
    break
done

echo "📝 You will be prompted for keystore information..."
echo ""

docker build -t "$IMAGE_TAG" - < Dockerfile.android

docker run --rm -i \
    -e KEYSTORE_PASS="$KEYSTORE_PASS" \
    -v "${PWD}/${KEYSTORE_DIR}:/home/user/keystore" \
    "${IMAGE_TAG}" \
    sh -c 'keytool -genkeypair \
        -v \
        -keystore /home/user/keystore/qanvas.keystore \
        -alias '"${KEY_ALIAS}"' \
        -keyalg RSA \
        -keysize 2048 \
        -validity '"${KEY_VALIDITY_DAYS}"' \
        -storepass "$KEYSTORE_PASS" \
        -keypass "$KEYSTORE_PASS" \
        -dname "CN=Qanvas, OU=Development, O=Qanvas, L=Warsaw, ST=Masovian, C=PL"'

echo ""
echo "✅ Keystore generated successfully!"
echo "📁 Location: $KEYSTORE_FILE"
echo "🔑 Alias: $KEY_ALIAS"
echo ""
echo "⚠️  Keep this keystore file secure and never commit it to version control!"
