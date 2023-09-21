add_library(extract OBJECT 
    CNFBaseFeatures.cc
    CNFSaniCheck.cc
    OPBBaseFeatures.cc
    WCNFBaseFeatures.cc
    MCNFBaseFeatures.cc
)
set_property(TARGET extract PROPERTY POSITION_INDEPENDENT_CODE ON)
