# Heltec 电源管理设计方案

## 1. 目标

本方案用于后续电源管理调整，当前阶段不修改实现。

设计目标：

- 在不影响 Mesh、GPS Track、Team 位置共享和找朋友功能的前提下降低功耗。
- 使用固定容量、无动态内存的结构，控制 RAM 和 Flash 占用。
- 统一资源使用规则，但不引入通用资源管理框架。
- 保留现有 `MainBoard::sleep()` 平台抽象，兼容 ESP32 和 nRF52840。
- 正常低功耗期间始终保持 LoRa 接收能力。
- MCU 休眠采用“硬件中断 + 有界一次性定时唤醒”，不能仅依赖外部中断。

## 2. 核心原则

GPS、指南针、显示等功能视为受管理资源。业务通过 demand bitmask 声明资源需求，
而不是直接控制硬件或使用普通引用计数。

普通引用计数容易因重复进入、重复释放或异常退出而失衡。bitmask 的每一位代表一个
固定使用者，设置和清除都是幂等操作。

当 demand mask 归零时，资源进入自身允许的最低功耗状态，不等同于统一断电：

| 资源 | demand 存在 | demand 为零 |
| --- | --- | --- |
| GPS | 保持定位和数据接收 | 关闭 GPS 模块 |
| Compass | 采样并运行姿态算法 | 停止采样和算法 |
| Display | 显示并运行 LVGL | 熄屏并停止 UI 刷新 |
| Button | 正常输入 | 保留中断唤醒 |
| Touch | 正常采样 | 停止轮询，保留 IRQ 唤醒 |
| LoRa | 正常收发 | 保持 RX 和 DIO1 唤醒 |
| MCU | 运行主循环 | WFE 或带有界 timer wake 的 light sleep |

只有用户主动关机或重启时才允许关闭 LoRa。

## 3. 固定 Demand

不使用动态注册、STL 容器、资源句柄或通用 `ResourceManager`。每类资源保留一个固定
整数 mask。

```cpp
enum class GpsDemand : uint8_t {
    Track      = 1U << 0,
    TeamShare  = 1U << 1,
    FindFriend = 1U << 2,
    GpsScreen  = 1U << 3,
    MapScreen  = 1U << 4,
};

enum class CompassDemand : uint8_t {
    FindFriend = 1U << 0,
    Screen     = 1U << 1,
    Calibration = 1U << 2,
};
```

接口保持类型化：

```cpp
void setGpsDemand(GpsDemand demand, bool active);
void setCompassDemand(CompassDemand demand, bool active);
```

不使用统一的 `setResourceDemand(resource, demand, active)`，避免不同资源的 demand
混用，同时不产生额外运行时开销。

## 4. 功能与资源映射

| 功能 | GPS | Compass | Display | LoRa RX |
| --- | --- | --- | --- | --- |
| GPS Track | 持续保持 | 不要求 | 可熄屏 | 保持 |
| Team 位置共享 | 持续保持 | 不要求 | 可熄屏 | 保持 |
| Find Friend | 持续保持 | 持续采样 | 可熄屏 | 保持 |
| GPS 页面 | 页面活动时保持 | 不要求 | 保持 | 保持 |
| Map 页面 | 页面活动时保持 | 不要求 | 保持 | 保持 |
| Compass 页面 | 不要求 | 页面活动时保持 | 保持 | 保持 |
| Compass 校准 | 不要求 | 校准结束前保持 | 保持 | 保持 |
| 普通熄屏 | 无后台需求时关闭 | 无后台需求时停采样 | 关闭 | 保持 |
| 主动关机 | 关闭 | 停止 | 关闭 | 关闭 |

Find Friend 的资源需求以功能是否开启为准，不能只以页面是否处于前台为准。页面状态
只负责 UI 展示，不应释放后台导航需要的 GPS 和 Compass。

## 5. 共享电源约束

T1 的多个传感器可能共享电源轨。单个逻辑资源不能直接关闭共享电源。

Compass demand 归零时只停止 I2C 采样、姿态算法和 UI 通知。只有确认 GPS、Compass
及其他共享资源均无需求后，才允许释放传感器电源轨。

共享电源判断应放在板级或传感器管理层，不放在 UI 页面中。

## 6. 平台低功耗边界

### ESP32

- 正常空闲使用现有 `MainBoard::sleep()` 和 light sleep；空闲路径必须使用有界唤醒时间，不能使用无定时器的 `sleep(0)`。
- LoRa DIO1、物理按键和触摸 IRQ 作为唤醒源。
- LoRa 保持 RX，FEM 保持接收状态。
- GPS 工作且依赖 UART 持续接收时，不强制进入可能丢失串口数据的 sleep；允许主循环
  使用短延时等待。
- Deep sleep 只用于主动关机等允许重新启动的场景。

### nRF52840

- 正常空闲继续使用 System ON idle、SoftDevice wait 或 WFE。
- 保留 RAM、LoRa RX、GPIO 中断和必要的 UART/传感器状态。
- System OFF 只用于主动关机。

`PowerMgr` 不新增 `idle()` 平台接口。普通 MCU 休眠继续统一调用现有
`MainBoard::sleep()`，ESP32 仅补充固定毫秒级 light sleep 入口，不建立软件 deadline 聚合框架。

## 7. PowerMgr 职责

后续调整后的 `PowerMgr` 只负责：

1. 显示自动关闭与禁止超时的原因 mask。
2. GPS demand mask。Compass demand 留在 UI/传感器采样协调层。
3. 资源状态协调、失败重试和状态快照。
4. 关机和重启流程。

硬件适配接口命名为 `PowerPlatform`。它负责显示、
GPS 和关机相关硬件操作，但不接管普通 MCU idle。

不引入动态对象、消息队列或通用电源域框架。资源状态只在 demand 变化、重试到期或
显示状态变化时协调，避免每轮重复操作硬件。

## 8. 现有实现需要保留的行为

- GPS Track 开启后，熄屏和切换页面不能关闭 GPS。
- Team 位置共享开启后，GPS 持续可用。
- Find Friend 开启后，GPS 与 Compass 持续可用。
- Compass 校准期间不能停止 Compass 采样。
- 普通休眠不能调用 `powerOffRadio()`。
- 未读消息、Team 协议、GPS Track 记录和告警超时不能依赖 UI 是否亮屏。
- 亮屏后必须恢复焦点、刷新当前 Surface 并处理休眠期间积累的状态变化。
- 无外部事件时，Team、Join、Mesh 延迟发送和资源关闭 deadline 仍必须推进。

## 9. 后续迁移顺序

1. 为 Compass 增加固定 demand mask，仅控制采样，不控制共享电源。
2. 将 Find Friend、GPS Track、Team Share 和页面生命周期映射到明确 demand。
3. 让 demand 变化驱动资源协调，删除主循环中重复的 GPS reconcile。
4. 熄屏后停止非必要的 LVGL 和 UI 查询，业务处理保持运行。
5. 删除或重命名含义不准确的 `RadioStandby`，不得因此关闭 LoRa RX。
6. 最后评估是否精简 `PowerListener`、`PowerFault` 等低收益结构。

### 9.1 MCU 唤醒规则

Compass 的 demand mask 归零后保留 1500 ms 采样宽限期，再停止 I2C 采样、姿态算法和 UI 通知；GPS demand 归零后同样保留默认 1500 ms 的供电宽限期。宽限期内若有新需求则不发生硬件状态抖动。Compass 延迟只作用于采样门控，不切断与 GPS 共享的传感器电源轨。

外部中断是主要唤醒方式，包括 LoRa DIO1、按键、Touch IRQ、GPS UART/PPS 和电源事件。
但 Team heartbeat、Join 超时、Mesh 延迟发送、联系人延迟保存和资源关闭可能没有硬件中断，
因此必须同时保留一次性定时唤醒：

```text
硬件中断唤醒 + 固定短周期 timer wake
```

ESP32 无活动时固定进入 250 ms light sleep，LoRa、按键和 Touch GPIO 可提前唤醒。该周期足以推进秒级 Team、Join、Mesh 和资源关闭计时，并避免在多个模块之间传播 deadline 查询接口。nRF52 保持原生 SoftDevice/WFE 事件等待，不额外引入未验证的板级定时器抽象。

## 10. 验证清单

- GPS Track 开启、熄屏后仍持续记录。
- Team 位置共享开启、熄屏后仍按协议发送位置。
- Find Friend 离开页面或熄屏后仍保持 GPS 和 Compass。
- Compass 页面退出且无其他需求时停止 50 ms 采样。
- 校准 Overlay 打开期间 Compass 不会被暂停。
- ESP32 可由 LoRa DIO1、两个物理按键和触摸 IRQ 唤醒。
- nRF52840 WFE/System ON idle 后仍可接收 LoRa、按键和 GPS 数据。
- 普通休眠不丢失 LoRa 数据；主动关机才关闭 Radio。
- ESP32 无 LoRa/按键活动时，最多 250 ms 后由 timer 唤醒处理软件计时。
- `millis()` 回绕时显示超时、GPS 重试和关机超时仍正确。
- 对 T1、V4 R8 TFT 分别比较待机电流、Flash 和 RAM。
