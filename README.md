# AeroVistaConfig

配置 JSON 契约辅助库：nlohmann/json 做语法解析，本库提供未知键拒绝、严格整数（`1.0` 可 / `1.5` 否）、缺键 / null 等读取策略。

不依赖 vsg、引擎或 sync。`vsgEngineLib` 与 `aerovistaSync` 都链本库；结构体解析（`parseWindow`、`parseHostConfig` 等）仍留在各自模块。

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
