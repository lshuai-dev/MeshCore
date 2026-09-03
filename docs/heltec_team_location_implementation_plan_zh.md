# Heltec Team 频道、位置同步与组内通信执行方案

## 1. 文档目的

本文给出一套可以直接在当前 MeshCore Firmware 仓库中实施的工程方案，用于实现以下产品目标：

1. Heltec T1 作为 Team 成员，周期性共享自身 GPS 位置。
2. Heltec V4 R8 TFT 默认作为 Team 队长，显示当前 Team 所有成员的位置。
3. T1 和 V4 R8 TFT 提供 10 个预置 Team 频道；设备选择相同频道后使用相同密钥通信。
4. 位置共享不再依赖高频 MeshCore Identity Advert。
5. 组内文本消息直接复用 MeshCore 原生 Group Text。
6. Team 位置和同步数据使用 MeshCore Group Data 的一个应用数据类型及三个子类型。
7. Team 数据支持有限跳数，避免位置包无限制地在 Repeater 网络中 Flood。

本文既是设计说明，也是实施清单。执行者不得跳过协议测试、旧配置兼容测试和多节点拥塞测试。

## 2. 已确定的设计决策

### 2.1 使用现有 Group，不申请新的 Payload Type

Team 数据使用现有：

```text
PAYLOAD_TYPE_GRP_DATA = 0x06
```

在 Group Data 的 16 位 `data_type` 中申请一个 Heltec Team 编号。开发阶段使用测试范围：

```text
HELTEC_TEAM_DATA_TYPE_DEV = 0xFF48
```

正式发布前，按照 `docs/number_allocations.md` 的要求申请 `0x0100`～`0xFEFF` 范围内的正式编号，并替换开发编号。禁止发布使用 `0xFF48` 的量产固件。

### 2.2 V1 只新增三个 Team 子类型

```text
0x01 TEAM_PRESENCE
0x02 TEAM_LOCATION
0x03 TEAM_SYNC_REQUEST
```

不新增以下帧：

- 不新增 Team Text；文本使用原生 `PAYLOAD_TYPE_GRP_TXT`。
- 不新增 Leave；离线状态根据最后接收时间判断。
- 不新增 Heartbeat；低频 Presence 即为心跳。
- 不新增 Sync Response；收到同步请求后复用 Presence 和 Location 响应。
- 不新增 Location ACK；广播位置不能引发 ACK 风暴。
- V1 不实现自定义二进制消息、分片和文件传输。

### 2.3 只激活一个 Team 频道

固件包含 10 组预置密钥，但运行时只把用户当前选择的一个 Team 频道注册到 MeshCore Group channel 表中。

禁止同时激活全部 10 个频道，否则设备会尝试解密和处理其他 Team 的数据，普通 Group 消息列表也会出现全部 Team。

### 2.4 Team 开启时停止位置 Advert

Team 开启且 Team Location Share 打开时：

- 使用 `TEAM_LOCATION` 发送位置。
- 不再由 `pollLocShareAdvert()` 高频发送带位置的 Identity Advert。
- 仍允许启动、手动操作或 MeshCore 原有策略产生低频身份 Advert。
- 关闭 Team 后恢复现有 Location Share Advert 行为。

### 2.5 Group Text 是组聊，不是私聊

原生 Group Text 的内容是：

```text
timestamp + flags + "sender_name: message"
```

同频道所有成员都能解密。发送者名称未经签名验证，因此 UI 必须称为“Team Chat”或“组内消息”，不得称为“私聊”或“端到端身份认证消息”。

## 3. 当前仓库基线

实施前必须理解以下现有路径：

| 能力 | 当前文件 | 当前行为 |
|---|---|---|
| Packet 类型 | `src/Packet.h` | Group Text 为 `0x05`，Group Data 为 `0x06` |
| Group 加解密 | `src/Mesh.cpp` | Group packet 使用 channel hash 查找密钥并解密 |
| Group Data 分发 | `src/helpers/BaseChatMesh.cpp` | 解析 16 位 `data_type` 和 8 位 `data_len` |
| Group Text 发送 | `src/helpers/BaseChatMesh.cpp` | `sendGroupMessage()` 创建并 Flood Group Text |
| Group Data 发送 | `src/helpers/BaseChatMesh.cpp` | `sendGroupData()` 支持 Flood 或 Direct |
| Heltec Group UI | `src/heltec/app/mesh_app_ui_messaging.cpp` | 已支持选择 Group channel 并发送文本 |
| 位置 Advert 调度 | `src/heltec/app/HeltecMesh.cpp` | `pollLocShareAdvert()` 按周期/移动距离发送 Advert |
| Advert 位置入库 | `src/heltec/app/HeltecMesh.cpp` | `onAdvertRecv()` 将位置关联到全局 Contact |
| Find Friend 数据源 | `src/heltec/app/mesh_app_ui_find_friend.cpp` | 从 Contact 表筛选有 GPS 的联系人 |
| T1 UI | `src/heltec/ui/screens/find_friend_screen.cpp` | 单目标导航与 Waypoint |
| V4 R8 TFT 构建 | `variants/heltec_v4_r8/platformio.ini` | TFT 当前排除了 `find_friend_screen.cpp` |
| 偏好存储 | `src/heltec/config/NodePrefs.h`、`DataStore.cpp` | `/new_prefs` 按固定偏移逐字段读写 |

重要约束：

- `MAX_PACKET_PAYLOAD` 为 184 字节。
- `MAX_GROUP_DATA_LENGTH` 当前为 165 字节。
- Group channel hash 当前只有 1 字节。
- Group MAC 当前为 2 字节。
- Flood 路径的跳数已经记录在 `Packet::path_len` 的低 6 位中。
- `Dispatcher::tryParsePacket()` 当前拒绝大于 `PAYLOAD_VER_1` 的包。
- `Mesh::routeRecvPacket()` 当前只受路径缓冲区大小和 `allowPacketForward()` 限制，没有业务 hop limit。

## 4. 总体架构

新增两个无 UI 依赖的模块：

```text
src/heltec/app/team_protocol.hpp
src/heltec/app/team_protocol.cpp
src/heltec/app/team_service.hpp
src/heltec/app/team_service.cpp
```

模块职责：

### `team_protocol`

- 定义常量、枚举和字节布局。
- 使用显式的小端读写函数编码和解码。
- 验证版本、长度、枚举、坐标和字符串。
- 不访问 GPS、Flash、LVGL、Radio 或全局 `the_mesh`。
- 可在主机单元测试中独立编译。

### `team_service`

- 管理 10 个频道预设和当前激活频道。
- 维护 Team 成员运行时表。
- 调度 Presence、Location 和 Sync Request。
- 处理重启、序列号、重复包、超时和频道切换。
- 提供只读快照给 `MeshAppUi`。
- 不直接创建 LVGL 对象。

依赖方向必须保持为：

```text
UI -> MeshAppUi -> TeamService -> HeltecMesh/BaseChatMesh -> MeshCore
```

禁止让 `TeamService` 依赖具体 screen，也禁止让 screen 直接读取 `HeltecMesh` 内部数组。

## 5. 频道和密钥设计

### 5.1 预设定义

在 `team_protocol.hpp` 或独立的私有配置头中定义：

```cpp
constexpr uint8_t kTeamChannelCount = 10;

struct TeamChannelPreset
{
    uint8_t id;
    const char *name;
    uint8_t key_version;
    uint8_t secret[32];
};
```

频道 ID 固定为 `0`～`9`，UI 显示为 `Team 01`～`Team 10`。

要求：

- 每个频道使用独立的随机 256-bit PSK。
- 密钥不得从频道编号、频道名称或同一个公开字符串派生。
- 密钥不得写入日志。
- 测试密钥和量产密钥必须分离。
- 密钥表只在支持 Team 功能的固件目标中链接。

### 5.2 与现有 Group channel 表的集成

增加一个系统保留的 channel slot，例如：

```text
HELTEC_TEAM_CHANNEL_INDEX = 1
```

槽位 `0` 继续保留现有 Public channel。Team slot 的行为：

- Team 关闭时，名称和密钥清零，不参与频道搜索和消息列表。
- Team 打开时，根据 `team_channel_id` 将一个预设写入该 slot。
- `DataStore::loadChannels()` 完成后再覆盖 Team slot，防止旧存储内容替换系统预设。
- `CMD_SET_CHANNEL` 不得修改系统 Team slot；应返回参数错误或只读错误。
- `saveChannels()` 不保存预置密钥，或者加载后始终由固件预设覆盖。
- `sendMessageGroupAt()` 只显示当前激活的 Team，不显示未激活的另外九个预设。

如果维护者不接受保留普通 channel slot，则让 `TeamService` 独立持有一个 `mesh::GroupChannel`，并扩展 `searchChannelsByHash()` 将当前 Team channel 与普通 channels 一起返回。此时必须另外扩展消息 UI，使原生 `sendGroupMessage()` 能取得该 channel。优先采用保留 slot，改动更小。

### 5.3 密钥安全边界

预置密钥只提供频道隔离，不提供严格成员准入：

- 提取任意支持 Team 的固件可能暴露全部预置密钥。
- 持有密钥的设备可伪造 `sender_id` 和显示名称。
- Group Text 的 sender name 本身未经验证。

V1 产品文档必须明确其适用于可信设备和临时队伍。如果后续要求防止伪造，需要增加设备白名单或签名机制，不在本次范围内。

## 6. Team 协议

### 6.1 字节序和通用规则

- 所有 16 位和 32 位整数使用 Little Endian，与 MeshCore 文档一致。
- 不直接 `memcpy` 整个 C++ struct 上线，避免 padding、ABI 和端序问题。
- 所有编码函数接收输出缓冲区长度并返回实际长度或失败。
- 所有解码函数先检查剩余长度，再读取字段。
- 未识别的协议版本或子类型直接忽略，但仍允许 MeshCore 完成正常去重和路由。
- 坐标沿用项目现有 `1e-6` 度格式，避免 Contact/Advert/Find Friend 之间反复换算。

### 6.2 公共头

三个子类型共享以下加密公共头：

| 偏移 | 字段 | 长度 | 说明 |
|---:|---|---:|---|
| 0 | `protocol_version` | 1 | V1 固定为 `1` |
| 1 | `subtype` | 1 | `1`～`3` |
| 2 | `flags` | 1 | 通用状态位 |
| 3 | `role` | 1 | `0=member`，`1=leader` |
| 4 | `sender_id` | 6 | 本机公钥前 6 字节 |
| 10 | `boot_id` | 2 | 每次启动随机值 |
| 12 | `sequence` | 2 | 每次发送递增，允许回绕 |
| 14 | `timestamp` | 4 | Unix 秒；时间无效时为 `0` |

公共头固定 18 字节。

`sender_id` 使用 6 字节是为了与现有 `sendDirectMessage()` 和 UI 公钥前缀保持一致。成员身份的最终键为：

```text
sender_id + boot_id
```

同一个 `sender_id` 出现不同 `boot_id` 时视为设备重启，重置该成员的 sequence 窗口。

通用 flags：

```text
bit 0: time_valid
bit 1: battery_valid
bit 2: urgent
bit 3..7: reserved, sender must write 0
```

### 6.3 `TEAM_PRESENCE`，子类型 `0x01`

公共头之后：

| 字段 | 长度 | 说明 |
|---|---:|---|
| `device_type` | 1 | `1=T1`，`2=V4_R8_TFT`，其他值预留 |
| `capabilities` | 2 | GPS、地图、触摸、Leader UI 等能力位 |
| `battery_mv` | 2 | 无效时写 0，并清除 `battery_valid` |
| `name_len` | 1 | `0`～`31` |
| `name` | N | UTF-8；V1 UI 可按字节安全截断 |

Presence 用途：

- 加入频道声明。
- Leader 身份声明。
- 低频在线心跳。
- 回应 Sync Request。
- 更新名称、能力和电池状态。

发送条件：

- Team 开启或频道切换后，随机延迟 `0`～`5000 ms`。
- 启动后首次进入正常 Mesh loop。
- 名称、角色或能力改变。
- 每 15 分钟一次，增加正负 10% 抖动。
- 收到 Sync Request 后按响应窗口错峰发送。

### 6.4 `TEAM_LOCATION`，子类型 `0x02`

Location flags 在通用 flags 基础上定义：

```text
bit 4: location_valid
bit 5: altitude_valid
bit 6: motion_valid
bit 7: reserved
```

公共头之后：

| 字段 | 长度 | 单位/说明 |
|---|---:|---|
| `latitude_e6` | 4 | 纬度乘 `1,000,000` |
| `longitude_e6` | 4 | 经度乘 `1,000,000` |
| `altitude_dm` | 2 | 有符号，0.1 米 |
| `accuracy_dm` | 2 | 0.1 米；未知写 0 |
| `speed_cms` | 2 | cm/s；饱和到 `UINT16_MAX` |
| `course_cdeg` | 2 | 0.01 度，范围 `0`～`35999` |
| `fix_age_sec` | 2 | GPS fix 年龄，饱和到 `UINT16_MAX` |
| `satellites` | 1 | 卫星数 |
| `battery_mv` | 2 | 无效时写 0 |

完整 Location 明文为 39 字节，远小于当前 Advert 的公钥和签名开销。

坐标验证：

```text
-90,000,000 <= latitude_e6 <= 90,000,000
-180,000,000 <= longitude_e6 <= 180,000,000
```

状态变化规则：

- 从有效 fix 变为无效时发送一帧 `location_valid=0`，之后停止重复发送无效状态。
- 从无效恢复有效时立即发送。
- 接收无效位置时保留成员，但将其位置标记为不可用于导航。

### 6.5 `TEAM_SYNC_REQUEST`，子类型 `0x03`

公共头之后：

| 字段 | 长度 | 说明 |
|---|---:|---|
| `request_id` | 2 | 请求方递增或随机生成 |
| `response_mask` | 1 | bit0=Presence，bit1=Location |
| `response_window_sec` | 1 | 建议 `5`～`30` 秒 |

接收行为：

1. 检查 `(sender_id, boot_id, request_id)` 是否最近处理过。
2. 计算确定性延迟，避免所有成员同时响应：

   ```text
   delay_ms = CRC32(local_sender_id || request_id)
              % (response_window_sec * 1000)
   ```

3. 如果请求 Presence，则在该延迟发送 Presence。
4. 如果请求 Location 且存在有效 fix，则在 Presence 后再增加 `300`～`1000 ms` 发送 Location。
5. 同一个请求只响应一次。

队长进入 Team 页面时不要每次都请求同步。仅在成员表为空、用户手动刷新或距离上次请求超过 60 秒时发送。

## 7. 有限跳数设计

### 7.1 必须满足的原则

Team 子类型和 `data_type` 位于 Group 密文中。没有 Team 密钥的 Repeater 无法读取，因此：

> 禁止只把 `max_hops` 放入 `TEAM_LOCATION` 加密正文，然后声称 Repeater 会执行它。

`max_hops` 必须位于 Repeater 可读取的 Mesh 路由元数据中，并由 `Mesh::routeRecvPacket()` 在追加本机 path hash 前检查。

转发条件定义为：

```text
current_hops < max_hops
```

其中 `current_hops = packet->getPathHashCount()`。当相等或超过时，本机仍可解密和消费，但返回 `ACTION_RELEASE`，不再转发。

### 7.2 推荐的 MeshCore 扩展流程

先向 MeshCore 维护者提交设计，不直接占用保留字段。申请内容应是通用的“Flood hop limit”，而不是 Heltec 私有规则：

- 适用于 Group Data，也可供其他 Flood payload 使用。
- hop limit 对 Repeater 明文可见。
- 不改变 Team 加密正文。
- 继续使用现有 path hash count 统计实际跳数。
- 明确旧固件 Repeater 不会执行限制。

如果上游批准复用 `transport_codes[1]`，可以采用带魔数的编码，例如高 12 位为协议标记、低 4 位为 `max_hops`。如果上游选择其他位置，严格跟随上游结论。禁止在没有批准的情况下将 `transport_codes[1]` 私自固化为公开协议。

需要修改或新增的核心 API 应达到以下语义：

```cpp
void sendFloodLimited(Packet *packet,
                      uint8_t max_hops,
                      uint32_t delay_millis,
                      uint8_t path_hash_size);

bool Packet::hasFloodHopLimit() const;
uint8_t Packet::getFloodHopLimit() const;
```

`Mesh::routeRecvPacket()` 的检查顺序：

1. 必须是 Flood。
2. 未标记 `DoNotRetransmit`。
3. 未超过 path 缓冲区。
4. `allowPacketForward()` 返回 true。
5. 若有 hop limit，确认当前 hop count 小于 max hops。
6. 追加本机 hash 并排队转发。

### 7.3 默认跳数

| 数据 | 默认 `max_hops` |
|---|---:|
| Presence | 2 |
| Location | 2 |
| Sync Request | 2 |
| 原生 Team Group Text | 3 |

为了让原生 Group Text 也受限，TeamService/消息 facade 在识别到系统 Team channel 时必须走“有限跳数 Group Text”发送入口；普通 Public/user channels 保持原有 Flood 行为。

### 7.4 兼容限制

- 未升级的 Repeater 会忽略新 hop limit，仍可能继续 Flood。
- 因此“严格限制两跳”只有在传播路径中的 Repeater 全部升级后成立。
- 新 Team endpoint 必须仍能接收旧的普通 Group Text。
- 如果上游扩展改变 Packet wire format，必须增加显式版本兼容测试，不能直接提升全局 `PAYLOAD_VER` 并导致旧包全部被拒绝。

## 8. TeamService 运行时状态

### 8.1 成员表

建议默认容量：

```text
HELTEC_TEAM_MAX_MEMBERS = 32
```

成员项至少包含：

```cpp
struct TeamMember
{
    uint8_t sender_id[6];
    uint16_t boot_id;
    uint16_t last_sequence;
    uint32_t last_seen_ms;
    uint32_t last_presence_ms;
    uint32_t last_location_ms;
    int32_t latitude_e6;
    int32_t longitude_e6;
    int16_t altitude_dm;
    uint16_t accuracy_dm;
    uint16_t speed_cms;
    uint16_t course_cdeg;
    uint16_t battery_mv;
    uint8_t satellites;
    uint8_t role;
    uint8_t device_type;
    uint16_t capabilities;
    bool location_valid;
    char name[32];
};
```

分配策略：

- 相同 sender ID 更新原项。
- 表未满时使用空项。
- 表满时淘汰最久未见且不是当前 UI 选中目标的成员。
- Team Leader 不因普通成员更新而被优先淘汰。
- 不将成员表周期性写入 Flash。

### 8.2 在线和位置新鲜度

```text
Online: last_seen <= 2 * 当前 Presence/Location 周期
Aging:  超过 Online，但不超过 10 分钟
Offline: 超过 10 分钟
```

导航位置新鲜度沿用现有 `FindFriendNavEstimator` 的 Fresh/Aging/Stale 模型。数据源从 Contact receipt 改为 Team member snapshot，但不重写距离、方位和平滑算法。

### 8.3 去重和乱序

MeshCore 已按 packet hash 去重，但 TeamService 仍需做语义去重：

- boot ID 相同：仅接受比 `last_sequence` 更新的 sequence，按 16 位回绕比较。
- boot ID 不同：接受为重启后的新会话，并重置 sequence。
- 相同 Sync Request 只响应一次。
- timestamp 仅用于展示，不作为唯一的乱序依据。
- 本机 RTC/GPS 时间无效时允许 timestamp 为 0。

## 9. 位置发送策略和拥塞控制

默认策略：

| 状态 | 发送策略 |
|---|---|
| 首次有效 GPS | 立即发送 |
| 正常移动 | 每 30 秒，或移动超过 30 米 |
| 静止 | 每 180 秒 |
| GPS 从有效变无效 | 立即发送一次无效 Location |
| GPS 持续无效 | 不周期发送 Location |
| Presence | 每 15 分钟 |

调度要求：

- 所有周期增加正负 10% 抖动。
- 频道切换和同步响应必须随机错峰。
- 发送队列繁忙或 Radio budget 不足时延后位置包，不能挤占文本消息。
- 同一设备队列中只保留最新的待发 Location；新位置覆盖尚未发送的旧位置。
- Location 不重发、不等待 ACK。
- Text 优先级高于 Presence 和 Location。
- Sync Request 最短间隔 60 秒。
- 不使用 `GPS_NOTIFY_INTERVAL_SEC=1` 作为无线发送周期；该值只允许驱动本地 UI/GPS 更新。

## 10. HeltecMesh 集成

### 10.1 接收 Group Data

在 `HeltecMesh` 中 override：

```cpp
void onChannelDataRecv(const mesh::GroupChannel &channel,
                       mesh::Packet *packet,
                       uint16_t data_type,
                       const uint8_t *data,
                       size_t data_len) override;
```

处理顺序：

1. `data_type` 不是 Heltec Team 时交给现有/未来其他处理器或忽略。
2. channel 不是当前激活 Team channel 时忽略。
3. 调用 `TeamProtocol::decode()`。
4. 调用 `TeamService::handleFrame()`。
5. 成员或位置发生可见变化时发送 `AppStateEventType::TeamChanged`。
6. 不把 Team Presence 自动写入全局 Contact 表。

### 10.2 发送 Team Group Data

提供一个窄接口，不让 TeamService直接操作 Packet：

```cpp
bool HeltecMesh::sendTeamData(const uint8_t *data,
                              size_t data_len,
                              uint8_t max_hops);
```

内部：

1. 获取当前 Team `mesh::GroupChannel`。
2. 调用 `sendGroupData()` 或新增的有限跳数变体。
3. 使用 `HELTEC_TEAM_DATA_TYPE_DEV/正式编号`。
4. 应用 Location/Presence 的低优先级。

### 10.3 替换位置 Advert 调度

修改 `pollLocShareAdvert()` 的调用策略，而不是删除旧代码：

```text
if Team enabled and Team location share enabled:
    TeamService::pollLocation()
else:
    existing pollLocShareAdvert()
```

`createPolicySelfAdvert()`、手动 `advert()` 和普通联系人发现继续保留，确保 Companion 和旧设备兼容。

## 11. 配置持久化

在 `NodePrefs` 尾部追加：

```cpp
uint8_t team_enabled;
uint8_t team_channel_id;
uint8_t team_role;
uint8_t team_location_share;
```

默认值：

| 目标 | enabled | channel | role | location share |
|---|---:|---:|---|---:|
| T1 | 0 | Team 01 | Member | 1 |
| V4 R8 TFT | 0 | Team 01 | Leader | 1 |
| 其他目标 | 0 | Team 01 | Member | 0 |

存储修改要求：

- `DataStore::savePrefs()` 在偏移 95 后顺序追加四个字段。
- `DataStore::loadPrefsInt()` 使用 `file.available()` 逐字段判断。
- 旧文件缺少字段时应用上述默认值。
- channel ID 加载后约束到 `0`～`9`。
- role 和 boolean 字段加载后做范围规范化。
- 不改变之前字段的偏移。
- 增加旧 96 字节 prefs 文件迁移测试。

如果后续 Team 配置继续扩展，应考虑建立独立的版本化 `/team_prefs`，V1 暂不需要扩大范围。

## 12. UI 和业务 facade

### 12.1 通用 facade

在 `biz_facade.hpp` 增加只读 DTO 和操作接口，不暴露 `TeamMember` 内部指针：

```text
TeamUiSnapshot
TeamMemberUiItem
teamEnabled()/setTeamEnabled()
teamChannel()/setTeamChannel()
teamMemberCount()/fillTeamMembers()
requestTeamSync()
```

在 `AppStateEventType` 增加：

```text
TeamChanged
```

Team member 位置变化只发送 invalidation event，screen 在刷新时从 facade 获取新快照，沿用当前 AppState 的模式。

### 12.2 T1

复用现有 Find Friend 页面和 `FindFriendNavEstimator`：

- Find Friend 的 Contact 模式改为 Team Member 模式。
- 目标列表来源改为当前 Team 成员表。
- 只列出具有有效或历史位置的成员。
- 默认优先选择 role 为 Leader 的成员。
- 保存选择时使用 6 字节 sender ID，而不是 Contact 数组索引。
- Waypoint 模式保持不变。
- 增加 Team Off/Team 01～Team 10 选择入口。
- 增加 Team Chat 入口，目标为当前系统 Team channel。

不要删除 Contact Location 代码；它仍用于 Team 关闭时的旧功能兼容。

### 12.3 V4 R8 TFT

V4 R8 TFT 当前排除了 `find_friend_screen.cpp`，不能简单打开 T1 单目标罗盘页面。新增 TFT Team 页面，建议文件：

```text
src/heltec/ui/screens/team_map_screen.hpp
src/heltec/ui/screens/team_map_screen.cpp
```

V1 UI 最小范围：

- Team 开关和 10 频道选择。
- 成员列表：名称、角色、在线状态、位置年龄、电量。
- 地图显示所有拥有有效位置的成员。
- Leader 使用不同图标。
- 点击成员后显示经纬度、距离和最后更新时间。
- Team Chat 按钮打开现有发送消息 overlay，并预选系统 Team channel。
- 手动 Refresh 按钮发送 Sync Request，并具有 60 秒冷却。

不要在 V1 实现聊天气泡、历史会话数据库或二进制文件 UI。

## 13. 原生 Group Text 集成

发送端继续调用现有：

```cpp
the_mesh.sendGroupMessage(timestamp,
                          team_channel,
                          the_mesh.getNodeName(),
                          text,
                          text_len);
```

需要补充：

- 系统 Team channel 出现在 Group 消息目标列表中。
- Team 关闭时不出现该目标。
- 接收到当前 Team channel 的 Group Text 时沿用离线队列和 Preview UI。
- 显示来源建议为 `Team 01 / sender`，不要只显示 `Unknown`。
- Team Group Text 使用默认 3 hops 的有限 Flood 发送入口。
- 普通 Group channel 行为不变。

## 14. 构建配置

新增功能宏：

```text
HELTEC_TEAM_FEATURE=1
HELTEC_TEAM_MAX_MEMBERS=32
HELTEC_TEAM_CHANNEL_INDEX=1
HELTEC_TEAM_LOCATION_HOPS=2
HELTEC_TEAM_TEXT_HOPS=3
```

仅在以下目标启用：

- `variants/heltec_t1/platformio.ini` 的 companion radio USB/BLE 环境。
- `variants/heltec_v4_r8/platformio.ini` 的 TFT companion radio USB/BLE/Wi-Fi 环境。

第一阶段不要在 repeater、room server、sensor 和 KISS modem 环境启用 Team UI/Service。通用 hop-limit 路由支持可以在所有 MeshCore 节点编译，以便 Repeater 执行转发限制。

## 15. 分阶段实施清单

### 阶段 A：协议和主机测试

1. 新建 `team_protocol.hpp/.cpp`。
2. 定义开发 data type 和三个 subtype。
3. 实现小端读写、Presence/Location/Sync 编解码。
4. 新建 `test/test_team_protocol/test_team_protocol.cpp`。
5. 覆盖正常、截断、超长名称、非法坐标、未知版本、未知 subtype。

完成标准：协议测试在无 Arduino/LVGL 环境下通过，没有 packed struct。

### 阶段 B：通用 Flood hop limit

1. 与 MeshCore 上游确认 wire encoding。
2. 在 `Packet` 中增加 hop-limit 查询接口。
3. 增加有限 Flood 发送 API。
4. 修改 `Mesh::routeRecvPacket()`。
5. 添加 0、1、2、最大跳数和无 hop limit 的路由测试。
6. 验证 Direct packet 不受影响。

完成标准：2-hop packet 在第三个节点可被消费但不会再次排队转发。

### 阶段 C：频道和配置

1. 增加十组测试 PSK。
2. 增加 Team prefs 并兼容旧文件。
3. 实现系统 Team channel slot。
4. 阻止 Companion 修改该 slot。
5. 实现频道切换和运行时清理。

完成标准：不同频道无法互相解密；重启后选择保持；旧 prefs 正常升级。

### 阶段 D：TeamService

1. 实现成员表。
2. 实现 boot ID、sequence 和去重。
3. 实现三个 frame handler。
4. 实现位置调度、移动阈值和抖动。
5. 实现 Sync Response 错峰。
6. 接入 `onChannelDataRecv()`。

完成标准：32 个模拟成员可稳定增删更新，乱序包不覆盖新位置。

### 阶段 E：替换位置共享

1. Team 开启时切换到 Team Location。
2. 停止高频位置 Advert。
3. 保留 Team 关闭时旧逻辑。
4. 验证 GPS 失效和恢复状态。

完成标准：开启 Team 后抓包不再出现周期性位置 Advert，只出现受限跳数 Group Data。

### 阶段 F：T1 UI

1. 将 Find Friend 数据源改为 Team 成员。
2. 保留导航估算器。
3. 增加频道选择和同步操作。
4. 增加 Team Chat 入口。

完成标准：T1 可选择 Leader，显示距离、方向、新鲜度并发送组消息。

### 阶段 G：V4 R8 TFT UI

1. 新建 Team map screen。
2. 显示成员列表和地图标记。
3. 增加频道选择、刷新和 Team Chat。
4. 适配触摸和页面生命周期。

完成标准：至少 10 个模拟成员可同时显示，进入退出页面无内存增长。

### 阶段 H：正式编号和量产密钥

1. 展示可工作的 POC。
2. 向 MeshCore 申请正式 Group Data type。
3. 更新 `docs/number_allocations.md`。
4. 替换开发编号和测试密钥。
5. 再做跨版本互通测试。

## 16. 测试矩阵

### 16.1 协议测试

- 三种 frame encode/decode round trip。
- 每个长度边界：0、最小、最大、超长、截断。
- sequence 回绕：`65535 -> 0`。
- boot ID 改变后接受低 sequence。
- 非法纬度、经度、course 和 name length。
- 未知 flags 保持可忽略。

### 16.2 频道测试

- 两台设备选择相同频道时互通。
- 不同频道不产生 Team/UI 更新。
- 切换频道后旧成员立即清空。
- Team 关闭后不发送 Presence/Location。
- Public 和用户自建 channel 不受影响。

### 16.3 路由测试

- `max_hops=0`：仅发送端无线发出，接收节点不继续转发。
- `max_hops=1`：一跳节点消费，不继续转发。
- `max_hops=2`：第二跳节点消费，不继续转发。
- 无 hop limit 的旧 Group packet 保持现有行为。
- 旧 Repeater 存在时记录其继续 Flood 的兼容限制。

### 16.4 GPS 测试

- 冷启动无 fix。
- 首次 fix。
- 静止 30 分钟。
- 连续移动。
- fix 丢失和恢复。
- GPS 时间无效但坐标有效。
- 经纬度边界和零坐标。

### 16.5 负载测试

至少模拟或实测：

```text
1 Leader + 10 Members + 2 Repeaters
1 Leader + 32 Members + 3 Repeaters
```

记录：

- 每分钟原始 Team packet 数。
- 每分钟 Repeater 转发数。
- Radio airtime。
- Dispatcher 队列峰值。
- Group Text 发送延迟。
- Location 丢包率和平均年龄。

验收目标：

- 正常 30 秒移动周期下不发生持续 outbound queue 堆积。
- Group Text 不被周期 Location 长时间阻塞。
- 同步请求后响应分布在配置窗口内，不出现集中发送峰值。

### 16.6 构建验证

每个阶段至少编译：

```text
Heltec_t1_companion_radio_usb
Heltec_t1_companion_radio_ble
heltec_v4_r8_tft_companion_radio_usb
heltec_v4_r8_tft_companion_radio_ble
heltec_v4_r8_tft_companion_radio_wifi
```

修改通用 `Packet`、`Mesh` 或 `Dispatcher` 后，还必须选择至少一个 Repeater 环境和一个不启用 Team 的普通 companion 环境编译，防止核心回归。

## 17. 代码质量约束

- 所有外部长度、索引、坐标和枚举先验证再使用。
- 局部变量初始化；结构体使用 `{}`。
- 只读输入使用 `const`。
- 禁止 `strcpy()`、`sprintf()`；使用有边界的复制和格式化。
- 协议代码单函数尽量不超过 50 行。
- 不使用动态内存维护成员表和协议缓冲区。
- nRF52 热路径避免大栈数组；协议最大缓冲区应由调用方提供或使用有所有权的固定缓冲区。
- 新增日志不得打印密钥、完整明文位置历史或个人消息。
- 新文件遵循项目 C/C++ 格式，并以换行结束。

## 18. 明确不在 V1 范围内的内容

- 私密的一对一 Team 消息。
- 可靠 Group Text ACK。
- 二进制分片和文件传输。
- 队长签发成员证书。
- 密钥远程轮换。
- 多 Leader 选举和冲突解决。
- 将所有 Team 成员持久化为 MeshCore Contact。
- 旧 Repeater 对 hop limit 的强制兼容。

## 19. 最终验收定义

只有同时满足以下条件，功能才算完成：

1. T1 和 V4 R8 TFT 均可选择 10 个预置 Team 频道。
2. 相同频道设备可发现成员、同步位置并使用原生 Group Text 聊天。
3. 不同频道不会进入彼此的 Team 成员表或 Team Chat。
4. Team 开启时不再周期发送带位置的 Identity Advert。
5. Presence、Location 和 Sync Request 使用一个正式/开发 Group Data type 的三个 subtype。
6. 位置帧默认最多传播两跳，并在升级后的 Repeater 上实际停止继续转发。
7. 同步响应具有确定性错峰，不产生响应风暴。
8. T1 可导航到选中的 Team 成员。
9. V4 R8 TFT 可同时查看多个 Team 成员的位置和状态。
10. 原有 Public channel、普通 contacts、Advert、Direct message 和 Team 关闭时的 Find Friend 行为没有回归。
11. 协议单元测试、配置迁移测试、路由测试和指定 PlatformIO 环境全部通过。

## 20. 实施者开始编码前必须确认的事项

以下三项尚不能由代码自行决定：

1. MeshCore 上游为 Heltec Team 分配的正式 Group Data type。
2. MeshCore 上游认可的 hop-limit wire encoding；在确认前只能使用受宏保护的 POC 编码。
3. 10 个量产频道的最终随机 PSK，以及密钥由谁生成、保管和注入构建。

除上述事项外，执行者应按照本文默认值推进，不应因为次要命名或 UI 文案问题停止协议和服务层实现。
