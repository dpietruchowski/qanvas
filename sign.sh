#!/usr/bin/env bash
set -e

echo "📦 Signing APK using apksigner..."

# Extract version from CMakeLists.txt
VERSION=$(grep "set(QT_ANDROID_VERSION_NAME" CMakeLists.txt | sed -n 's/.*"\(.*\)".*/\1/p')
if [ -z "$VERSION" ]; then
    echo "❌ Failed to extract version from CMakeLists.txt"
    exit 1
fi

echo "📌 Version: $VERSION"

BUILD_DIR="build-android"
IMAGE_TAG=qanvas-qt6-android:6.10-api36
KEYSTORE="/home/user/project/android/qanvas.keystore"
KEY_ALIAS="qanvas_key"
UNSIGNED_APK="/home/user/build/src/android-build/build/outputs/apk/release/android-build-release-unsigned.apk"

APKSIGNER_PATH="/opt/android-sdk/build-tools/36.0.0/apksigner"

read -s -p "Enter keystore password: " KEYSTORE_PASS
echo ""
if [ -z "$KEYSTORE_PASS" ]; then
    echo "❌ Password cannot be empty."
    exit 1
fi

docker build -t "$IMAGE_TAG" - < Dockerfile.android

# Sign the APK
echo "🔑 Signing APK..."
docker run --rm \
    -e KEYSTORE_PASS="$KEYSTORE_PASS" \
    -v "${PWD}:/home/user/project:ro" \
    -v "${PWD}/${BUILD_DIR}:/home/user/build" \
    "${IMAGE_TAG}" \
    sh -c 'cd /home/user/build && \
           '"${APKSIGNER_PATH}"' sign \
               --ks '"${KEYSTORE}"' \
               --ks-key-alias '"${KEY_ALIAS}"' \
               --ks-pass env:KEYSTORE_PASS \
               --out /home/user/build/qanvas-'"${VERSION}"'-release-signed.apk \
               '"${UNSIGNED_APK}"

echo "✅ APK signed: ${BUILD_DIR}/qanvas-${VERSION}-release-signed.apk"

# Verify the signature
echo "🔍 Verifying signature..."
if docker run --rm \
    -v "${PWD}:/home/user/project:ro" \
    -v "${PWD}/${BUILD_DIR}:/home/user/build" \
    "${IMAGE_TAG}" \
    sh -c "${APKSIGNER_PATH} verify --verbose /home/user/build/qanvas-${VERSION}-release-signed.apk"; then
    echo '✅ Signature verification succeeded!'
else
    echo '❌ Signature verification FAILED!'
    exit 1
fi
