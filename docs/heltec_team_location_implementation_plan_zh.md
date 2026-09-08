# Heltec Team 频道、位置同步与组内通信执行方案

> 文档状态：待实施（V1 设计基线）
>
> 最后校对：2026-09-04
>
> 适用目标：`Heltec_t1_companion_radio_usb/ble` 和
> `heltec_v4_r8_tft_companion_radio_usb/ble/wifi`

## 1. 文档目的

本文给出一套可以直接在当前 MeshCore Firmware 仓库中实施的工程方案，用于实现以下产品目标：

1. 任何启用 Team 功能的设备都可以按构建角色作为 Leader 或 Member；角色不由设备型号决定。
2. V4 R8 TFT 构建为 Leader 时，可通过 Tracker Screen 显示 Team 成员的位置和状态；
   其他设备作为 Leader 时可使用其自身 UI 或 Companion 接口管理 Team。
3. Leader 创建 Team 并生成随机组密钥；Member 通过隐藏 Join Group 发现处于限时配对窗口的
   Leader，经双方核对配对确认码、队长在设备端批准后才下发真正密钥。
4. 位置共享不再依赖高频 MeshCore Identity Advert。
5. 发给全队的文本消息复用 MeshCore 原生 Group Text；发给单个队友的消息
   复用原生 Direct Message。
6. Team 位置和同步数据使用 MeshCore Group Data 的一个应用数据类型及三个子类型。
7. Team 数据支持有限跳数，避免位置包无限制地在 Repeater 网络中 Flood。

本文既是设计说明，也是实施清单。执行者不得跳过协议测试、旧配置兼容测试和多节点拥塞测试。

本文中的关键词含义：

- “必须/禁止”是合并和发布门禁。
- “建议”是默认实现；偏离时必须在 PR 中记录原因和替代验证。
- “POC”仅用于开发验证，不得进入量产构建。

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
已入队日常帧和入队前 Join 帧可使用同一 `data_type` 的不同 subtype，但 Join 帧只能
通过固件内置的隐藏 Join Group 发送，日常帧只能通过动态 Team Group 发送。

### 2.2 入队后 Team Data 只新增三个子类型

```text
0x01 TEAM_PRESENCE
0x02 TEAM_LOCATION
0x03 TEAM_SYNC_REQUEST
```

上述三帧只用于已安装 Team Key 的日常运行。加入前的配对与密钥下发使用独立的
Join Protocol，见 5.2，不与 Team Group Data 共用密钥。

不新增以下已入队帧：

- 不新增 Team Text；群发使用 `PAYLOAD_TYPE_GRP_TXT`，单发使用原生 Direct Message。
- 不新增 Leave；离线状态根据最后接收时间判断。
- 不新增 Heartbeat；低频 Presence 即为心跳。
- 不新增 Sync Response；收到同步请求后复用 Presence 和 Location 响应。
- 不新增 Location ACK；广播位置不能引发 ACK 风暴。
- V1 不实现自定义二进制消息、分片和文件传输。

### 2.3 只激活一个动态 Team

不再内置预置 Team 频道或量产 PSK。每个 Team 由 Leader 生成独立的随机 256-bit
Team Key，运行时只把已创建/已加入的一个 Team 注册到 MeshCore Group channel 表中。

Member 不能在本机创建 Team Key。Member 只能在 Leader 主动开启的限时配对窗口内申请，
双方必须显示相同的配对确认码，且 Leader 对每个申请显式同意或拒绝；未同意前绝不下发
Team Key。

### 2.4 角色由构建宏决定

```text
HELTEC_TEAM_ROLE=0  Member（默认）
HELTEC_TEAM_ROLE=1  Leader
```

源码中给宏提供默认值 0。任何设备的构建环境都可以显式设置为 1，从而具备 Leader
协议能力；设备型号不参与角色判断。当前 V4 R8 TFT 的 Team 构建默认设置为 1，并额外
实现 Tracker Screen 的成员可视化和审批入口。其他 Leader 目标必须提供等价的本机 UI
或 Companion 管理入口，才能实际创建 Team、开放加入窗口及审批申请。
不根据板型宏、设备名称或运行时配置推断角色，UI 也不提供角色切换。

### 2.5 Team 开启时停止位置 Advert

Team 开启即自动共享位置，不再提供独立 Location Share 开关：

- 使用 `TEAM_LOCATION` 发送位置。
- 不再由 `pollLocShareAdvert()` 高频发送带位置的 Identity Advert。
- 仍允许启动、手动操作或 MeshCore 原有策略产生低频身份 Advert。
- 退出/关闭 Team 后停止 Group 位置发送，不恢复旧 Location Share Advert。
- GPS 页面移除 Location Share 和旧 Adv Interval；Team 使用自己的发送周期。

### 2.6 Group Text 是组聊，不是私聊

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
- T1 当前为 `MAX_GROUP_CHANNELS=32`，V4 R8 TFT companion 环境为 40。
- `searchChannelsByHash()` 当前扫描所有数组项，不会自动忽略空名称/零密钥项。
- `Mesh::onRecvPacket()` 当前一次最多尝试 4 个同 hash Group channel。
- `sendGroupMessage()` 和 Flood `sendGroupData()` 当前都通过同一
  `sendFloodScoped()` 路径发送，优先级没有区分。
- Flood 路径的跳数已经记录在 `Packet::path_len` 的低 6 位中。
- `Dispatcher::checkRecv()` 当前拒绝大于 `PAYLOAD_VER_1` 的包。
- `Mesh::routeRecvPacket()` 当前只受路径缓冲区大小和 `allowPacketForward()` 限制，没有业务 hop limit。

## 4. 总体架构

新增两个无 UI 依赖的模块：

```text
src/heltec/app/team_protocol.hpp
src/heltec/app/team_protocol.cpp
src/heltec/app/team_join_service.hpp
src/heltec/app/team_join_service.cpp
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

- 管理已安装的动态 Team Credentials 和当前激活频道。
- 维护 Team 成员运行时表。
- 调度 Presence、Location 和 Sync Request。
- 处理重启、序列号、重复包、超时和 Team Key 替换。
- 提供只读快照给 `MeshAppUi`。
- 不直接创建 LVGL 对象。

### `team_join_service`

- Leader 创建 Team Credentials、限时配对窗口和待确认申请表。
- Member 管理 Team 发现、ECDH 配对会话和 Team Credentials 安装。
- 封装经审查密码库，不在 UI 或 `TeamService` 中实现密码学细节。
- 只向 UI 发布 Team 摘要、配对确认码和状态，不发布共享秘密、会话密钥或 Team Key。

依赖方向必须保持为：

```text
UI -> MeshAppUi -> TeamJoinService/TeamService -> HeltecMesh/BaseChatMesh -> MeshCore
```

禁止让 `TeamService` 依赖具体 screen，也禁止让 screen 直接读取 `HeltecMesh` 内部数组。

### 4.1 最小接口与所有权

`team_protocol` 使用值类型和调用方提供的缓冲区，不分配堆内存。推荐的最小接口：

```cpp
enum class TeamDecodeResult : uint8_t
{
    Ok,
    UnsupportedVersion,
    UnsupportedSubtype,
    InvalidLength,
    InvalidField,
    InvalidUtf8
};

bool encode_team_frame(const TeamFrame &frame,
                       uint8_t *output,
                       size_t output_capacity,
                       size_t &output_length);

TeamDecodeResult decode_team_frame(const uint8_t *data,
                                   size_t data_length,
                                   TeamFrame &frame);
```

- 解码失败时 `frame` 保持零初始化状态，不暴露部分解码结果。
- 输出缓冲区容量不足时编码返回 false，`output_length=0`。
- `TeamService` 拥有成员表、调度器和去重缓存；`HeltecMesh` 拥有 packet 的创建与入队。
- 发送接口必须以 bool/错误码反映“已成功入队”，不得把“已生成帧”当成发送成功。

## 5. 频道和密钥设计

### 5.1 Leader 创建 Team

Leader 首次点击“Create Team”时生成：

```cpp
struct TeamCredentials
{
    uint64_t team_id;
    uint32_t key_version;
    uint8_t secret[32];
    char name[32];
};
```

- `team_id` 和 256-bit `secret` 都使用 MeshCore CSPRNG，不从设备 ID、配对确认码、时间或名称派生。
- `key_version` 初始为 1，每次重置 Team Key 递增，回绕到 0 时跳过 0。
- Team 名称由 Leader 输入，按 6.1 的 UTF-8 规则验证；名称不是密钥的一部分。
- UI 只显示例如 `Rescue Team · A7C3` 的短指纹，不显示或导出完整 Team Key。

Member 没有“Create Team”能力。Leader 重置 Team Key 时，旧成员立即失去访问权，
所有需要继续留队的 Member 必须重新申请。

### 5.2 限时配对和入队确认

所有启用 Team 功能的设备内置同一组隐藏 Join Group Credentials，仅用于让尚未加入 Team
的设备交换 Join 帧。该固定密钥可从固件中提取，因此不是访问凭证，也不得用于加密 Team Key。
Join Group 不出现在普通频道列表、Companion `GET/SET_CHANNEL` 或消息目标中。

Leader 只有在用户点击“开放加入”后才开启配对窗口，默认持续 120 秒，并允许随时关闭。
这不是选择某个设备执行“添加成员”：窗口开启期间，附近任何兼容 Member 都可以主动
发现 Team 并提交加入申请，Leader 再对每一项申请核对确认码并同意或拒绝。窗口关闭时
Leader 不发送 Join 响应、不创建会话，也不弹出申请。Member 点击“查找队伍”
时才开始最长 15 秒的前台搜索；没有后台周期性 Join Beacon 或 Identity Advert。

V1 复用设备现有 Ed25519 身份公钥以及 `LocalIdentity::calcSharedSecret()` 的 ECDH 能力，
不新增长期或临时私钥格式。会话密钥必须通过经审查的 HKDF-SHA-256 从以下输入派生，
不能直接截取 ECDH 输出：

```text
shared_secret = ECDH(local_identity_private, peer_identity_public)
transcript_hash = SHA-256(protocol_version || session_id || team_id ||
                         leader_public_key || member_public_key ||
                         leader_nonce || member_nonce)
root_key = HKDF-Extract-SHA-256(salt=transcript_hash, IKM=shared_secret)
decision_key = HKDF-Expand-SHA-256(root_key, "Leader to Member v1", 32)
ack_key = HKDF-Expand-SHA-256(root_key, "Member to Leader v1", 32)
pairing_key = HKDF-Expand-SHA-256(root_key, "Pairing Code v1", 32)
pairing_mac = HMAC-SHA-256(pairing_key, transcript_hash)
pairing_code = LE32(pairing_mac[0..3]) mod 10000
```

`leader_nonce` 和 `member_nonce` 均为每次会话重新生成的 128-bit CSPRNG 随机数。
`pairing_code` 固定格式化为含前导零的 4 位数字，只用于人工核对，不作为 KDF 输入、不通过
无线发送，也不能推导 Team Key。每个会话只有一次批准机会，因此在线主动攻击者单次蒙中的
概率上限为 `1/10000`；需要更高等级时可在后续协议版本升级为 6 位。

逻辑帧固定为：

```text
JOIN_DISCOVER   Member 主动广播发现请求和短期 request_id
JOIN_OFFER      Leader 返回 session_id、Team 摘要、公钥和 leader_nonce
JOIN_REQUEST    Member 返回选中的 session_id、公钥、名称和 member_nonce
JOIN_DECISION   Leader 加密的 Approve/Reject；Approve 携带 Team Credentials
JOIN_ACK        Member 确认已持久化 Team Credentials 并切换到 Team Group
```

Join subtype 使用 `0x10`～`0x14`，外层复用隐藏 Join Group Data，固定 `max_hops=1`。
`JOIN_DISCOVER`、`JOIN_OFFER` 和 `JOIN_REQUEST` 不包含 Team Key；`JOIN_DECISION` 必须使用
`decision_key` 和带 128-bit tag 的 AEAD 加密并认证，`JOIN_ACK` 使用 `ack_key` 认证。
两者的 AAD 都包含完整 `transcript_hash`、subtype 和方向。
不得复用当前 Group 的 2 字节 MAC 作为内层认证，也不得自行组合 AES-ECB 与短 MAC。

每个 AEAD 帧使用独立的 96-bit CSPRNG nonce。重发必须缓存并原样发送同一密文，禁止使用
同一 key/nonce 重新加密不同内容。会话完成、拒绝或超时后立即清零共享秘密、派生密钥和
缓存密文；同一 `session_id` 不得恢复或再次批准。

必须按以下顺序执行：

1. Leader 创建 Team 后，由用户点击“开放加入”，显示 120 秒倒计时和“停止加入”。
   窗口不预先指定申请人；其有效期内任何兼容 Member 都可以发起申请。
2. Member 点击“查找队伍”发送 `JOIN_DISCOVER`；处于窗口内的 Leader 各回复一次
   `JOIN_OFFER`，Member 列出 Team 名称和 Leader 短指纹供用户选择。
3. Member 选择 Team 后发送 `JOIN_REQUEST`。双方计算同一会话密钥和 4 位配对确认码；
   Member 显示“请将 4 位数字告诉队长”，Leader 弹窗同时显示 Member 名称、公钥短指纹和确认码。
4. 队长核对两台设备显示的数字。只有数字一致时才点击“同意”；不一致时点击“拒绝”。
5. Leader 点击“同意”后，使用 AEAD 下发 `team_id`、`key_version`、`secret`、Team 名称
   和 Leader 公钥；点击“拒绝”时只发送不含原因和凭据的认证 Reject。
6. Member 完整验证 AEAD 并原子持久化凭据，再发送 `JOIN_ACK` 并安装运行时 Team Group。
7. Leader 收到 `JOIN_ACK` 后才把 Member 标记为“Joined”；超时显示“Key sent, unconfirmed”。

30 秒未处理、配对窗口关闭、确认码不一致、AEAD 校验失败或会话过期都不得下发 Team Key。
Leader 最多保留 4 个待确认申请，同时只显示一个弹窗；同一 Member/session 的重发合并。
同一公钥 60 秒内最多创建 3 个会话，全局每分钟最多处理 10 个 `JOIN_DISCOVER`；超限包静默
丢弃，避免隐藏 Join Group 被用于持续骚扰 UI 或耗尽 packet pool。

一次前台搜索最多发送 3 次相同 `request_id` 的 `JOIN_DISCOVER`，间隔使用带抖动的退避；
Leader 对同一 `request_id` 只创建一个 session，并把 Offer 随机延迟 `0`～`2000 ms`，降低多个
Leader 同时回复的碰撞。Offer、Request、Decision 和 ACK 都必须按 `(peer_public_key, session_id)`
去重；需要可靠重发时复用已编码报文，搜索结束或页面退出后不得继续产生 Join 流量。

### 5.3 与现有 Group channel 表的集成

采用“扩容后保留两个末尾 slot”，不得直接占用现有索引 1。否则升级后会静默覆盖用户的
第二个频道。Join slot 固定，Team slot 动态：

```text
T1:        MAX_GROUP_CHANNELS 32 -> 34, Join slot = 32, Team slot = 33
V4 R8 TFT: MAX_GROUP_CHANNELS 40 -> 42, Join slot = 40, Team slot = 41

HELTEC_TEAM_USER_CHANNEL_CAPACITY = MAX_GROUP_CHANNELS - 2
HELTEC_TEAM_JOIN_CHANNEL_INDEX = MAX_GROUP_CHANNELS - 2
HELTEC_TEAM_CHANNEL_INDEX = MAX_GROUP_CHANNELS - 1
```

两个系统 slot 的行为：

- 旧 `/channels2` 记录仍按原索引加载，最多加载
  `HELTEC_TEAM_USER_CHANNEL_CAPACITY` 项，不丢失原有 32/40 个频道。
- Join slot 在 Team 功能初始化时从只读固件常量安装；Team 关闭时仍保留，但只有主动搜索的
  Member 和已打开配对窗口的 Leader 才处理其报文。
- Team 关闭时，Team slot 的名称、hash 和密钥全部清零。
- `searchChannelsByHash()` 必须跳过 `name[0] == '\0'` 的 slot；仅清零密钥不能代表“禁用”。
- 同 hash 搜索顺序固定为动态 Team、Join、用户 slot。同时把解密候选数从 4 扩到至少 6，
  确保“原有最多 4 个候选 + 两个系统 slot”不因一字节 hash 冲突丢失。
- Leader 创建 Team 或 Member 完成 `JOIN_DECISION` 验证后，将动态 Team Credentials
  写入最后一个 slot。
- `DataStore::loadChannels()` 完成后再安装两个系统 slot，避免存储内容替换系统配置。
- `DataStore::saveChannels()` 在系统 slot 前结束，不把 Join 或 Team Credentials 写入
  `/channels2`。
- `CMD_GET_CHANNEL` 和 `CMD_SET_CHANNEL` 对两个系统 slot 返回参数错误，不暴露密钥。
- UI 可以通过 facade 看到当前 Team，但 Join Group 和两个 slot 索引均不可见；
  `sendMessageGroupAt()` 只额外显示当前已激活 Team。

安装/重置 Team 必须在 Mesh 主任务中串行执行：停止 Team 调度、持久化新凭据、清除旧
Team slot 与成员表、安装新 Team slot、重新启动调度。过程中 UI 只收到一次最终
`TeamChanged` 事件。持久化失败时恢复旧凭据和运行状态，不允许半成功。

### 5.4 密钥生命周期和安全边界

动态 Team Key 提供频道隔离，但不自动解决退队后的密钥撤销：

- 已获批成员可能从自身 Flash 中提取当前 Team Key。
- 持有密钥的设备可伪造 `sender_id` 和显示名称。
- Group Text 的 sender name 本身未经验证。
- Leader 拒绝申请只能防止未获得密钥的新设备加入。已获批 Member 被“移除”后，
  必须重置 Team Key 并让保留成员重新加入，否则它仍能解密后续群组数据。

V1 产品文档必须明确其适用于可信设备和临时队伍。如果后续要求防止伪造，需要增加设备白名单或签名机制，不在本次范围内。

### 5.5 威胁模型与隐私提示

- Group 只提供共享密钥的机密性和短 MAC 完整性，不提供成员级身份认证、前向保密
  或密钥泄露后的历史保护。
- 内置 Join Group Credentials 对所有量产设备相同且可被固件提取，只能降低无关流量，
  不能证明申请者是 Heltec 设备或被授权成员。入队认证依赖人工核对配对确认码。
- 配对码不一致时必须拒绝；跳过核对等同于接受中间人攻击风险。V1 复用长期身份密钥完成
  ECDH，因此不提供加入过程的前向保密，这一限制必须进入产品安全说明。
- 当前 Group MAC 仅 2 字节，高频伪造尝试不能仅靠 MAC 长度防御。发送调度、解码错误日志
  和 Sync 响应都必须限速。
- 位置是敏感数据。加入 Team 表示向组内共享位置；停止共享需要退出/关闭 Team。
  这不能撤回其他设备已收到的历史位置，用户手册需明示。
- 活跃 Team 在 GPS 总开关允许时申请供电，不受旧 Advert 开关及周期影响；
  缺少 GPS 或无有效 fix 时仍发 Presence，UI 显示“无位置”。

## 6. Team 协议

### 6.1 字节序和通用规则

- 所有 16 位和 32 位整数使用 Little Endian，与 MeshCore 文档一致。
- 不直接 `memcpy` 整个 C++ struct 上线，避免 padding、ABI 和端序问题。
- 所有编码函数接收输出缓冲区长度并返回实际长度或失败。
- 所有解码函数先检查剩余长度，再读取字段。
- 对 V1 已知 subtype 要求长度精确匹配，不接受尾随未定义字节；扩展字段必须升级
  `protocol_version` 或定义新 subtype。
- 未识别的协议版本或子类型直接忽略，但仍允许 MeshCore 完成正常去重和路由。
- 坐标沿用项目现有 `1e-6` 度格式，避免 Contact/Advert/Find Friend 之间反复换算。
- 名称不允许内嵌 NUL 或控制字符；接收端必须验证 UTF-8，非法序列拒绝整帧。
  UI 按字符边界截断，不得显示半个多字节字符。

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

`sender_id` 使用 6 字节是为了与现有 `sendDirectMessage()` 和 UI 公钥前缀保持一致。启动会话的键为：

```text
sender_id + boot_id
```

`boot_id` 必须由 MeshCore RNG 在每次启动生成，不从 `millis()`、RTC 或固定设备字段派生。
sequence 初值同样使用随机 16 位值，然后每个实际入队的 Team frame 递增一次；编码或入队
失败不消耗 sequence。

成员表以 `sender_id` 作稳定索引，`boot_id` 只标识当前启动会话。同一
`sender_id` 出现不同 `boot_id` 时更新原成员项并重置 sequence 窗口，不新建第二个
成员。六字节前缀冲突时 V1 无法可靠区分，UI 应附加短 ID 便于诊断。

通用 flags：

```text
bit 0: time_valid
bit 1: battery_valid
bit 2: urgent
bit 3: reserved, sender must write 0
bit 4..7: subtype-specific
```

Presence 和 Sync Request 的 bit 4..7 必须写 0；Location 按 6.4 解释。解码器忽略未识别位，
但 V1 编码器不得主动置位。

### 6.3 `TEAM_PRESENCE`，子类型 `0x01`

公共头之后：

| 字段 | 长度 | 说明 |
|---|---:|---|
| `device_type` | 1 | `1=T1`，`2=V4_R8_TFT`，其他值预留 |
| `public_key` | 32 | 原生 Direct Message 所需的完整公钥 |
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
- 发布用于单独消息的完整公钥；必须验证 `sender_id` 等于 `public_key` 的前 6 字节。

Presence 是建立启动会话的唯一帧。未见过该 `sender_id` 的 Presence 时，不为其 Location
创建成员；已知成员的 Location 若带有不同 `boot_id` 也忽略，直到新 Presence 建立新会话。
这可避免延迟的旧包在两个 boot ID 之间反复切换成员状态。

发送条件：

- Team 开启或 Team Key 更换后，随机延迟 `0`～`5000 ms`。
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
加上 Group Data 的 3 字节封装后是 42 字节；按当前 16 字节加密块、1 字节
channel hash 和 2 字节 MAC 计算，Mesh payload 预计为 51 字节。单元测试必须以
`createGroupDatagram()` 的实际返回长度锁定该预算，防止加密实现变更后静默超包。

坐标验证：

```text
-90,000,000 <= latitude_e6 <= 90,000,000
-180,000,000 <= longitude_e6 <= 180,000,000
```

状态变化规则：

- 从有效 fix 变为无效时发送一帧 `location_valid=0`，之后停止重复发送无效状态。
- 从无效恢复有效时立即发送。
- 接收无效位置时保留成员，但将其位置标记为不可用于导航。
- `location_valid=0` 时坐标、高度、精度、速度、航向和卫星数全部写 0，防止旧位置
  被误用或无意泄露。
- `altitude_valid=0` 时 `altitude_dm=0`；`motion_valid=0` 时 `speed_cms=0`、
  `course_cdeg=0`。
- `fix_age_sec` 指发布者在编码时距最后有效 fix 的时长，接收端展示位置年龄时使用
  `fix_age_sec + elapsed_since_receive`，两项均做饱和运算。

当前仓库的 `StableGpsFixSnapshot` 可直接提供经纬度、高度、卫星数和 fix age，
`GpsRmcSpeedStream` 可提供速度，但没有统一的 course 和精度来源。V1 数据映射为：

- 未接入真实精度（例如 HDOP 到米的明确换算）前，`accuracy_dm=0`。
- 只有速度和 course 都有效且属于同一个新鲜 fix 时才置 `motion_valid=1`；否则两字段均为 0。
- 高度从米/毫米转为分米时先在 32 位中间值中计算，再饱和到 `int16_t`；饱和发生时
  清除 `altitude_valid`，不把截断值当成真实高度。
- 速度、course、电压和 fix age 的浮点/宽整数转换都在转为窄类型前做有限值和
  范围检查。

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

当同时请求 Presence 和 Location 时，延迟取模的上限要预留 1000 ms，保证第二帧仍在
`response_window_sec` 内入队。当窗口即将结束或队列繁忙时，优先 Presence；Location
可合并到下一个正常位置发送，不得为赶窗口抢占 Text 优先级。

补充规则：

- 忽略自己发出的 Sync Request。
- `response_window_sec` 不在 5～30 时直接拒绝，不在接收端静默修正。
- `response_mask` 的 bit 2..7 忽略；bit 0..1 均为 0 时不调度任何响应。
- 当已有更早的 Presence/Location 待发时，同步响应与其合并，队列中每种子类型
  最多保留一帧。
- 每个请求者保留最近 8 个 `(boot_id, request_id)`，至少 10 分钟后才淘汰；另设
  每设备 10 秒的最小响应间隔，避免持有组密钥的异常节点持续触发发送。

队长进入 Team 页面时不要每次都请求同步。仅在成员表为空、用户手动刷新或距离上次请求超过 60 秒时发送。

### 6.6 Join Protocol 线格式和包长

Join 帧不使用 6.2 的 Team 公共头；它们拥有独立的最小头，避免尚未入队的设备伪造 Team
运行时状态。所有多字节整数仍使用 Little Endian，所有保留字段发送为 0。

```text
JOIN_DISCOVER (0x10)
version:u8 | subtype:u8 | request_id:u32

JOIN_OFFER (0x11)
version:u8 | subtype:u8 | request_id:u32 | session_id:u64 | team_id:u64 |
leader_public_key:32 | leader_nonce:16 | team_name_len:u8 | team_name:N

JOIN_REQUEST (0x12)
version:u8 | subtype:u8 | session_id:u64 | member_public_key:32 |
member_nonce:16 | member_name_len:u8 | member_name:N

JOIN_DECISION (0x13)
version:u8 | subtype:u8 | session_id:u64 | aead_nonce:12 |
ciphertext_len:u8 | ciphertext:N | tag:16

JOIN_ACK (0x14)
version:u8 | subtype:u8 | session_id:u64 | aead_nonce:12 |
ciphertext_len:u8 | ciphertext:N | tag:16
```

- `request_id` 为 Member 每次搜索随机生成的 32 位值；Leader 的 Offer 必须原样带回。
- `session_id` 为 Leader 每个 Offer 随机生成的 64 位值，在配对窗口内不得重复。
- Team/Member 名称最大 31 字节，并执行与 6.1 相同的 UTF-8 和控制字符检查。
- Approve 明文包含 decision、`team_id`、`key_version`、32 字节 Team Key、Team 名称和
  Leader 公钥；Reject 明文只包含 decision。ACK 明文包含结果和 `key_version`。
- `ciphertext_len` 必须与 subtype 对应的精确明文长度一致；认证通过前不得解析、显示或持久化。
- 按 31 字节最长名称和 16 字节 tag 计算，Approve 明文最大 109 字节，Join Data 最大
  148 字节，低于 `MAX_GROUP_DATA_LENGTH=165`；加上 Group Data 头并经当前 16 字节分组
  加密后的 Mesh payload 预计为 163 字节，低于 `MAX_PACKET_PAYLOAD=184`。单元测试必须通过
  真实编码器锁定这两个最大长度；超出时构建失败，不允许运行时静默截断名称或认证标签。

## 7. 有限跳数设计

### 7.1 必须满足的原则

Team 子类型和 `data_type` 位于 Group 密文中。没有 Team 密钥的 Repeater 无法读取，因此：

> 禁止只把 `max_hops` 放入 `TEAM_LOCATION` 加密正文，然后声称 Repeater 会执行它。

`max_hops` 必须位于 Repeater 可读取的 Mesh 路由元数据中，并由 `Mesh::routeRecvPacket()` 在追加本机 path hash 前检查。

本协议把 hop 定义为“从发送端开始的无线链路次数”，不是 path 中已记录的中继数。
因此发送端首次发射已经是第 1 hop，接收时：

```text
received_hops = packet->getPathHashCount() + 1
forward only when received_hops < max_hops
```

`max_hops` 的有效范围为 1～15；1 表示只到直接邻居，2 表示允许一个 Repeater 再转发一次。
到达上限的节点仍可解密和消费，但返回 `ACTION_RELEASE`。不定义
`max_hops=0`，需要本地回环时由上层直接处理。

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
bool sendFloodLimited(Packet *packet,
                      uint8_t max_hops,
                      uint8_t priority,
                      uint32_t delay_millis,
                      uint8_t path_hash_size);

bool Packet::hasFloodHopLimit() const;
uint8_t Packet::getFloodHopLimit() const;
```

`Mesh::routeRecvPacket()` 的检查顺序：

1. 必须是 Flood。
2. 未标记 `DoNotRetransmit`。
3. 若有 hop limit，确认 `path_hash_count + 1 < max_hops`。
4. 未超过 path 缓冲区。
5. `allowPacketForward()` 返回 true。
6. 追加本机 hash 并排队转发。

`sendFloodLimited()` 对空指针、非法 hop 和非法 path hash size 返回 false，并负责释放已取得的
packet，调用方不得再释放。该所有权约定要在头文件注释中写明并测试。

### 7.3 默认跳数

| 数据 | 默认 `max_hops` |
|---|---:|
| Presence | 2 |
| Location | 2 |
| Sync Request | 2 |
| 原生 Team Group Text | 3 |

为了让原生 Group Text 也受限，TeamService/消息 facade 在识别到系统 Team channel 时必须走“有限跳数 Group Text”发送入口；普通 Public/user channels 保持原有 Flood 行为。

上表按“无线链路次数”理解：Location 可到达直接邻居及经一个已升级 Repeater
的节点；Team Text 可再多经一个 Repeater。

### 7.4 兼容限制

- 未升级的 Repeater 会忽略新 hop limit，仍可能继续 Flood。
- 因此“严格限制两跳”只有在传播路径中的 Repeater 全部升级后成立。
- 新 Team endpoint 必须仍能接收旧的普通 Group Text。
- 如果上游扩展改变 Packet wire format，必须增加显式版本兼容测试，不能直接提升全局 `PAYLOAD_VER` 并导致旧包全部被拒绝。
- 当前 `transport_codes[0]` 已被 Region/Scope 使用，`transport_codes[1]` 也有 Region 相关预留语义；
  未经上游确认不得在量产包中占用任何一个字段。
- 上游未确定时可在 `HELTEC_TEAM_HOP_LIMIT_POC` 宏下做实验编码；量产构建必须对该宏
  使用编译期 `#error`。

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
    uint8_t public_key[32];
    uint16_t boot_id;
    uint16_t last_presence_sequence;
    uint16_t last_location_sequence;
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

`TEAM_SYNC_REQUEST` 的 sequence 不写入成员项，由请求去重缓存管理。Presence 和 Location
必须分别维护 sequence；若只用一个全局 `last_sequence`，后到的 Presence 会使稍早但仍有效的
Location 被错误丢弃。

分配策略：

- 相同 sender ID 更新原项。
- 表未满时使用空项。
- 表满时淘汰最久未见且不是当前 UI 选中目标的成员。
- Team Leader 不因普通成员更新而被优先淘汰。
- 不将成员表周期性写入 Flash。
- 本机不通过无线回环包加入成员表；facade 在生成快照时可把本机状态合并为一个
  `is_self=true` 的条目。
- 表满且没有可淘汰项时丢弃新成员，递增可观测计数器，不覆盖当前选中成员、
  本机或 Leader。

### 8.2 在线和位置新鲜度

```text
Online:  last_seen_age <= 20 分钟
Aging:   20 分钟 < last_seen_age <= 30 分钟
Offline: last_seen_age > 30 分钟
```

该门限与 15 分钟 Presence 及其抖动配套，不与 30/180 秒 Location 周期混用。
Location 是否可用另由 `location_valid`、`fix_age_sec` 和本地接收时间判断；成员 Online
不代表位置 Fresh。

导航位置新鲜度沿用现有 `FindFriendNavEstimator` 的 Fresh/Aging/Stale 模型。数据源从 Contact receipt 改为 Team member snapshot，但不重写距离、方位和平滑算法。

### 8.3 去重和乱序

MeshCore 已按 packet hash 去重，但 TeamService 仍需做语义去重：

- boot ID 相同：Presence 和 Location 分别与各自的 last sequence 比较。
- 仅 Presence 可以建立或切换 boot ID；切换后重置两个 sequence 窗口和该成员的旧位置。
- 相同 Sync Request 只响应一次。
- timestamp 仅用于展示，不作为唯一的乱序依据。
- 本机 RTC/GPS 时间无效时允许 timestamp 为 0。

16 位序列号的唯一比较公式为：

```cpp
bool is_sequence_newer(uint16_t incoming, uint16_t current)
{
    const uint16_t delta = static_cast<uint16_t>(incoming - current);
    return delta != 0U && delta < 0x8000U;
}
```

差值恰为 `0x8000` 时按旧包处理。每个 subtype 在新 boot 会话的首帧使用独立
`has_sequence` 标志，不可假设首个 sequence 必须为 0。所有基于 `millis()` 的 deadline 都使用仓库现有
wrap-safe 比较工具，必须覆盖 32 位毫秒回绕测试。

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
- Team Key 更换和同步响应必须随机错峰。
- 发送队列繁忙或 Radio budget 不足时延后位置包，不能挤占文本消息。
- 同一设备队列中只保留最新的待发 Location；新位置覆盖尚未发送的旧位置。
- Location 不重发、不等待 ACK。
- Text 优先级高于 Presence 和 Location。
- Sync Request 最短间隔 60 秒。
- 不使用 `GPS_NOTIFY_INTERVAL_SEC=1` 作为无线发送周期；该值只允许驱动本地 UI/GPS 更新。

不要先创建 `Packet` 再长时间留在 Dispatcher 队列中。`TeamService` 只保存一份待发的结构化
Location，到 deadline 且队列有余量时才编码和申请 packet pool。申请或入队失败时：

1. 保留最新状态，使用 1～5 秒有界退避重试。
2. 最多连续重试 3 次，之后等待下一个正常周期；状态变化帧不受此限制。
3. 递增 `team_tx_dropped`/`team_tx_deferred` 计数器，不打印每包日志。

当前 `sendFlood()` 会把普通 Group Text 和 Group Data 都以优先级 1 入队，因此仅在文档中
声明“Text 优先”不会生效。有限 Flood API 必须允许调用方指定优先级（数值越小越高）：

| 数据 | 本机入队优先级 |
|---|---:|
| Team Group Text | 1 |
| Sync Request | 3 |
| Location | 4 |
| Presence | 5 |

转发优先级仍按 `Mesh::routeRecvPacket()` 的 path distance 规则，不携带发送端的本地队列优先级。
为了让上层区分“已入队”和“队列已满且 packet 已释放”，需将
`PacketManager::queueOutbound()` -> `Dispatcher::sendPacket()` -> `sendFloodLimited()` 的结果逐层
传为 bool，并修改所有 PacketManager 实现。这项通用 API 变更必须单独提交，不与 UI 改动混在一个
commit 中。

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

1. Team 未开启时立即忽略。
2. `data_type` 不是 Heltec Team 时交给现有/未来其他处理器或忽略。
3. 以完整 32 字节 secret 或保留 slot 索引确认 channel 是当前 Team，不得只比较
   1 字节 hash。
4. 调用 `TeamProtocol::decode()`，失败时递增按错误类别聚合的计数器。
5. 调用 `TeamService::handleFrame()`。
6. 成员或位置发生可见变化时发送 `AppStateEventType::TeamChanged`。
7. 不把 Team Presence 自动写入全局 Contact 表。

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

停止主循环调用旧 `pollLocShareAdvert()`，旧 `loc_share_enabled` 偏好不再生效：

```text
if Team enabled:
    TeamService schedules Presence and Location
else:
    do not transmit Team location or legacy Location Share Advert
```

`createPolicySelfAdvert()`、手动 `advert()` 和普通联系人发现继续保留，确保 Companion 和旧设备兼容。
持有 Team 凭据时，普通身份 Advert 不携带坐标。退出后也不恢复旧位置共享定时器；
MeshCore 独立配置的 `advert_loc_policy` 不属于旧 Location Share 开关，保留其非 Team 行为。

### 10.4 生命周期与线程边界

- `TeamService::begin()` 在 identity、prefs、channels 和 GPS provider 就绪后调用，不在全局构造期访问它们。
- `poll()` 只由 `HeltecMesh::loop()` 所在任务调用；无线接收、配置切换和发送调度在同一
  上下文中串行化。
- UI 不持有 `TeamMember` 指针。facade 使用固定大小 DTO 复制快照，并带一个递增
  `generation` 供 UI 判断是否需要重绘。
- GPS callback 只更新本地 snapshot/脏标志，不在 callback 内编码、发包或调用 LVGL。
- Team 关闭、更换 Team Key 或开始 OTA 时取消所有待发 Team deadline，但不释放非 Team packet。

## 11. 临时 Team 生命周期

Team 仅存在于 RAM，不序列化凭据、队名、Team Key、成员表、加入会话或启用状态。

- 启动始终为未组队；队长需重新创建 Team，队员需重新申请加入。
- 队员重启不影响其他在线设备的 Team；队长重启后新建的是新 Team，其他成员需退出旧队后重新加入。
- 退出 Team 清零本机凭据与成员运行态；不改动普通频道、设备身份及其他设置。
- 删除 Team 配置读写、CRC 和文件替换逻辑，减少 Flash 代码、栈缓存和写入次数。
- 兼容旧版本时仅尝试删除设备文件系统中的 `/team_prefs`、`/team_prefs.tmp`、
  `/team_prefs.bak`，不读取它们。删除失败只记录日志，不阻止临时组队。
- 上述删除不是物理安全擦除；旧 Flash 页可能存在可取证残留。
- Team 保留频道不进入普通频道序列化。

## 12. UI 和业务 facade

### 12.1 通用 facade

在 `biz_facade.hpp` 增加只读 DTO 和操作接口，不暴露 `TeamMember` 内部指针：

```text
TeamUiSnapshot
TeamMemberUiItem
teamEnabled()/setTeamEnabled()
teamMemberCount()/fillTeamMembers()
requestTeamSync()
createTeam()/resetTeamKey()/leaveTeam()
openJoinWindow()/closeJoinWindow()
discoverTeams()/selectTeamOffer()/cancelJoinRequest()
pendingJoinRequest()/approveJoinRequest()/rejectJoinRequest()
```

在 `AppStateEventType` 增加：

```text
TeamChanged
TeamJoinChanged
```

Team member 位置变化只发送 invalidation event，screen 在刷新时从 facade 获取新快照，沿用当前 AppState 的模式。
多个位置包在短时间到达时合并 UI invalidation，最多每 200 ms 触发一次重绘；
紧急标志、选中成员离线和 Team 切换不受该节流限制。

### 12.2 T1

复用现有 Find Friend 页面和 `FindFriendNavEstimator`：

- Find Friend 的 Contact 模式改为 Team Member 模式。
- 目标列表来源改为当前 Team 成员表。
- 列出所有已收到 Presence 的在线/近期 Team 成员；没有可用位置时标记“no location”，
  超过在线门限时标记“offline”，但不得伪装为可导航目标。
- 默认优先选择 role 为 Leader 的成员。
- 保存选择时使用 6 字节 sender ID，而不是 Contact 数组索引。
- Waypoint 模式保持不变。
- 未加入时显示“Join Team”，点击后主动查找当前处于配对窗口的 Leader。
- 搜索结果显示 Team 名称和 Leader 短指纹；选中后显示 4 位配对确认码以及
  “请将此数字告诉队长”，随后等待 Leader 确认、接收密钥并显示完成状态。
- 已加入时显示 Team 名称和短指纹，提供退出操作。
- 增加消息入口，可选“All Teammates”或单个 Team Member。

不要删除 Contact Location 代码；它仍用于 Team 关闭时的旧功能兼容。
同时出现多个 Leader 时全部保留 Leader 标识；T1 默认选择 `last_seen` 最新者，
再以 `sender_id` 字节序升序打破平局。V1 不做 Leader 选举或隐式降级。

### 12.3 V4 R8 TFT Leader 可视化

当 V4 R8 TFT 构建为 Leader 时，Tracker Screen 提供成员地图和审批界面。这是该型号的
可视化能力，不是 Leader 协议对设备型号的限制。V4 R8 TFT 当前排除了
`find_friend_screen.cpp`，不能简单打开 T1 单目标罗盘页面。新增 TFT Team 页面，建议文件：

```text
src/heltec/ui/screens/team_map_screen.hpp
src/heltec/ui/screens/team_map_screen.cpp
```

V1 UI 最小范围：

- 无 Team 时显示“Create Team”；已有 Team 时显示 Team 名称、指纹和密钥版本。
- “Open Joining”打开 120 秒配对窗口，允许任何兼容 Member 申请，显示倒计时和
  “Stop Joining”；关闭时不响应发现请求。
- 收到申请后弹出确认框，显示 Member 名称、公钥短指纹和 4 位配对确认码，提示队长先与
  Member 屏幕核对数字，再选择“Agree”或“Reject”；30 秒未操作按 Reject 处理。
- 多个待审请求排队显示，但一次只弹一个框，不覆盖当前弹窗。
- 成员列表：名称、角色、在线状态、位置年龄、电量。
- 地图显示所有拥有有效位置的成员。
- Leader 使用不同图标。
- 点击成员后显示经纬度、距离和最后更新时间。
- Message 按钮打开现有发送消息 overlay，可选全队或单个成员。
- 手动 Refresh 按钮发送 Sync Request，并具有 60 秒冷却。
- Reset Team Key 要二次确认，并明示“所有 Member 必须重新加入”。

不要在 V1 实现聊天气泡、历史会话数据库或二进制文件 UI。
TFT 页面复用现有 `MapPanel` 和 `MapPlotMarker` DTO，不引入第二套地图引擎。现有上限为
40 个 marker，足以容纳 32 个 Team 成员加本机；测试必须覆盖 33 个可见 marker。

## 13. 全队消息和单独消息

### 13.1 全队消息

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
- 显示来源建议为 `<Team name> / sender`，不要只显示 `Unknown`。
- Team Group Text 使用默认 3 hops 的有限 Flood 发送入口。
- 普通 Group channel 行为不变。

### 13.2 单独发给某个队友

“单独消息”默认按真正的一对一保密语义实现，不使用“Group 密文 + recipient ID +
其他人 UI 不显示”的伪私聊方案。发送时复用现有 Direct Message：

```text
All Teammates -> sendGroupMessage(active_team_channel, text)
specific TeamMember -> sendMessage(transient_team_peer, text)
```

为了不污染普通 Contact 表，新增固定容量的运行时 `TeamPeer` 目录，与 `TeamMember`
使用同一 sender ID 索引。它保存完整公钥、最后可用 path、最后在线时间和 Direct Message
所需的短期状态，但不写入 contacts 文件。

集成要求：

- 不能只修改 `searchPeersByHash()`；当前 `getPeerSharedSecret()`、`onPeerDataRecv()`、
  `onPeerPathRecv()` 和 ACK 跟踪都假定匹配项来自 Contact 数组。
- 先在 `BaseChatMesh` 增加可选的 transient peer hook/适配层，让公钥匹配、shared secret、
  收包分发、路径更新和 ACK 跟踪可以安全定位到 Contact 或 TeamPeer。
- `searchPeersByHash()` 先保持现有 Contact 搜索，再通过上述适配层搜索当前 TeamPeer，
  使队友能接收 Direct Message。
- 通过 Presence 的完整公钥创建 TeamPeer，并验证公钥前 6 字节与 `sender_id` 一致。
- 已知路径时按 Direct 发送；路径未知时沿用 MeshCore 的加密 Flood Direct 行为。
- ACK、超时、失败提示和离线队列沿用现有 Direct Message 语义。
- Team 退出、重置密钥或换队时清空 TeamPeer，不删除同公钥的普通 Contact。
- 如果同一公钥同时存在于 Contact 和 TeamPeer，复用 Contact 的成熟路径，不建立第二份
  ACK 状态。

## 14. 构建配置

新增功能宏：

```text
HELTEC_TEAM_FEATURE=1
HELTEC_TEAM_ROLE=0
HELTEC_TEAM_MAX_MEMBERS=32
HELTEC_TEAM_JOIN_CHANNEL_INDEX=(MAX_GROUP_CHANNELS-2)
HELTEC_TEAM_CHANNEL_INDEX=(MAX_GROUP_CHANNELS-1)
HELTEC_TEAM_USER_CHANNEL_CAPACITY=(MAX_GROUP_CHANNELS-2)
HELTEC_TEAM_JOIN_MAX_HOPS=1
HELTEC_TEAM_JOIN_WINDOW_SEC=120
HELTEC_TEAM_JOIN_CONFIRM_SEC=30
HELTEC_TEAM_JOIN_MAX_SESSIONS=4
HELTEC_TEAM_LOCATION_MAX_HOPS=2
HELTEC_TEAM_TEXT_MAX_HOPS=3
```

公共头文件中提供默认和范围检查：

```cpp
#ifndef HELTEC_TEAM_ROLE
#define HELTEC_TEAM_ROLE 0
#endif

static_assert(HELTEC_TEAM_ROLE == 0 || HELTEC_TEAM_ROLE == 1,
              "HELTEC_TEAM_ROLE must be 0 (Member) or 1 (Leader)");
```

仅在以下目标启用：

- `variants/heltec_t1/platformio.ini` 的 companion radio USB/BLE 环境。
- `variants/heltec_v4_r8/platformio.ini` 的 TFT companion radio USB/BLE/Wi-Fi 环境。

T1 不需要额外角色 build flag，使用默认 `HELTEC_TEAM_ROLE=0`。V4 R8 TFT 的三个
Team companion 环境通过共享 PlatformIO section 追加：

```ini
-D HELTEC_TEAM_ROLE=1
```

该宏是产品角色配置，不以 `HELTEC_V4_R8_TFT` 等板型宏推导。同一块硬件未来如果有
Member SKU，可通过不同构建环境使用默认值 0。

第一阶段不要在 repeater、room server、sensor 和 KISS modem 环境启用 Team UI/Service。通用 hop-limit 路由支持可以在所有 MeshCore 节点编译，以便 Repeater 执行转发限制。

Team 宏只加在上述具体 `[env:...]` 中，不加在被 repeater/room server 共用的基础 section。
对 Team 环境将原频道容量扩容两项（T1 34，V4 R8 TFT 42），并增加编译期检查：

```cpp
static_assert(HELTEC_TEAM_JOIN_CHANNEL_INDEX < MAX_GROUP_CHANNELS,
              "Join channel slot is out of range");
static_assert(HELTEC_TEAM_CHANNEL_INDEX < MAX_GROUP_CHANNELS,
              "Team channel slot is out of range");
static_assert(HELTEC_TEAM_JOIN_CHANNEL_INDEX + 1 == HELTEC_TEAM_CHANNEL_INDEX,
              "System channel slots must be contiguous");
static_assert(HELTEC_TEAM_USER_CHANNEL_CAPACITY + 2 == MAX_GROUP_CHANNELS,
              "Team channel capacity mismatch");
```

## 15. 分阶段实施清单

阶段依赖为 `A -> C -> D -> E -> F/G`；B 可与 A/C 并行，但 D 只能在一跳
`max_hops=1` 的本地验证配置下进行，不得把未批准的 POC hop encoding 默认开启。
E～G 合并到发布分支的前置门禁是 B 的 wire encoding 获得上游结论。H 是量产发布门禁，
不阻塞使用测试编号和测试密钥的内部 POC。

### 阶段 A：协议和主机测试

1. 新建 `team_protocol.hpp/.cpp`。
2. 定义开发 data type 和三个 subtype。
3. 实现小端读写、Presence/Location/Sync 编解码。
4. 新建 `test/test_team_protocol/test_team_protocol.cpp`。
5. 覆盖正常、截断、超长名称、非法坐标、未知版本、未知 subtype。
6. 以仓库现有主机测试框架添加 corpus/fuzz 入口，对 0～165 字节任意输入确保不越界。

完成标准：协议测试在无 Arduino/LVGL 环境下通过，没有 packed struct。

### 阶段 B：通用 Flood hop limit

1. 与 MeshCore 上游确认 wire encoding。
2. 在 `Packet` 中增加 hop-limit 查询接口。
3. 增加有限 Flood 发送 API。
4. 修改 `Mesh::routeRecvPacket()`。
5. 添加 1、2、3、15、非法 0/16 和无 hop limit 的路由测试。
6. 验证 Direct packet 不受影响。

完成标准：2-hop packet 由发送端发到直接邻居，直接邻居只转发一次；
第二条无线链路后的接收节点可消费但不再入队。

### 阶段 C：Team Credentials 和 Join Protocol

1. 实现 Leader 随机生成 Team ID、Team Key 和 key version。
2. 实现版本化 `/team_prefs`、完整性校验和掉电安全替换。
3. 实现隐藏 Join Group slot 和动态 Team Group slot，阻止 Companion 读写或落盘。
4. 复用现有身份 ECDH，选择并集成经审查的 HKDF-SHA-256 和 AEAD 实现，完成两平台
   测试向量、性能、栈空间和代码尺寸测量。
5. 实现 5 个 Join subtype、120 秒配对窗口、4 位配对码、发现/申请限速和最多 4 个待审请求。
6. 实现 Leader Agree/Reject 和 Member `JOIN_ACK` 状态机。

完成标准：没有 Leader 明确同意时无法获得 Team Key；同意后 Member 重启仍可加载凭据；
拒绝、超时、配对码不一致、重放和掉电都不会安装部分或未验证密钥；抓包中没有配对码或
Team Key 明文。

### 阶段 D：TeamService

1. 实现成员表。
2. 实现 boot ID、sequence 和去重。
3. 实现三个 frame handler。
4. 实现位置调度、移动阈值和抖动。
5. 实现 Sync Response 错峰。
6. 接入 `onChannelDataRecv()`。
7. 补齐成员表快照、计数器和 millis 回绕测试。

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
3. 增加查找队伍、选择 Team、显示配对确认码、加入进度、取消和退出操作。
4. 增加全队/单人消息目标选择。

完成标准：T1 可主动发现并选择处于配对窗口的 Leader，显示与 Leader 相同的配对码；
获批后显示距离、方向、新鲜度，
并发送全队或单人消息。

### 阶段 G：V4 R8 TFT UI

1. 新建 Team map screen。
2. 显示成员列表和地图标记。
3. 增加创建 Team、配对窗口、含配对码的申请队列、Agree/Reject 弹窗和重置密钥。
4. 增加刷新以及全队/单人消息。
5. 适配触摸、弹窗排队和页面生命周期。

完成标准：至少 10 个模拟成员可同时显示，进入退出页面无内存增长。

### 阶段 H：正式编号和量产审核

1. 展示可工作的 POC。
2. 向 MeshCore 申请正式 Group Data type。
3. 更新 `docs/number_allocations.md`。
4. 完成 ECDH/HKDF/AEAD 组合、转录编码和配对码计算的密码学审查及测试向量验证。
5. 替换开发编号，再做跨版本互通测试。

## 16. 测试矩阵

### 16.1 协议测试

- 三种 frame encode/decode round trip。
- 每个长度边界：0、最小、最大、超长、截断。
- sequence 回绕：`65535 -> 0`。
- sequence 差值为 `0`、`0x7FFF`、`0x8000`、`0x8001` 的比较边界。
- boot ID 改变后接受低 sequence。
- 新 boot 的 Location 早于 Presence 到达时丢弃，Presence 到达后的 Location 接受。
- Presence 和 Location 乱序时各自的 sequence 窗口互不影响。
- 非法纬度、经度、course 和 name length。
- 内嵌 NUL、控制字符、无效 UTF-8 和多字节边界截断。
- 未知 flags 可忽略，V1 encoder 始终将保留位写 0。
- `location_valid=0` 时所有位置/运动字段为 0。
- 每种已知 subtype 的精确长度及尾随字节拒绝。

### 16.2 创建 Team 和入队测试

- 两次创建生成不同 Team ID 和 Team Key；配对码不能直接推导 Team Key 或会话密钥。
- 配对窗口关闭时 Leader 不响应；打开后 Member 可发现 Team，120 秒到期或手动关闭后停止响应。
- 双方对相同转录计算出完全相同且保留前导零的 4 位配对码；任一公钥、nonce、session ID
  或 Team ID 变化时，测试向量中的转录 hash、会话密钥和配对码随之变化。
- 只有 Leader 核对配对码并点击 Agree 后 Member 才能安装密钥；Reject、超时和取消都不安装。
- 篡改 Offer/Request、篡改 AEAD 密文或 AAD、nonce 重复、旧会话重放和跨会话 Decision 注入。
- 相同请求重发不重复弹窗；4 个待审请求、第 5 个请求、弹窗排队、30 秒自动拒绝、
  每公钥和全局发现限速均按定义工作。
- `JOIN_DECISION` 下发后 Member 掉电，Leader 显示 unconfirmed；Member 不得加载部分写入的密钥。
- Member 安装密钥后与 Leader 互通，其他 Team Key 不产生 Team/UI 更新。
- Reset Team Key 后旧 Member 不能解密新包，重新获批后恢复互通。
- Team 关闭后不发送 Presence/Location；隐藏 Join Group 和用户自建 channel 不受影响。
- 从旧 32/40 项 `/channels2` 升级后用户频道顺序、名称和密钥完全不变。
- Join/Team slot 无法通过 Companion GET/SET 读取或覆盖，且不会写入 `/channels2`。
- Team 关闭时零密钥 slot 不会命中 `searchChannelsByHash()`。

### 16.3 消息测试

- All Teammates 使用 Group Text，所有当前 Team 成员可接收。
- 指定 TeamPeer 使用 Direct Message，其他成员不能解密消息。
- Contact 与 TeamPeer 公钥相同时只有一份 ACK/超时状态。
- Direct path 已知、未知、失效和重建路径。
- Team 退出/重置后 TeamPeer 被清空，同公钥普通 Contact 保留。

### 16.4 路由测试

- `max_hops=0` 和 16：发送 API 拒绝，不入队且无内存泄漏。
- `max_hops=1`：直接邻居消费，不继续转发。
- `max_hops=2`：直接邻居转发，第二条无线链路后的节点消费但不再转发。
- `max_hops=3`：类推，并校验 path hash count 每次只增加 1。
- 无 hop limit 的旧 Group packet 保持现有行为。
- 旧 Repeater 存在时记录其继续 Flood 的兼容限制。

### 16.5 GPS 测试

- 冷启动无 fix。
- 首次 fix。
- 静止 30 分钟。
- 连续移动。
- fix 丢失和恢复。
- GPS 时间无效但坐标有效。
- 经纬度边界和零坐标。
- GPS 关闭但 Team 仍开启：只有 Presence，不自动打开 GPS。
- 退出/关闭 Team：后续不再发送 Group 位置，也不恢复旧 Location Share Advert。
- `fix_age_sec` 与接收后经过时间正确累加并饱和。

### 16.6 负载测试

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
- Presence、Location、Sync Request 和 Team Group Text 的本地入队顺序与延迟分布。
- packet pool 耗尽和 outbound queue 满时的延迟/丢弃计数。

验收目标：

- 正常 30 秒移动周期下不发生持续 outbound queue 堆积。
- Group Text 不被周期 Location 长时间阻塞。
- 同步请求后响应分布在配置窗口内，不出现集中发送峰值。
- 负载停止后，outbound queue 在两个静止 Location 周期内回到基线，无单调增长。
- 在相同射频参数和节点拓扑下，Team Group Text 的 p95 首次发射等待时间不高于
  关闭 Team Location 基线的 1.25 倍。
- 除了无线冲突或队列明确丢包，90% 以上的在线成员在 `response_window_sec`
  加一个最大单包 airtime 内完成同步响应。
- 按目标地区和 LoRa 配置校验 duty-cycle/airtime budget；超限时通过增大 Location
  周期降载，不通过丢弃 Text 达标。

### 16.7 构建验证

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

### 17.1 可观测性与诊断

不存储每包明细，但 debug/stats 接口至少提供以下可清零饱和计数器：

```text
team_rx_presence
team_rx_location
team_rx_sync_request
team_rx_decode_error
team_rx_wrong_channel
team_rx_duplicate_or_old
team_member_table_full
team_tx_queued
team_tx_deferred
team_tx_dropped
team_sync_rate_limited
team_join_discover_rx
team_join_offer_tx
team_join_request_rx
team_join_rate_limited
team_join_pairing_approved
team_join_pairing_rejected
team_join_aead_error
team_join_timeout
```

计数器更新不得打印节点密钥、精确坐标或聊天明文。生产日志只记录聚合值、子类型和错误类型。
为便于现场诊断，可显示本机 Team ID 短指纹、key version、boot ID 和 sender short ID，
但不显示 Join Credentials、ECDH 共享秘密、会话密钥或 Team Key。

### 17.2 发布、回滚与数据兼容

- Team 默认关闭；新固件升级后不会自动开始发送位置。
- 回滚到旧固件时，旧固件不会读取独立的 `/team_prefs`；原有 `/channels2` 中 32/40 个
  用户频道仍可加载。
- 两个系统 slot 都不写入 `/channels2`，因此回滚后不会将 Join 或 Team Credentials
  暴露为普通用户频道。
- POC 构建在启动画面或版本信息中标记 `TEAM-POC`，防止与量产版混用。
- 发布前做一次升级、断电、配置截断恢复和回滚演练，并保留测试记录。

## 18. 明确不在 V1 范围内的内容

- 可靠 Group Text ACK。
- 二进制分片和文件传输。
- 队长签发成员证书。
- 多 Leader 选举和冲突解决。
- 将所有 Team 成员持久化为 MeshCore Contact。
- 旧 Repeater 对 hop limit 的强制兼容。
- 不重置 Team Key 就对已获批成员实现密码学撤权。
- 在不让保留成员重新加入的情况下自动分发新 Team Key。

## 19. 最终验收定义

只有同时满足以下条件，功能才算完成：

1. Leader 可创建包含随机 256-bit Team Key 的 Team，Member 没有创建密钥的 UI/API。
2. Member 只能主动发现已打开 120 秒配对窗口的 Leader；双方从 ECDH 会话独立计算并显示
   相同的 4 位配对确认码，无邀请码或周期性 Join Beacon。
3. 只有队长核对配对码并点击 Agree 后才下发 Team Key；Reject、确认超时、配对码不一致、
   篡改包和重放不导致密钥下发或安装。
4. Team 开启时不再周期发送带位置的 Identity Advert。
5. Presence、Location 和 Sync Request 使用一个正式/开发 Group Data type 的三个 subtype。
6. 位置帧默认最多传播两跳，并在升级后的 Repeater 上实际停止继续转发。
7. 同步响应具有确定性错峰，不产生响应风暴。
8. T1 可导航到选中的 Team 成员。
9. V4 R8 TFT 可同时查看多个 Team 成员的位置和状态。
10. 任一成员可使用 Group Text 发送全队消息，并使用 Direct Message 单独发给某个队友。
11. `HELTEC_TEAM_ROLE` 默认为 0，V4 R8 TFT Team 构建为 1，角色不持久化且 UI 无法更改。
12. 原有 Public channel、普通 contacts、Advert、Direct message 和 Team 关闭时的 Find Friend 行为没有回归。
13. 协议单元测试、Join 密码学测试、配置测试、路由测试和指定 PlatformIO 环境全部通过。
14. 旧 32/40 个用户频道升级后无丢失，Join/Team 两个系统 slot 无法从 Companion 读写且不落盘。
15. 加入 Team 自动共享位置；退出后不回退到旧 Location Share Advert。
16. 同 hash 冲突、队列已满、packet pool 耗尽和 prefs 保存失败均有可重现测试，
    不导致密钥泄露、成员串频、内存泄漏或半持久化状态。

## 20. 实施者开始编码前必须确认的事项

以下三项尚不能由代码自行决定：

1. MeshCore 上游为 Heltec Team 分配的正式 Group Data type。
2. MeshCore 上游认可的 hop-limit wire encoding；在确认前只能使用受宏保护的 POC 编码。
3. Join Protocol 使用的经审查 HKDF-SHA-256/AEAD 实现、参数、测试向量和平台资源预算。

## 21. 当前实现状态（2026-09-04）

已在本仓库完成 V1 POC 代码：协议编解码、固定容量成员表、仅 RAM 的临时 Team 凭据、两个
保留 Group slot、五段 Join 状态机、ECDH/HKDF/ChaCha20-Poly1305、4 位配对确认码、
Leader Agree/Reject、超时与限速、Presence/Location/Sync 调度、Team 模式下停止位置 Advert、
T1 Find Friend Team 数据源与配对控件、V4 Tracker 成员地图与配对弹窗、全队 Group Text、
临时 TeamPeer Direct Message、密钥重置、退出、统计计数器和协议主机测试。

TeamPeer Direct Message 的收发复用一个固定 ContactInfo 工作对象，ACK 表保存完整收件人
公钥，不保存工作对象指针；成员无需写入普通联系人表即可参与私信解密与路径响应。
Presence 接收校验 sender short ID 与完整公钥前缀；Sync 响应使用设备相关的确定性错峰，
Presence 成功入队后才允许对应 Location 入队，不保证无线接收顺序或可靠送达。
启动同步具有随机抖动、失败重试上限，主动 Sync Request 保留 60 秒冷却。
Tracker 工具栏不再通过长按触发 Team Key Reset；为减少页面及对象开销，明确命名的
`reset team key` 放在 Find Friend 的 Team 操作区，并使用公共二次确认弹窗。

实现采用定长数组，不为成员表、配对 session 或协议帧申请堆内存。所有支持设备均通过
`HELTEC_TEAM_ROLE=0/1` 条件编译决定角色，只保留对应角色的配对状态；T1 默认队员，
V4 r8 TFT 默认队长。Location 使用移动阈值及静止
保活周期；GPS fix 丢失时发送无效位置，不回退到位置 Advert。

以下两项仍属于外部发布门禁，不能由本仓库单方面宣称完成：

1. `0xFF48` 仍是开发 data type，量产前必须取得 MeshCore 正式编号并替换。
2. MeshCore 尚未确认可互操作的 hop-limit wire encoding。因此 Join 使用严格零跳发送，
   Team 运行帧仍沿用现有 Group Flood；不得为了“完成”而私占 header/version 位。正式发布前
   必须按阶段 B 接入上游方案，并完成新旧 Repeater 混合拓扑测试。

除上述事项外，执行者应按照本文默认值推进，不应因为次要命名或 UI 文案问题停止协议和服务层实现。

### 21.1 资源受限修复与回归检查（2026-09-07）

- 审批弹窗一次读取申请人、配对码和 session ID。确认与拒绝只操作该 session，服务层重新
  检查窗口、会话期限与处理状态；旧弹窗不能批准新申请。公共 UI 调度后续申请，不依赖
  Tracker 的状态枚举变化，也不覆盖已经显示的确认内容。
- Find Friend 为队长提供开放加入、关闭窗口、重置密钥和退出；重置、退出均二次确认。
  已开放窗口重复点击不会清掉在途申请。该入口仍要求设备包含 Find Friend UI；无显示设备
  的交互方式不在本次新增范围内。
- 单聊 ACK 表仍为 8 槽，指针改为 32 字节公钥，在 32 位目标上该表增加
  `8 × (32 - 4) = 224` 字节；收发工作对象由 UI 移至 Mesh，仅保留一份。
  不新增逐成员 ContactInfo 数组，不自动写联系人 Flash。其他新增核心状态只有少量
  标志、计数与 session 快照；LVGL 行对象、结构对齐及 Flash 增量需要实际构建/测量。
- Sync Location 绕过静止抑制，无有效 fix 时也返回无效位置帧。每种响应最多尝试入队
  3 次，Presence 失败耗尽后放弃对应 Location；重试可能超出原响应窗口，每种响应额外
  最多约 2 秒，两阶段连续重试及间隔合计约 4.3 秒（主循环延迟与无线队列等待另计）。
  定时更新与响应合并，避免同轮重复发包。
- UI 变化检测改为不重叠字段，包含成员变更序号、配对状态、搜索结果数、加入窗口、
  共享与启用状态。换队/退出清理成员、定位缓存及待发定时器；保存配置失败不提前改变
  原有定时器。`mode=none` 关闭导航，若位置共享仍开启则明确提示。

本次不编译、不烧录。手工验证至少覆盖：

1. A 的审批弹窗保持到超时，B 随后申请；点击旧弹窗确认/拒绝不能处理 B，关闭后应显示 B。
2. A/B 连续申请、连续批准/拒绝；确认码必须与对应成员一致，不漏弹窗。
3. 清空普通联系人但保留 Team，队员间双向私信；连续给 A、B 发消息，倒序收到 ACK，
   `[TEAM] dm queued/ack/rx` 的公钥前缀应对应，旧 ACK 不取消较新消息的超时。
4. 所有设备静止时，新成员加入并请求同步，应收到 Presence 与 Location；无 GPS 的成员
   显示无定位；模拟发送队列满后确认响应有限重试，不产生无限重发。
5. 重置/换队后保持相同坐标，首次位置发送不受旧位置缓存抑制，旧申请不能获得新凭据。
6. T1 设为队长后验证开放、关闭、审批、重置和退出；默认 T1 队员与 V4 队长流程不变。
7. 分别验证 Team 关闭编译分支、共享关闭、mode=none、普通联系人单聊与 Group 群聊。

成员列表仍是 RAM 中由 Presence 得到的近期成员缓存，不是持久化名册；重启后需要重新组队，再通过同步获得成员列表。

### 21.2 统一位置共享与 GPS 焦点（2026-09-07）

- 新建/加入的活跃 Team 自动共享位置；不恢复旧 Team 配置。
  凭据只保存在 RAM；移除独立共享状态与设置接口。
- 删除 GPS 页旧 Location Share/Adv Interval 行、事件与焦点项。GPS 开关仍通过
  `addFocusItem(_sw_gps, _row_power)` 加入页面 group，不设置 DISABLED，保持可选中。
  GPS 总开关始终可关闭，不能因持有 Team 凭据而强制打开或阻止关闭。
- GPS 供电由总开关和 Team 需求共同决定；总开关打开时保持连续采样以支持 30 秒位置周期及随机 Sync 响应，
  无新增任务、成员缓存或动态协议内存。GPS 原始偏好不被改写，退出后恢复原供电策略。
- 旧位置 Advert 主循环调用停止，历史开关不再启动广播；Group Location 编解码、
  移动抑制、静止保活、无效 fix 和 Sync 响应继续保留。

手工验证：GPS 行可通过按键/旋钮选中；重启后为未组队，重新加入后自动共享；
GPS 页关闭/离开屏幕后 Group 仍发送；退出 Team 后无旧位置 Advert 恢复；
无 GPS/fix 时不发送伪造坐标。此次只做静态检查，不编译、不烧录。

补充：`mode=none` 只提示导航关闭，不宣称正在共享。只有已创建/获批加入且启用的 Team
才调度 Group 位置；GPS 关闭时不得使用缓存的有效坐标，必要时发送一次无效位置通知，
成员 Presence 仍保留。关闭导航不删除当前 RAM 中的 Group 凭据，退出 Group 才停止全部 Team 活动。


### 21.3 临时 Team 验证

重启队员后必须重新申请；重启队长后必须重新建队；旧 Team 文件不能恢复成员资格。
连续建队、加入、退出不产生 Team 配置写入，普通频道保存不包含 Team Key。
迁移删除仅在新固件运行时执行，本次未烧录设备、未实际删除设备文件。

### 21.4 Team 控制及消息入口

- Tracker 将 T/J 合并为一个 Team 键：长按打开/关闭本机临时 Team；短按开放/关闭
  接收申请窗口。关闭时不再二次弹窗；长按后的释放不触发短按。
- 键上显示 Off / Team / Join，分别表示未建队、已建队但不接收申请、正在接收申请。
  未建队时短按只提示长按建队。开放申请仍使用 120 秒默认窗口，并保留配对确认。
- Tracker 保留 All 群发按钮，无 Team 时变灰。删除独立 J 和 DM 按钮。
  长按地图上的队员标记进入私聊，预填该对象；普通联系人标记也可私聊。
  拖动地图不触发私聊，自己位置不作为收件人。
- 标记缓存保存 6 字节公钥前缀，按下时复制身份，长按时重新解析；
  不使用会因成员增删而变化的列表序号。复用既有 Send Message overlay，
  不分配额外 ContactInfo 缓存。地图不可用不阻止 Team 开关和群发。
  地图数据直接填入现有缓存，避免额外的返回值标记数组占用调用栈。
- Find Friend 保留 mode（friend / waypoint / none），按模式显示相关选项。
  friend 模式中，队员用同一行执行搜索或取消搜索/配对，搜索到结果才显示 team 下拉框；
  队长用同一行开放或关闭申请。移除重置密钥入口，保留有确认的退出 Team 操作，
  需要新密钥时退出并重新建队。
- 加入 Team 后显示 friend、message friend、message team；消息入口仅在有 Team 时显示，
  未选 friend 时私聊变灰。实际行顺序与 LVGL group 焦点顺序一致。
- HELTEC_TEAM_LOG 控制位置日志：scheduled、skipped（gps_off/no_valid_fix/stationary）、
  invalidation、queue_failed、queued，以及接收拒绝原因和 applied 坐标。
  queued 只表示入队，不代表无线送达；接收端 applied 表示位置已被接收并应用。
  日志不输出 Team Key。

### 21.5 搜索触发重启：加固及验证

- 原接收路径会在 DISCOVER 回调内直接构造、加密 OFFER；现在回调只标记固定会话待应答，
  在主循环 poll 中发送，降低收发嵌套调用栈压力。保留零跳、随机发送延迟和既有限流。
- 不新增动态队列，每个固定会话增加一个待应答标志；关闭申请窗口时清除会话和待处理标志。
  检查 OFFER 名称长度和发送帧最小长度，避免越界。
- ESP32 且启用 HELTEC_TEAM_LOG 时输出启动 reset_reason，以及 discover rx / offer poll 的
  heap 和 stack_hwm，发送结束输出 offer poll complete。没有异常堆栈前，
  不能确认收发嵌套就是本次重启的根因，也不能宣称重启已经彻底修复。

本次不编译、不烧录。实机验证项目：

1. V4 开放申请后，T1 连续搜索，核对 discover accepted → offer poll →
   join tx subtype=17 → offer poll complete；同时保存重启原因及完整 Backtrace。
2. Team 长按开关、短按申请、120 秒超时；长按释放不能再次打开或关闭申请。
3. 长按不同队员标记，确认私聊对象准确；拖动、自己位置、重叠标记和成员退出分别验证。
4. All 群发预填，Find Friend 搜索/取消/选择/确认、friend 私聊及 T1 焦点顺序。
5. GPS 关闭、无定位、静止、移动以及新成员同步的位置日志。


### 21.6 T1 第一轮资源精简

- 保留 32 人容量、4 个搜索结果、配对认证、位置共享、群发/私聊和 waypoint；
  暂不修改导航算法或缩小 LVGL 内存池。
- HELTEC_TEAM_LOG 分为 0（关闭）、1（关键状态/错误）、2（详细帧/位置日志）。
  T1 默认 1，USB 默认关闭 MESH_DEBUG；验证位置流程时将 Team 日志切换为 2。
  V4 调整为 2，保留原有详细诊断能力。
- TeamMember 删除无读取用途的 capabilities、device_type、last_presence_ms、
  altitude_dm、speed_cms、course_cdeg 缓存。无线帧字段、解码校验和发送内容保持不变。
- 下拉列表保留每页 10 人和现有翻页逻辑，缓冲按最坏标签长度分配（327 字节）；
  统计人数/选中排名不再创建临时页面或格式化标签，减少重复扫描和临时栈占用。
- 未编译；实际 Flash 净收益、结构对齐后的 RAM 收益及运行栈峰值需同配置编译/实测。

### 21.7 T1 第二轮：轻量导航

- T1 设置 HELTEC_GEO_COMPACT=1，距离/方位统一使用平均半径 6371008.8 m 的球面算法，
  编译时排除 Vincenty 椭球迭代；其他设备默认 0，保持原算法。改回 0 可恢复原行为。
- 这是精度取舍，不是等价替换：球面距离与椭球距离存在模型误差，远距离累计误差更大。
  不用于测绘级测距。找朋友距离、GPS 样本离散度和 GPS 轨迹距离阈值均使用所选模型。
- 保留 double 坐标、Haversine 数值夹紧、经度归一化及近距离小差值计算。
  对跖点没有唯一初始方向；本轮不新增这种情况下的 UI 状态。
- 删除五个 GPS 样本中未读取的 source_ms，保留全局最新时间戳及重复样本抑制。
  离散度计算复用纬度临时数组，源码层面减少一个 5 个 double 的栈数组。
- 新增同点、亚米级、日期变更线、极区、对跖点和球面半径测试。
  native 使用原算法，native_geo_compact 使用轻量模式；两者均未编译或执行。
- 静态检查不能证明实机性能或 Flash 节省量；需编译对比，并验证近目标迟滞、
  GPS 抖动滤波和轨迹记录触发边界。组队、消息、安全校验与成员上限不变。

### 21.8 第三轮：列表扫描精简

- 普通联系人数量与选中排名合并为一次遍历；不请求排名的调用不加载选中候选。
  比较排名时显式排除选中项自身，避免两次采样的接收年龄差导致自身计入排名。
- 页面加载不再请求随后丢弃的排名；跳过前页和生成当前页共用一个选择循环。
  保持原有最近接收优先、排序判定和分页规则，不新增联系人排序数组或常驻缓存。
- Team 元数据查询直接使用成员数量和连续序号；只为当前页生成标签。
  人数仍为 32、每页仍为 10，负 offset 归零，超出末页返回空列表。
- 不修改 LVGL 配置、加密和导航算法。未编译；Flash 净收益待相同环境编译对比。
  实机需验证空列表、首/末页、跨页选择、无定位的选中联系人及成员退出后的焦点。

### 21.9 产品收敛：friend 仅使用当前 Team

- 本节取代此前普通联系人回退设计：未加入 Team 时不显示 friend 选择框，
  friend 模式不读取普通联系人坐标，也不回退到 MeshCore 联系人列表。
- 移除找朋友适配层的普通联系人筛选、接收时间排序、位置读取和旧选中身份恢复。
  MeshCore 联系人存储、普通消息、BLE/USB、Tracker 地图联系人显示均不改动。
- 当前 Team 成员仍可分页选择；无定位成员允许选中并私聊，但没有有效位置时不提供导航。
  Team 目标身份仅驻留 RAM，选择/自动选择不写配置；模式和 waypoint 设置继续保存。
  兼容现有设置文件格式，但忽略其中的旧 friend 索引/公钥，保存时将这些字段置空。
- waypoint 独立保留，未加入 Team 也可使用。没有编译 Team 功能的设备不提供 friend 目标。
- 无新增动态分配。未编译；需验证旧设置升级、无 Team、入队/退队、无定位私聊、
  成员列表变化和 waypoint 切换；Flash 净收益待同配置编译对比。


### 21.10 UI 清理与固定文本存储

- 删除无调用的 cycleFindFriendTargetContact，以及已无 UI 入口的 TeamReset 事件、
  确认分支、文案和 facade 方法；保留 TeamLeave 的原事件数值。
- 简化找朋友设置加载包装函数，底层 DataStore 文件兼容格式不变。
  成员标签、定位判断改为私有方法，ReceiptPin 改名 TeamPin，明确当前只固定 Team 目标。
- 消息主菜单六条固定文字、类别文字使用只读常量；只保留可变收件人行，
  类别使用三个字节的编号。模型字段层面减少 213 字节（实际结构对齐以编译结果为准）。
- LVGL LONG_DOT 会写入标签文本，不能直接绑定只读常量。
  按键版增加五行固定可写显示缓冲，共 140 字节；触摸版复用已有缓冲。
  更新前先恢复旧省略号内容，再覆盖缓冲、重新应用 DOT。
  因此按键版字段级净减少约 73 字节，并非全部 213 字节。
- 个人列表直接读取现有联系人名称缓存，取消复制到模型列表文本；
  两种显示端仍保持原有最多 27 字节列表文本长度，不改变发送目标身份缓存。
  群组列表文本仍需独立保存，本轮不复用其存储来覆盖联系人身份数据。
- Recent 页面仍保留显示名称副本：其 LONG_DOT 会改写文本，直接绑定 _items.name
  可能污染原始名称。不改变省略号表现的前提下，需额外设计渲染缓冲才能安全复用。
  键盘提交快照同样保留，不假设异步提交与编辑生命周期互斥。
- 未新增动态分配，不改 MeshCore 非显示核心，不缩小 LVGL 内存池；未编译。
  验证重点：所有消息类别组合、长名称省略/翻页/返回、私聊目标身份、退出 Team 确认、
  waypoint 和自动选择成员。Flash 与实际 RAM 净收益需同配置编译对比。
