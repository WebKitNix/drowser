
uiBundle = Library:new("UiBundle")
uiBundle:usePackage(nix)
uiBundle:addIncludePath("../Shared")
uiBundle:addFiles([[
    Bundle.cpp
    ../Shared/WKConversions.cpp
]])
