glib = findPackage("glib-2.0", REQUIRED)
openGL = findPackage("gl", REQUIRED)
x11 = findPackage("x11", REQUIRED)
nix = findPackage("WebKitNix", REQUIRED)

addSubdirectory("Browser")
-- addSubdirectory("ContentsInjectedBundle")
addSubdirectory("UIInjectedBundle")
