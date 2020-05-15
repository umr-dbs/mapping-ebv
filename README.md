# MAPPING EBV Module

## Requirements
 * mapping-core

## Building
```
cmake .
make
```

### Options
 * Debug Build
   * `-DCMAKE_BUILD_TYPE=Debug`
 * Release Build
   * `-DCMAKE_BUILD_TYPE=Release`
 * Specify the *mapping-core* path
   * it tries to find it automatically, e.g. at the parent directory
   * `-MAPPING_CORE_PATH=<path-to-mapping-core>` 
