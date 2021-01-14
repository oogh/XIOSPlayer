echo "[CLEAN] Begin!"
# ios example clean
PROJ_IOS=./examples/iOS

if [ -f "$PROJ_IOS/Podfile.lock" ]; then
    rm $PROJ_IOS/Podfile.lock
    echo "[CLEANing] $PROJ_IOS/Podfile.lock"
fi

if [ -d "$PROJ_IOS/Pods" ]; then
    rm -rf $PROJ_IOS/Pods
    echo "[CLEANing] $PROJ_IOS/Pods"
fi

if [ -d "$PROJ_IOS/XRender.xcworkspace" ]; then
    rm -rf $PROJ_IOS/XRender.xcworkspace
    echo "[CLEANing] $PROJ_IOS/XRender.xcworkspace"
fi

# mac example clean
PROJ_Mac=./examples/Mac

if [ -d "$PROJ_Mac/camke-build-xcode" ]; then
    rm $PROJ_Mac/camke-build-xcode
    echo "[CLEANing] $PROJ_Mac/camke-build-xcode"
fi

if [ -d "$PROJ_Mac/camke-build-debug" ]; then
    rm $PROJ_Mac/camke-build-debug
    echo "[CLEANing] $PROJ_Mac/camke-build-debug"
fi

echo "[CLEAN] Done!"
