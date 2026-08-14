param(
    [string]$OutputDir = (Join-Path $PSScriptRoot "..\docs\软著申报材料")
)

$ErrorActionPreference = "Stop"
$softwareName = "便携式离网网状通信终端软件 V1.0"

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$docxPath = Join-Path $OutputDir "便携式离网网状通信终端软件V1.0_使用与设计说明书.docx"
$pdfPath = Join-Path $OutputDir "便携式离网网状通信终端软件V1.0_使用与设计说明书.pdf"

$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
$word.ScreenUpdating = $false
$word.Options.Pagination = $false

function Add-Paragraph {
    param(
        [string]$Text,
        [int]$Size = 10,
        [bool]$Bold = $false,
        [int]$Alignment = 0,
        [int]$SpaceAfter = 6,
        [int]$FirstLineIndent = 21
    )
    $p = $script:doc.Content.Paragraphs.Add()
    $p.Range.Text = $Text
    $p.Range.Font.NameFarEast = "宋体"
    $p.Range.Font.Name = "Times New Roman"
    $p.Range.Font.Size = $Size
    $p.Range.Font.Bold = [int]$Bold
    $p.Alignment = $Alignment
    $p.Format.SpaceAfter = $SpaceAfter
    $p.Format.LineSpacingRule = 1
    $p.Format.FirstLineIndent = $FirstLineIndent
    $p.Range.InsertParagraphAfter()
}

function Add-Title {
    param([string]$Text, [int]$Level = 1)
    $size = if ($Level -eq 1) { 16 } elseif ($Level -eq 2) { 13 } else { 11 }
    Add-Paragraph -Text $Text -Size $size -Bold $true -Alignment 0 -SpaceAfter 10 -FirstLineIndent 0
}

function Add-PageBreak {
    $r = $script:doc.Content
    $r.Collapse(0)
    $r.InsertBreak(7)
}

function Add-Table {
    param([string[]]$Headers, [object[][]]$Rows, [int[]]$Widths = @())
    $range = $script:doc.Content
    $range.Collapse(0)
    $table = $script:doc.Tables.Add($range, $Rows.Count + 1, $Headers.Count)
    $table.Borders.Enable = 1
    $table.Range.Font.NameFarEast = "宋体"
    $table.Range.Font.Size = 9
    for ($c = 1; $c -le $Headers.Count; $c++) {
        $table.Cell(1, $c).Range.Text = $Headers[$c - 1]
        $table.Cell(1, $c).Range.Font.Bold = 1
        $table.Cell(1, $c).Shading.BackgroundPatternColor = 14277081
        if ($Widths.Count -ge $c) { $table.Columns.Item($c).PreferredWidth = $Widths[$c - 1] }
    }
    for ($r = 1; $r -le $Rows.Count; $r++) {
        for ($c = 1; $c -le $Headers.Count; $c++) {
            $table.Cell($r + 1, $c).Range.Text = [string]$Rows[$r - 1][$c - 1]
        }
    }
    $table.Range.ParagraphFormat.SpaceAfter = 2
    $table.Rows.AllowBreakAcrossPages = 0
    $after = $script:doc.Content.Paragraphs.Add()
    $after.Range.InsertParagraphAfter()
}

try {
    $script:doc = $word.Documents.Add()
    $section = $doc.Sections.Item(1)
    $section.PageSetup.TopMargin = $word.CentimetersToPoints(2.54)
    $section.PageSetup.BottomMargin = $word.CentimetersToPoints(2.54)
    $section.PageSetup.LeftMargin = $word.CentimetersToPoints(3.0)
    $section.PageSetup.RightMargin = $word.CentimetersToPoints(2.5)
    $section.Headers.Item(1).Range.Text = ""
    $section.Footers.Item(1).Range.Text = ""

    Add-Paragraph -Text "" -Size 12 -SpaceAfter 48 -FirstLineIndent 0
    Add-Paragraph -Text $softwareName -Size 22 -Bold $true -Alignment 1 -SpaceAfter 30 -FirstLineIndent 0
    Add-Paragraph -Text "使用与设计说明书" -Size 20 -Bold $true -Alignment 1 -SpaceAfter 80 -FirstLineIndent 0
    Add-Paragraph -Text "软件类别：嵌入式终端应用软件" -Size 12 -Alignment 1 -SpaceAfter 12 -FirstLineIndent 0
    Add-Paragraph -Text "文档用途：计算机软件著作权登记材料" -Size 12 -Alignment 1 -SpaceAfter 12 -FirstLineIndent 0
    Add-Paragraph -Text "版本标识：V1.0" -Size 12 -Alignment 1 -FirstLineIndent 0

    Add-PageBreak
    Add-Title "目录"
    $tocItems = @(
        "1. 文档说明", "2. 软件概述", "3. 运行环境", "4. 总体功能结构", "5. 启动与初始化",
        "6. 首页与状态查看", "7. 消息通信", "8. 最近记录", "9. 无线参数设置", "10. 定位信息",
        "11. 地图浏览", "12. 指南针辅助", "13. 查找联系人", "14. 设备与系统设置", "15. 声光与交互反馈",
        "16. 数据管理与异常处理", "17. 安全与隐私说明", "18. 测试与验收"
    )
    foreach ($item in $tocItems) { Add-Paragraph -Text $item -Size 11 -FirstLineIndent 0 -SpaceAfter 5 }

    Add-PageBreak
    Add-Title "1. 文档说明"
    Add-Paragraph "本文档用于说明 $softwareName 的用途、运行条件、功能组成、操作方法、异常处理及验收要点。文档面向软件使用人员、测试人员和登记材料审核人员，不承担源程序说明功能。"
    Add-Paragraph "为保护软件实现细节，本文档不提供源程序、伪代码、函数名称、类名称、内部数据结构、关键技术参数、通信协议字段、加密过程、密钥内容及可还原核心逻辑的流程。所有示意均停留在用户可观察的功能层。"
    Add-Title "1.1 编写原则" 2
    Add-Table @("原则", "说明") @(
        @("名称一致", "全文仅使用统一的软件名称和版本标识。"),
        @("功能真实", "仅描述终端实际具备或可配置启用的用户功能。"),
        @("避免披露", "不引用仓库源码，不展示调试日志中的敏感内容。"),
        @("便于操作", "以任务为主线说明入口、步骤、结果和注意事项。")
    )
    Add-Title "1.2 术语说明" 2
    Add-Paragraph "节点：安装并运行本软件的便携式终端。联系人：已保存并可选择通信的目标节点。群组：用于多人消息交流的逻辑集合。中继：协助扩大无线消息可达范围的节点。离网：不依赖互联网或中心服务器即可完成本地无线通信的状态。"

    Add-PageBreak
    Add-Title "2. 软件概述"
    Add-Paragraph "$softwareName 是面向便携式低功耗终端的离网通信应用。软件通过图形界面集中提供设备状态查看、消息收发、联系人选择、无线配置、定位辅助、地图浏览及设备设置等能力，适用于户外协作、临时现场联络、应急保障和无公网覆盖区域的信息交换。"
    Add-Title "2.1 建设目标" 2
    Add-Paragraph "软件以操作直观、资源占用可控和环境适应性为目标，使用户在便携终端上完成从启动、状态确认、目标选择到消息发送和记录回看的完整工作过程。设备之间可在无线覆盖条件允许时直接交换信息，也可由网络中的其他节点协助转发。"
    Add-Title "2.2 主要特点" 2
    Add-Table @("特点", "用户价值") @(
        @("离网运行", "无需依赖公网账户或中心服务器即可执行基础通信。"),
        @("多入口交互", "支持按键、旋钮或触控等适配后的输入方式。"),
        @("状态集中展示", "在首页查看电量、无线、定位及连接状态。"),
        @("多种辅助工具", "提供地图、方向、定位和目标查找等户外功能。"),
        @("低功耗管理", "通过屏幕、背光和工作状态管理减少不必要耗电。")
    )

    Add-PageBreak
    Add-Title "3. 运行环境"
    Add-Paragraph "软件运行于具备微控制器、无线收发模块、显示屏和本地存储能力的便携式嵌入式终端。不同终端可选配定位模块、触摸屏、方向传感器、蜂鸣器、振动器、存储卡或外接天线。实际可用菜单依据硬件能力自动呈现。"
    Add-Title "3.1 硬件条件" 2
    Add-Table @("组成", "最低要求", "说明") @(
        @("处理单元", "支持嵌入式应用运行", "负责界面、设备管理与通信任务协调。"),
        @("无线单元", "具备兼容的低功耗远距离收发能力", "使用前需正确连接天线。"),
        @("显示与输入", "显示屏及至少一种本地输入装置", "输入形式随终端型号变化。"),
        @("本地存储", "可保存设置和必要记录", "容量由具体终端决定。"),
        @("供电", "电池或稳定外部电源", "建议首次使用前充分充电。")
    )
    Add-Title "3.2 使用前检查" 2
    Add-Paragraph "确认天线连接可靠，电池电量足够，显示与输入部件无明显损坏。若使用定位、地图或方向功能，应在空旷区域完成必要的定位等待、地图资源准备或传感器校准。"

    Add-PageBreak
    Add-Title "4. 总体功能结构"
    Add-Paragraph "软件采用面向任务的功能组织方式。用户通过首页进入通信、记录、无线、定位、地图、方向、目标查找和系统设置等模块；提示层用于确认、输入、告警和快捷操作，不改变用户当前主任务。"
    Add-Table @("一级功能", "二级功能", "输出结果") @(
        @("状态中心", "电量、无线、定位、连接概况", "当前设备运行摘要"),
        @("消息通信", "选择目标、编辑、发送、结果提示", "发送状态与消息记录"),
        @("记录管理", "最近消息、状态回看", "按时间排列的可读记录"),
        @("无线设置", "预设选择、参数确认、状态同步", "当前无线工作配置"),
        @("位置工具", "定位、地图、方向、目标查找", "位置与方向辅助信息"),
        @("系统管理", "显示、声音、电源、设备信息", "个性化设置及系统状态")
    )
    Add-Title "4.1 典型业务流" 2
    Add-Paragraph "开机 → 完成初始化 → 查看首页状态 → 选择通信目标 → 输入消息 → 确认发送 → 查看结果提示 → 在最近记录中回看。若通信条件不满足，用户可先调整位置、检查天线或核对无线配置，再重新执行发送。"

    Add-PageBreak
    Add-Title "5. 启动与初始化"
    Add-Paragraph "接通电源或按设备电源键后，软件进入启动阶段。启动画面用于提示系统正在准备运行环境。初始化完成后进入首页；若检测到需要用户处理的事项，软件以提示框说明原因及建议操作。"
    Add-Title "5.1 首次启动" 2
    Add-Table @("步骤", "操作", "预期结果") @(
        @("1", "连接天线并为终端供电", "设备进入启动画面。"),
        @("2", "等待自动初始化完成", "显示首页或必要的设置提示。"),
        @("3", "检查电量和无线状态", "状态图标无异常告警。"),
        @("4", "按需完成定位或方向校准", "对应功能具备可用状态。")
    )
    Add-Title "5.2 启动异常" 2
    Add-Paragraph "若长时间停留在启动画面，可先保持供电稳定并重新启动。若屏幕无显示，应检查电池、电源线和屏幕连接。若无线状态异常，应确认天线已连接且当前配置符合本地使用条件。重复出现异常时，应保留现象描述并停止反复写入设置。"

    Add-PageBreak
    Add-Title "6. 首页与状态查看"
    Add-Paragraph "首页是软件的主要入口，用于集中呈现设备名称、当前状态和常用功能。顶部状态区域可显示电量、无线活动、定位状态或其他设备能力；主体区域提供模块入口；底部或侧边区域根据硬件显示操作提示。"
    Add-Title "6.1 基本操作" 2
    Add-Table @("动作", "按键/触控设备的一般表现", "说明") @(
        @("移动焦点", "切换到相邻功能项", "当前选中项以高亮或边框表示。"),
        @("确认", "打开选中的页面或执行已确认操作", "危险操作会再次询问。"),
        @("返回", "回到上一层页面", "未确认的临时输入可能被放弃。"),
        @("长按或快捷动作", "打开快捷功能", "具体动作以屏幕提示为准。")
    )
    Add-Title "6.2 状态判断" 2
    Add-Paragraph "电量较低时应及时充电；无线状态不可用时不建议连续发送；定位状态未就绪时，位置相关页面可能显示等待或无数据；外部连接状态变化时，页面会在下一次刷新时更新。"

    Add-PageBreak
    Add-Title "7. 消息通信"
    Add-Paragraph "消息通信模块用于向联系人或群组发送短文本，并向用户反馈待发送、发送中、已提交或失败等结果。软件只在界面层展示必要状态，不在本文档中描述消息编码、路由选择或安全处理的内部实现。"
    Add-Title "7.1 发送消息" 2
    Add-Table @("步骤", "操作内容", "结果") @(
        @("1", "从首页进入消息功能", "显示目标选择或编辑界面。"),
        @("2", "选择联系人或群组", "目标名称显示在编辑区域。"),
        @("3", "使用屏幕键盘或外部输入方式编辑文本", "输入内容可预览。"),
        @("4", "检查目标和内容后确认发送", "进入发送状态并给出提示。"),
        @("5", "返回记录页面查看结果", "可回看消息摘要和状态。")
    )
    Add-Title "7.2 快捷消息" 2
    Add-Paragraph "在支持快捷操作的终端上，用户可从首页或快捷层快速选择目标并发送预设含义的短消息。快捷操作适合需要减少输入时间的场景。执行前仍应确认目标，避免误发。"
    Add-Title "7.3 注意事项" 2
    Add-Paragraph "发送前应确认终端天线连接正常、无线配置一致且目标有效。发送状态表示本机处理结果，不等同于对方已经阅读。弱信号、遮挡、距离过远或配置不一致均可能造成延迟或失败。"

    Add-PageBreak
    Add-Title "8. 最近记录"
    Add-Paragraph "最近记录页面用于按时间顺序展示近期消息和事件摘要，帮助用户确认已执行的操作并快速返回相关目标。记录内容以便于识别为原则，不显示底层报文或内部调试数据。"
    Add-Title "8.1 查看记录" 2
    Add-Paragraph "从首页选择【最近记录】入口，使用方向输入浏览列表。选中某条记录后，可查看目标、内容摘要和可用状态。记录数量受终端存储空间和软件策略限制，较早内容可能被自动替换。"
    Add-Title "8.2 记录保护" 2
    Add-Paragraph "终端可能包含个人通信内容，设备借用、送修或转交前应按实际需要清理记录和个性化信息。执行清理前应确认不再需要相关内容；清理后通常无法通过普通操作恢复。"
    Add-Table @("现象", "可能原因", "处理建议") @(
        @("未看到刚发送的记录", "页面尚未刷新或发送未完成", "返回首页后重新进入并等待状态更新。"),
        @("记录数量减少", "存储空间轮换或执行过清理", "确认存储策略并备份必要信息。"),
        @("内容显示不完整", "显示空间有限", "进入详情页查看可用的完整摘要。")
    )

    Add-PageBreak
    Add-Title "9. 无线参数设置"
    Add-Paragraph "无线设置用于选择经过预先定义的工作方案并查看当前状态。为降低错误配置风险，普通用户应优先使用经确认的预设方案，不应在不了解当地规定和设备能力的情况下任意修改。"
    Add-Title "9.1 设置流程" 2
    Add-Table @("步骤", "操作", "系统反馈") @(
        @("1", "进入无线设置页面", "显示当前工作方案。"),
        @("2", "选择适用的区域或预设", "候选项高亮显示。"),
        @("3", "确认变更", "软件保存设置并执行状态同步。"),
        @("4", "返回状态页检查", "无线状态恢复为可用。")
    )
    Add-Title "9.2 合规提示" 2
    Add-Paragraph "无线设备的可用频段、发射条件和使用场景可能受所在地法规限制。用户应使用与设备销售区域及当地规定相符的配置。本文档不提供规避限制、提高发射能力或修改管制参数的方法。"

    Add-PageBreak
    Add-Title "10. 定位信息"
    Add-Paragraph "定位页面用于查看终端当前位置的可用状态及用户可理解的位置摘要。定位能力取决于终端是否配备相应模块、天线条件和所处环境。室内、峡谷、密集建筑或强干扰环境可能延长首次定位时间。"
    Add-Title "10.1 获取定位" 2
    Add-Paragraph "进入定位页面后，将终端置于较开阔位置并保持一段时间。页面由等待状态变为可用状态后，可查看位置、方向或精度类提示。若终端未配置定位硬件，对应入口可能隐藏或显示不可用。"
    Add-Title "10.2 隐私注意" 2
    Add-Paragraph "位置信息属于敏感个人信息。向他人发送或展示位置前，应确认接收对象和使用目的。设备转交前应检查是否保存了位置相关记录。软件提供的是辅助信息，不应作为人身安全、专业测绘或唯一导航依据。"
    Add-Table @("状态", "含义", "建议") @(
        @("等待", "尚未获得稳定位置", "移至开阔区域并继续等待。"),
        @("可用", "已获得可显示的位置摘要", "结合环境判断信息是否合理。"),
        @("不可用", "硬件缺失、关闭或环境不满足", "检查设备能力和设置。")
    )

    Add-PageBreak
    Add-Title "11. 地图浏览"
    Add-Paragraph "地图页面用于在终端上查看本地地图资源、当前位置和可用目标标记。地图数据可来自设备预置或外部存储，软件不依赖实时互联网地图服务即可显示已准备的内容。"
    Add-Title "11.1 使用方法" 2
    Add-Table @("操作", "效果", "注意事项") @(
        @("打开地图", "显示当前可用地图区域", "首次加载可能需要短暂等待。"),
        @("移动视图", "查看相邻区域", "超出资源范围时可能显示空白。"),
        @("调整比例", "改变可见范围和细节", "终端性能有限时应避免频繁快速操作。"),
        @("返回当前位置", "视图重新定位到当前位置", "需要有效定位状态。")
    )
    Add-Title "11.2 地图资源" 2
    Add-Paragraph "地图资源应通过合法渠道获得并按设备支持的方式准备。为保护软件核心实现，本文档不列出内部文件组织、索引方式或解析过程。地图仅用于辅助判断，实际行动应结合现场标识和可靠导航资料。"

    Add-PageBreak
    Add-Title "12. 指南针辅助"
    Add-Paragraph "指南针页面用于显示设备朝向或目标的大致方位。终端具备相应传感器时可提供实时方向辅助；不具备传感器时，软件可能依据可用运动信息提供有限提示，或直接标记该功能不可用。"
    Add-Title "12.1 校准与使用" 2
    Add-Paragraph "在远离强磁体、车辆和大面积金属结构的位置打开方向功能。若页面提示校准，应按屏幕说明平稳转动设备。校准完成后保持设备姿态稳定，再读取方向信息。"
    Add-Title "12.2 误差来源" 2
    Add-Table @("来源", "表现", "处理") @(
        @("磁性物体", "方向持续偏转", "远离干扰源后重新校准。"),
        @("设备姿态变化", "读数快速波动", "保持终端平稳。"),
        @("传感器未就绪", "页面显示等待或不可用", "检查硬件能力并重新进入页面。"),
        @("目标位置无效", "无法显示目标方向", "先更新本机及目标位置。")
    )

    Add-PageBreak
    Add-Title "13. 查找联系人"
    Add-Paragraph "查找联系人功能用于从已知目标中选择联系人，并结合可用的位置和方向信息给出距离或方位辅助。该功能不保证实时追踪，也不会在本文档中披露目标识别和数据交换的内部规则。"
    Add-Title "13.1 操作步骤" 2
    Add-Table @("步骤", "说明") @(
        @("选择目标", "从可用联系人列表中选择需要查找的对象。"),
        @("检查信息", "确认本机与目标均存在可用的位置摘要。"),
        @("查看方向", "根据页面提示调整朝向并观察距离或方位。"),
        @("必要时刷新", "等待新的有效信息后重新判断。")
    )
    Add-Title "13.2 使用边界" 2
    Add-Paragraph "目标信息可能因无线条件、设备休眠、位置更新间隔或环境遮挡而存在延迟。用户不得将该功能作为搜救、医疗或生命安全场景中的唯一判断依据。未经对方同意，不应持续收集或传播其位置。"

    Add-PageBreak
    Add-Title "14. 设备与系统设置"
    Add-Paragraph "系统设置用于调整显示、输入、时间、电源和设备相关选项，并查看必要的运行信息。不同硬件提供的设置项可能不同；软件仅显示当前设备支持的内容。"
    Add-Title "14.1 常用设置" 2
    Add-Table @("设置类别", "可调整内容", "生效方式") @(
        @("显示", "亮度、熄屏或界面表现", "确认后立即或下次唤醒生效。"),
        @("输入", "按键、旋钮或触摸相关选项", "保存后用于后续操作。"),
        @("时间", "本地显示所需的时间类设置", "保存后刷新页面。"),
        @("电源", "休眠、唤醒及节能偏好", "满足条件时自动执行。"),
        @("设备信息", "版本、能力和运行状态摘要", "只读查看。")
    )
    Add-Title "14.2 恢复操作" 2
    Add-Paragraph "若提供恢复默认设置功能，执行前应确认已记录必要配置。恢复操作可能清除个性化设置、联系人或本地记录，具体以确认框提示为准。恢复过程中应保持供电稳定，完成后重新启动并逐项检查。"

    Add-PageBreak
    Add-Title "15. 声光与交互反馈"
    Add-Paragraph "软件可通过屏幕高亮、提示框、背光变化、蜂鸣或振动反馈操作结果。实际反馈形式取决于终端硬件配置和用户设置。安静环境、夜间或低电量场景可适当降低声音和屏幕亮度。"
    Add-Title "15.1 提示级别" 2
    Add-Table @("级别", "典型表现", "用户动作") @(
        @("普通提示", "短暂文字或轻微反馈", "阅读后继续操作。"),
        @("确认提示", "要求确认或取消", "核对对象和影响后选择。"),
        @("异常提示", "突出显示并可能伴随声光反馈", "按提示检查状态。"),
        @("重要告警", "持续或重复提醒", "优先处理并避免继续高风险操作。")
    )
    Add-Title "15.2 背光与休眠" 2
    Add-Paragraph "终端在无操作一段时间后可降低背光或进入休眠，以减少耗电。通过支持的唤醒动作可恢复界面。休眠期间部分状态刷新可能降低频率，唤醒后应等待页面完成更新。"

    Add-PageBreak
    Add-Title "16. 数据管理与异常处理"
    Add-Paragraph "软件在本地保存运行所需的用户设置和必要记录。数据写入由软件统一管理，用户不需要直接访问内部文件。为避免损坏数据，保存设置、升级或恢复期间不应突然断电。"
    Add-Title "16.1 常见异常处理" 2
    Add-Table @("异常", "检查顺序", "建议处理") @(
        @("无法发送消息", "天线→电量→无线状态→目标→配置", "修正可见问题后再次发送。"),
        @("界面无响应", "等待任务完成→尝试返回→重新启动", "避免连续快速重复输入。"),
        @("定位不可用", "硬件能力→开关→环境→等待时间", "移至开阔区域后重试。"),
        @("地图空白", "资源是否存在→位置是否在范围内", "重新准备合法地图资源。"),
        @("设置未保存", "供电→存储状态→确认步骤", "稳定供电后重新设置。")
    )
    Add-Title "16.2 升级注意事项" 2
    Add-Paragraph "升级前应确认升级包来源可靠、设备型号匹配并保持电量充足。必要时先记录重要设置。升级过程中不要断电、拔除连接或同时执行其他操作。升级完成后检查软件名称、版本标识、无线状态和主要功能。"

    Add-PageBreak
    Add-Title "17. 安全与隐私说明"
    Add-Paragraph "软件面向离网通信场景，用户仍需对设备保管、通信对象、位置分享和本地记录负责。终端丢失可能导致已保存信息被他人查看，应结合使用场景采取物理保管和信息最小化措施。"
    Add-Title "17.1 安全使用要求" 2
    Add-Table @("要求", "说明") @(
        @("可靠来源", "仅使用来源明确且与设备匹配的软件包。"),
        @("最小披露", "消息中不发送无必要的身份、位置或敏感信息。"),
        @("对象确认", "发送前核对联系人或群组，避免误发。"),
        @("设备保管", "离开设备时采取适合场景的保管措施。"),
        @("合规使用", "遵守所在地无线电、隐私和数据管理规定。")
    )
    Add-Title "17.2 文档披露边界" 2
    Add-Paragraph "本说明书只描述用户可见行为和常规维护方法，不构成内部安全设计说明，也不公开任何能够直接复制核心能力的实现材料。需要提交其他登记材料时，应另行审查内容来源、权属和重复风险。"

    Add-PageBreak
    Add-Title "18. 测试与验收"
    Add-Paragraph "验收以用户可观察结果为依据，重点确认软件能够稳定启动、正确显示状态、完成主要页面导航、执行消息操作并保存必要设置。测试不要求查看源程序或内部调试接口。"
    Add-Title "18.1 功能验收表" 2
    Add-Table @("编号", "测试项目", "预期结果", "结论") @(
        @("T01", "冷启动与重复启动", "均可进入首页，无持续异常提示。", "□通过 □不通过"),
        @("T02", "首页导航", "各可用模块可进入并正常返回。", "□通过 □不通过"),
        @("T03", "消息编辑与发送", "可选择目标、编辑内容并获得结果提示。", "□通过 □不通过"),
        @("T04", "最近记录", "可查看近期消息或事件摘要。", "□通过 □不通过"),
        @("T05", "无线设置", "可选择合法预设并显示同步结果。", "□通过 □不通过"),
        @("T06", "定位与方向", "具备硬件时可显示相应状态。", "□通过 □不通过"),
        @("T07", "地图浏览", "存在资源时可加载、移动和返回当前位置。", "□通过 □不通过"),
        @("T08", "系统设置", "修改后可生效并在重启后按设计保留。", "□通过 □不通过"),
        @("T09", "低电量与休眠", "提示和节能行为符合设置。", "□通过 □不通过"),
        @("T10", "异常恢复", "常见异常经提示或重启后可恢复。", "□通过 □不通过")
    )
    Add-Title "18.2 验收结论" 2
    Add-Paragraph "当上述主要测试项目通过，且软件名称、版本标识、说明书内容与实际提交版本保持一致时，可判定本次文档对应的软件版本具备登记材料整理条件。提交前仍应对权属、名称、版本和材料页数进行最终复核。"

    $doc.Fields.Update() | Out-Null
    $doc.Repaginate()
    $doc.SaveAs2($docxPath, 16)
    $doc.ExportAsFixedFormat($pdfPath, 17)
    $pages = $doc.ComputeStatistics(2)
    $doc.Close($false)
    "DOCX=$docxPath"
    "PDF=$pdfPath"
    "PAGES=$pages"
}
finally {
    if ($script:doc) { try { $script:doc.Close($false) } catch {} }
    $word.Quit()
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
