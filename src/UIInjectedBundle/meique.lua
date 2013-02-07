
uiBundle = Library:new("UiBundle")
uiBundle:usePackage(nix)
uiBundle:addIncludePath("../Shared")
uiBundle:addCustomFlags("-Wall -std=c++0x")
uiBundle:addFiles([[
    Bundle.cpp
    ../Shared/WKConversions.cpp
]])
