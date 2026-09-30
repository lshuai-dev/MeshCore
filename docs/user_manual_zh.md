# MeshCore LVGL UI 用户操作手册

本文面向使用带屏幕 MeshCore 设备的普通用户，按照“要完成什么”组织操作步骤。不同设备的
屏幕、按键、触摸、GPS、指南针和地图能力不同；没有显示的页面或控件，通常表示当前硬件或
固件不包含该功能。

## 1. 第一次使用：先设置无线参数

第一次开机不要先组队或发送消息。请按以下顺序完成基础设置。

### 1.1 开机前

1. 安装与设备频段匹配的 LoRa 天线。
2. 接入电池或稳定的 USB 电源。
3. 如果需要 GPS，确认 GPS 天线已经连接并朝向天空。
4. 等待设备进入 `Home` 页面。

禁止在没有天线或合适假负载的情况下进行发射测试。

### 1.2 第一步：设置 Region 和频率

`Region` 是整套 LoRa 参数预设，不只是频率。它会同时决定频率、带宽、扩频因子等参数。

1. 打开页面导航。
2. 进入 `System`。
3. 选中 `Region` 并打开下拉框。
4. 选择符合设备频段和当地法规的预设。
5. 确认保存。
6. 进入 `Radio`，核对 `FQ`、`SF`、`BW` 和 `CR`。

需要互相通信的设备必须使用兼容的无线参数。只把频率设成相同，但 `SF`、`BW` 或 `CR`
不同，仍然无法通信。

`Custom/fwd` 表示当前处于自定义参数或转发状态，不是普通地区预设。首次使用不要开启
`Repeat Mode`；先完成普通收发测试。

### 1.3 第二步：连接 Companion

BLE 固件：

1. 确认固件名称包含 `_ble`。
2. 在手机上打开蓝牙和 MeshCore Companion 客户端。
3. 扫描并选择当前设备。
4. 配对时使用 `Home` 页面显示的 PIN。
5. `Home` 显示 `Connected` 即表示连接成功。

USB 固件：

1. 确认固件名称包含 `_usb`。
2. 使用支持数据传输的 USB 线连接设备。
3. 在 Companion 客户端选择对应串口。
4. 回到 `Home` 检查连接状态。

Companion 链路和 LoRa 链路相互独立。Companion 未连接不代表 LoRa 已停止工作。

### 1.4 第三步：测试 LoRa 收发

最好准备两台无线参数一致的设备：

1. 在设备 A 的 `Radio` 操作菜单中选择 `send advert`。
2. 在设备 B 的 `Recent` 页面查看是否出现设备 A。
3. 从设备 A 或 Companion 向设备 B 发送一条消息。
4. 检查设备 B 是否出现消息预览、提示音或未读数量变化。
5. 交换发送和接收角色，再测试一次。

`Queued` 只表示消息进入本机发送队列，不代表接收方已经收到。验收应以接收方实际显示为准。

### 1.5 第四步：检查 GPS 和指南针

设备带 GPS 时：

1. 进入 `GPS`。
2. 打开 `gps`。
3. 到室外开阔区域等待状态变为 `fix`。

设备带指南针时：

1. 进入 `Compass`。
2. 远离磁铁、音箱、钢制桌面和大电流导线。
3. 按校准提示缓慢做多方向“8”字运动。
4. 确认航向和校准质量稳定。

完成以上步骤后，再使用 Find Friend、Team、位置分享和电子围栏。

## 2. 基本操作

### 2.1 通用操作语义

| 操作语义 | 作用 |
| --- | --- |
| 上一项 / `PREV` | 移到上一个图标、列表项或控件 |
| 下一项 / `NEXT` | 移到下一个图标、列表项或控件 |
| 确认 / `ENTER` | 进入页面、打开下拉框、切换开关或执行命令 |
| 返回 / `ESC` | 关闭当前弹层；在页面根层打开导航 |
| 操作菜单 | 打开发送消息、发送 Advert、校准等页面命令 |

当前选中的控件通常会显示边框、反色或高亮。控件不在可视区域时，移动焦点会自动滚动页面。

### 2.2 常见单按键设备

| 按键动作 | 功能 |
| --- | --- |
| 单击 | 下一项 |
| 双击 | 返回 |
| 三击 | 打开操作菜单 |
| 长按约 1 秒 | 确认 |

多击通常需要在约 350 ms 内完成。屏幕刚熄灭时，第一次操作可能只唤醒屏幕。

### 2.3 RadioCore P0/P1

带 TCA6408A 扩展输入的 RadioCore RC32/RC52：

| 输入 | 导航页面 | 普通 Screen |
| --- | --- | --- |
| P0 | 上一个图标 | 上一个选中项 |
| P1 | 下一个图标 | 下一个选中项 |

P0/P1 只负责移动，确认仍使用 USER 长按。P0/P1 为低电平有效，每次按下只触发一次；如果
使用保持式拨码开关，必须先拨回 OFF，再拨到 ON 才能再次触发。

### 2.4 触摸设备

- 点击：聚焦、切换开关或执行按钮。
- 上下拖动：滚动列表。
- 从左边缘向右滑：打开导航。
- 在地图上拖动：平移地图。
- 地图工具栏按钮按其图标执行缩放、回到 GPS 或围栏操作。

### 2.5 导航和弹层

在普通页面根层执行返回可以打开导航。弹层、下拉框或键盘打开时，返回会先关闭最上层，
可能需要多次返回才能打开导航。

逻辑页面通常包括：

```text
Home → Recent → Radio → Compass（可选） → Find Friend（可选）
     → GPS（可选） → Tracker（可选） → System
```

网格导航的视觉排列可能不同，`Find Friend` 也可能位于单独的底部入口。

## 3. 收发消息和 Advert

### 3.1 发送消息

1. 进入 `Radio`。
2. 打开操作菜单。
3. 选择 `send message`。
4. 选择目标：广播、群组或联系人。
5. 选择预设消息，或选择 `custom...` 输入文字。
6. 确认发送。

自定义消息不能为空，当前 UI 最多输入 120 个字符。部分触摸构建也可通过 `Quick Ping`
直接选择目标和消息。

### 3.2 发送 Advert

在 `Radio` 操作菜单选择 `send advert`。Advert 用于发布本机节点信息；开启位置分享后，还可
携带位置。Advert 不是聊天消息，频繁发送会增加信道占用和耗电。

### 3.3 查看收到的消息

- 新消息通常先显示在 Preview overlay。
- 上一项/下一项可以查看预览队列中的其他消息。
- 消息超过显示区域时会自动上下滚动。
- 确认键关闭预览并清空当前预览队列。
- 返回键不会关闭预览。
- `Home > MSG` 显示未读数量，不是完整历史消息数。

## 4. 创建或加入 Team

Find Friend 的 `friend` 模式和电子围栏都以 Team 为基础。参与设备应先设置相同的无线参数，
并保持在可通信范围内。

### 4.1 Leader 创建 Team

在作为 Leader 的设备上：

1. 进入 `Find Friend`。
2. 将 `mode` 设为 `friend`。
3. 将 `role` 设为 `leader`。
4. 选择 `> create team`。
5. 创建成功后，选择 `> open joining`，允许成员加入。
6. 等待成员发起加入请求。
7. 出现确认弹层时，核对成员名称和四位配对码。
8. 配对码与成员设备一致时选择 `Confirm`；不一致时取消。
9. 所有成员加入后，选择 `> close joining`。

### 4.2 Member 加入 Team

在成员设备上：

1. 进入 `Find Friend`。
2. 将 `mode` 设为 `friend`。
3. 将 `role` 设为 `member`。
4. 等待 `team` 下拉框出现可加入的 Team。
5. 选择目标 Team；超时后可选择 `search again`。
6. 屏幕显示 `Confirm xxxx` 后，与 Leader 核对四位配对码。
7. 等待 Leader 确认。
8. 加入成功后，`member` 下拉框会列出 Team 成员。

出现 `waiting...` 表示正在等待 Leader；`rejected` 表示请求被拒绝；`join failed` 或
`no team / timeout` 时，应检查无线参数和距离后重试。

### 4.3 退出 Team

- Member 可在 `team` 下拉框选择 `leave team`。
- 将 `mode` 改为 `none` 会关闭 Find Friend；当前实现也会退出正在使用的 Team。
- Leader/Member 改变角色、退出 Team 或重新加入其他 Team 时，原 Team 的临时状态会清除。

## 5. 使用 Find Friend

### 5.1 使用前提

- GPS 硬件或 LocationProvider 可用。
- 本机获得有效 GPS fix。
- Friend 模式下已加入 Team，目标成员有有效位置。
- Waypoint 模式下已输入有效坐标。
- 有指南针时建议先完成校准。

GPS 硬件不可用时，`mode` 会被禁用，无法聚焦或打开。GPS 可用但尚未 fix 时，`mode` 仍可
选择，页面显示 `need gps`。

### 5.2 找 Team 成员

1. 按第 4 章创建或加入 Team。
2. 在 `Find Friend` 中选择 `mode > friend`。
3. 在 `member` 中选择目标成员。
4. 等待本机和目标成员的位置有效。
5. 按方向指针和距离移动。

有指南针时，指针表示相对于设备朝向的转向；没有指南针时显示 north-up 方位。目标位置来自
最近收到的 Team 位置，不是连续实时连接。

### 5.3 导航到坐标

1. 选择 `mode > waypoint`。
2. 选择 `> set waypoint coordinates`。
3. 按 `纬度,经度` 输入十进制度数，例如：

```text
31.230400,121.473700
```

4. 确认保存。
5. 等待本机 GPS fix，然后查看方向和距离。

纬度范围为 -90 到 90，经度范围为 -180 到 180。

### 5.4 状态含义

| 状态 | 含义和处理 |
| --- | --- |
| `mode` 禁用 | GPS 硬件或 LocationProvider 不可用 |
| `starting...` | 正在等待 GPS、目标或航向数据 |
| `need gps` | 本机还没有有效 GPS fix |
| `no location` | 目标成员没有有效位置 |
| `target stale` | 目标位置太旧 |
| `calibrate` | 指南针质量不足，需要校准 |
| `nearby` | 已非常接近目标，不再给出稳定转向 |

## 6. 使用电子围栏

电子围栏是条件功能。只有固件同时启用了 `Tracker` 和 `HELTEC_GEOFENCE` 时才会出现；当前
标准构建中主要用于带触摸屏的地图设备，例如 Heltec V4 R8 TFT。RadioCore 当前不包含
`Tracker`，因此不会显示电子围栏。

### 6.1 使用条件

- 当前设备是 Team Leader。
- 已创建并启用 Team。
- Team 成员已加入，并能上报有效位置。
- `Tracker` 地图可打开。
- 当前实现通过触摸地图添加围栏顶点。

不满足条件时，围栏按钮会隐藏或禁用。

### 6.2 创建围栏

1. 先创建 Team，并让成员加入。
2. 进入 `Tracker`。
3. 平移和缩放地图，使整个围栏区域位于屏幕中。
4. 长按工具栏中的 `F`。
5. 出现 `Fence marking started` 后，依次点击地图添加顶点。
6. 至少添加 3 个顶点。
7. 最后点击第一个顶点附近，在地理距离约 10 米内完成闭合。
8. 出现 `Fence active` 表示围栏已经生效。

第一个点会使用不同颜色标记。围栏最多 12 个顶点；出现 `Fence full; tap first point` 时，
应点击首点附近完成闭合。

### 6.3 取消或删除

- 编辑过程中再次长按 `F`：取消本次编辑，未闭合顶点全部丢弃。
- 编辑过程中离开 `Tracker`：未完成围栏自动取消。
- 围栏生效后长按 `D`：删除围栏。

活动围栏存在时不能直接创建第二个围栏，必须先删除。

### 6.4 告警行为

Leader 收到成员新的有效位置后会判断其是否位于围栏内：

- 成员越界：显示 `<成员名> outside fence`，并触发告警反馈。
- 成员回到围栏内：显示 `<成员名> back inside`。
- 成员没有位置或位置已经过期：本次不判断。
- Leader 自身不作为被监控成员。

围栏只保存在 Leader 本机，不会同步给成员，也不会自动发送 Team 消息。以下情况会清除围栏：

- 删除围栏。
- Team 解散或退出。
- Leader 改为 Member。
- 创建或加入新的 Team。
- 设备重启。

电子围栏依赖 GPS 和 Mesh 位置更新，不是生命安全级实时监控功能。

## 7. GPS、位置分享和轨迹

### 7.1 GPS 状态

| 状态 | 含义 |
| --- | --- |
| `can't access gps` | 固件无法访问 GPS 硬件 |
| `GPS Off` | GPS 已关闭 |
| `GPS Sleep` | GPS 已允许，但当前没有持续供电需求 |
| `no fix` | GPS 已工作，尚未定位 |
| `fix` | 已获得有效位置 |

冷启动可能需要数分钟。应在室外开阔区域静置测试，不要在室内仅凭 `no fix` 判断硬件损坏。

### 7.2 分享位置

打开 `location share` 后，设备通过 Advert 或 Team 状态分享位置。`adv interval` 越短，位置更新
越及时，但发射和耗电越多。离开测试区域或不希望暴露位置时应关闭。

### 7.3 GPS 轨迹

固件支持时可打开 `gps track`：

- 第一条有效定位作为起点。
- 后续通常移动约 50 米才增加一个轨迹点。
- 当前最多约 600 个点，达到上限后自动停止。
- 关闭 `gps track` 会结束当前轨迹文件。
- 当前屏幕 UI 不提供本地轨迹浏览，需要通过 Companion 导出。

导出前先停止记录，否则请求可能返回 `recording_active`。

## 8. 指南针

`Compass` 页面可能显示方向盘、`HDG` 航向、`Q` 校准质量和磁场数据。

### 8.1 校准

1. 进入 `Compass`。
2. 打开操作菜单并选择 `calibrate`，或等待自动校准提示。
3. 远离磁性物体和大块金属。
4. 缓慢做多方向“8”字运动并改变各轴朝向。
5. 出现 `Calibration OK` 后结束。

`No compass` 表示没有检测到硬件；`Starting...` 表示传感器尚未提供稳定数据；`Q` 较低表示
航向可能不准确。

改变设备安装姿态后，建议重新校准。Find Friend 的相对转向直接受指南针误差影响。

## 9. Tracker 离线地图

### 9.1 地图文件

Tracker 使用标准 XYZ PNG 瓦片。SD 卡支持以下任一目录：

```text
/maps/z/x/y.png
/tiles/z/x/y.png
```

要求：

- PNG 为 256 x 256 像素。
- 扩展名使用小写 `.png`。
- `z`、`x`、`y` 都是十进制目录或文件名。
- `z` 建议在 2 到 18 之间。
- 使用 XYZ 行号，不是 TMS；TMS 需要先执行 `y_xyz = 2^z - 1 - y_tms`。
- `maps` 或 `tiles` 必须直接位于 SD 卡根目录。

不要把 `.mbtiles`、在线 URL、JPG 或一张完整大地图直接复制进去。

### 9.2 使用地图

- 拖动地图：平移。
- `-`：缩小。
- `+`：放大。
- `G`：回到本机有效 GPS 位置。
- Team 成员标记：显示最近收到的成员位置。

手动平移后，GPS 更新不会强制把地图拉回本机；需要点击 `G`。

`noSD` 表示未识别 SD 地图；`Map unavailable` 表示地图资源初始化失败；空白区域通常表示当前
缩放级别和位置没有对应瓦片。

## 10. 页面用途速查

| 页面 | 主要用途 |
| --- | --- |
| `Home` | 节点 ID、未读数量、Companion 连接和配对 PIN |
| `Recent` | 最近听到的节点 Advert |
| `Radio` | 查看无线参数、RSSI/SNR，发送消息或 Advert |
| `GPS` | GPS 开关、定位状态、位置分享和轨迹 |
| `Compass` | 查看航向和校准指南针 |
| `Find Friend` | Team、成员导航和 Waypoint 导航 |
| `Tracker` | 离线地图、成员位置和电子围栏 |
| `System` | Region、Repeat Mode、息屏、Companion、蜂鸣器和数据清除 |

## 11. System 设置

### 11.1 Repeat Mode

只在确实需要转发时打开。开启后选择硬件支持且符合法规的转发频率。出现
`Frequency unsupported`、`Invalid radio params` 或 `Repeat mode unavailable` 时，不要反复
强制开启，应检查频段、预设和硬件能力。

改变 `Region` 会关闭正在运行的 Repeat Mode，避免旧参数继续转发。

### 11.2 Screen Off

设置显示自动关闭时间。屏幕关闭不等于设备关机，LoRa、Team、位置分享和 Companion 可以继续
运行。屏幕熄灭后的第一次按键或触摸可能只负责唤醒。

### 11.3 Bluetooth、Buzzer 和消息预设

- `Bluetooth` 控制 Companion 链路能力，不控制 LoRa。
- `Buzzer` 只控制提示音，不影响消息接收。
- 支持音量的设备会显示音量控件。
- 支持屏幕键盘的构建可编辑消息预设。

### 11.4 Clear Data 和 Factory Reset

这些操作会删除配置、联系人或频道。执行前应确认数据已经备份。恢复出厂不会删除 SD 卡中的
地图 PNG，但会重置内部地图中心、缩放和其他偏好。

## 12. 时间、电源和隐私

### 12.1 时间同步

设备时间可来自 Companion、GPS、CLI 或最近联系人时间。GPS 有 fix 后通常能提供更准确时间。
`Recent` 的“多久前”明显不正确时，先连接 Companion 或让 GPS 完成一次定位。

### 12.2 省电建议

- 不需要定位时关闭 GPS、轨迹和 Find Friend。
- 不需要位置广播时关闭 `location share`。
- 增大 Advert 周期。
- 设置合理的 `Screen Off` 时间。
- 不需要转发时关闭 Repeat Mode。

### 12.3 隐私

位置分享、Team 状态和 Advert 可能暴露位置或活动状态。只在可信网络中使用；结束测试或离开
活动区域后，关闭位置分享并退出临时 Team。

## 13. 常见问题

### 13.1 两台设备不能通信

1. 检查天线是否安装。
2. 对照两台设备 `Radio` 页的 `FQ`、`SF`、`BW` 和 `CR`。
3. 确认 Region 和频段匹配。
4. 先在近距离无遮挡环境发送 Advert。
5. 检查接收端 RSSI/SNR 是否变化。

### 13.2 BLE 搜不到设备

- 确认使用 `_ble` 固件，不是 `_usb` 固件。
- 确认 `System > Bluetooth` 已打开。
- 删除手机中的旧配对记录后重新扫描。
- 重启手机蓝牙和设备。

### 13.3 GPS 一直 no fix

- 到室外开阔区域，GPS 天线朝上静置数分钟。
- 确认状态不是 `GPS Off` 或 `GPS Sleep`。
- 检查天线、串口和供电。
- `can't access gps` 表示硬件或固件能力不可用，不是普通搜星慢。

### 13.4 Find Friend 的 mode 被禁用

这表示 GPS 硬件或 LocationProvider 不可用。尚未定位只会显示 `need gps`，不会禁用 `mode`。
应核对设备型号、固件功能和 GPS 连接。

### 13.5 Find Friend 没有方向

- 确认 `mode` 不是 `none`。
- 确认本机 GPS 为 `fix`。
- Friend 模式检查是否已加入 Team、目标是否有位置。
- Waypoint 模式检查坐标是否保存成功。
- 有指南针时检查校准质量。
- 无指南针设备只显示 north-up 方位。

### 13.6 Team 搜不到或加入失败

- Leader 必须创建 Team 并打开 `open joining`。
- Member 与 Leader 必须使用兼容无线参数。
- 配对码必须一致。
- 超时后在 `team` 下拉框选择 `search again`。
- Leader 关闭加入窗口后，新成员不能加入。

### 13.7 电子围栏按钮不显示

- 当前固件必须包含 Tracker 和电子围栏功能。
- 当前设备必须是 Team Leader，并且已有活动 Team。
- 当前实现需要触摸地图添加顶点。
- RadioCore 当前不提供 Tracker/电子围栏。

### 13.8 电子围栏不能闭合

- 至少添加 3 个不重合的顶点。
- 最后点击第一个点约 10 米范围内。
- 放大地图后再点击首点附近。
- 顶点达到 12 个时必须回到首点闭合，或长按 `F` 取消重画。

### 13.9 地图显示 noSD、空白或 Map unavailable

按以下顺序检查：

1. SD 卡是否被识别。
2. 根目录是否存在 `maps` 或 `tiles`。
3. 文件是否为 `z/x/y.png`。
4. PNG 是否为 256 x 256，扩展名是否为小写。
5. 是否使用 XYZ 而不是 TMS。
6. 当前地图位置和缩放级别是否真的存在瓦片。

### 13.10 Compass 航向不稳定

- 远离磁铁、音箱、车辆、钢制桌面和大电流线缆。
- 重新做多方向“8”字校准。
- 观察 `Q` 是否稳定。
- 改变安装姿态后重新校准。

### 13.11 按键只能移动，不能确认

- 单按键设备通常使用长按确认。
- RadioCore P0/P1 只移动焦点，确认使用 USER 长按。
- 屏幕刚唤醒时再操作一次。
- 检查焦点是否落在可操作控件上。

## 14. 功能差异

| 硬件或构建差异 | 用户看到的结果 |
| --- | --- |
| 无 GPS | GPS 功能不可用，Find Friend `mode` 禁用 |
| 无指南针 | 不显示 Compass；Find Friend 使用 north-up 方位 |
| 无地图 | 不显示 Tracker 和电子围栏 |
| 无触摸 | 不能使用当前触摸式电子围栏选点 |
| 无蜂鸣器 | System 不显示 Buzzer |
| 单键设备 | 使用单击、双击、三击和长按完成操作 |
| BLE/USB/Wi-Fi 固件 | Companion 连接方式不同，LoRa 功能不因此改变 |

## 15. 快速操作索引

| 目标 | 操作路径 |
| --- | --- |
| 首次设置频率 | `System > Region`，然后到 `Radio` 核对参数 |
| 发送消息 | `Radio > 操作菜单 > send message` |
| 发送 Advert | `Radio > 操作菜单 > send advert` |
| 打开 GPS | `GPS > gps` |
| 分享位置 | `GPS > location share` |
| 校准指南针 | `Compass > 操作菜单 > calibrate` |
| 创建 Team | `Find Friend > mode: friend > role: leader > create team` |
| 加入 Team | `Find Friend > mode: friend > role: member > team` |
| 找 Team 成员 | `Find Friend > mode: friend > member` |
| 导航到坐标 | `Find Friend > mode: waypoint > set waypoint coordinates` |
| 创建电子围栏 | Leader 进入 `Tracker`，长按 `F`，触摸添加顶点并回到首点闭合 |
| 删除电子围栏 | `Tracker` 中长按 `D` |
| 查看离线地图 | `Tracker` |
| 回到本机位置 | `Tracker > G` |
| 设置息屏 | `System > Screen Off` |
| 清除数据 | `System > Clear Data` |
| 恢复出厂 | `System > Factory Reset` |

首次使用的推荐顺序始终是：安装天线 → 设置 Region/频率 → 测试 Advert 和消息 → 获取 GPS fix
→ 校准指南针 → 创建 Team → 使用 Find Friend 或电子围栏。
