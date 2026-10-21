# AeroVistaConfig

配置 JSON 库：nlohmann/json 做语法解析。本库提供未知键拒绝、严格整数（`1.0` 可 / `1.5` 否）、缺键 / null，并把 JSON 解析成结构体（`HostConfig`、`IgConfig`、`ViewDefinition`、`EngineChannelConfig`、IG 实体目录 `loadEntitiesFile`、Host 权威表子集 `loadHostEntityCatalog`）。

不依赖 vsg、引擎或 sync。`vsgEngineLib` 与 `aerovistaSync` 都链本库。sync 的 `SyncConfig.h` 只重新导出本库的解析入口，不再自己读 JSON。

```cmake
add_subdirectory(thirdparty/nlohmannJson)
add_subdirectory(thirdparty/config)
target_link_libraries(your_target PRIVATE AeroVistaConfig)
```

```cpp
#include <aerovista/config/ConfigJson.h>

using aerovista::config::parseJsonText;
using aerovista::config::rejectUnknownKeys;
using aerovista::config::requireInt;
```
