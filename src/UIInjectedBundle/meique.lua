
uiBundle = Library:new("UiBundle")
uiBundle:use(nix)
uiBundle:addIncludePath("../Shared")
uiBundle:addFiles([[
    Bundle.cpp
    ../Shared/WKConversions.cpp
]])
