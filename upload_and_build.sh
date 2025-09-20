#!/bin/bash

echo "Building and uploading ROVR project..."
echo "======================================="

# Navigate to project directory
cd "$(dirname "$0")"

echo "1. Building the project..."
pio run

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    
    echo "2. Uploading filesystem (LittleFS)..."
    pio run --target uploadfs
    
    if [ $? -eq 0 ]; then
        echo "✅ Filesystem upload successful!"
        
        echo "3. Uploading firmware..."
        pio run --target upload
        
        if [ $? -eq 0 ]; then
            echo "✅ Firmware upload successful!"
            echo ""
            echo "🎉 ROVR is ready!"
            echo "Connect to WiFi network: ROVR1"
            echo "Password: redroverredrover"
            echo "Open browser to: http://192.168.4.1"
        else
            echo "❌ Firmware upload failed!"
        fi
    else
        echo "❌ Filesystem upload failed!"
    fi
else
    echo "❌ Build failed!"
fi
