from pathlib import Path
from datetime import date

from docx import Document
from docx.enum.section import WD_ORIENT
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "docs" / "deliverables"
TODAY = "2026-08-19"


def set_font(run, name="Microsoft YaHei", size=10.5, bold=False, color=None):
    run.font.name = name
    run._element.rPr.rFonts.set(qn("w:eastAsia"), name)
    run._element.rPr.rFonts.set(qn("w:ascii"), "Arial")
    run._element.rPr.rFonts.set(qn("w:hAnsi"), "Arial")
    run.font.size = Pt(size)
    run.bold = bold
    if color:
        run.font.color.rgb = RGBColor(*color)


def shade(cell, color):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:fill"), color)
    tc_pr.append(shd)


def cell_text(cell, text, size=9, bold=False, color=None, align=WD_ALIGN_PARAGRAPH.LEFT):
    cell.text = ""
    paragraph = cell.paragraphs[0]
    paragraph.alignment = align
    paragraph.paragraph_format.space_after = Pt(0)
    paragraph.paragraph_format.space_before = Pt(0)
    for index, part in enumerate(str(text).split("\n")):
        if index:
            paragraph.add_run().add_break()
        run = paragraph.add_run(part)
        set_font(run, size=size, bold=bold, color=color)
    cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER


def set_repeat_table_header(row):
    tr_pr = row._tr.get_or_add_trPr()
    element = OxmlElement("w:tblHeader")
    element.set(qn("w:val"), "true")
    tr_pr.append(element)


def prevent_row_split(row):
    tr_pr = row._tr.get_or_add_trPr()
    element = OxmlElement("w:cantSplit")
    tr_pr.append(element)


def set_cell_width(cell, width_cm):
    tc_pr = cell._tc.get_or_add_tcPr()
    tc_w = tc_pr.find(qn("w:tcW"))
    if tc_w is None:
        tc_w = OxmlElement("w:tcW")
        tc_pr.append(tc_w)
    tc_w.set(qn("w:w"), str(int(width_cm * 567)))
    tc_w.set(qn("w:type"), "dxa")


def add_page_number(paragraph):
    paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = paragraph.add_run("第 ")
    set_font(run, size=8)
    field = OxmlElement("w:fldSimple")
    field.set(qn("w:instr"), "PAGE")
    paragraph._p.append(field)
    run = paragraph.add_run(" 页")
    set_font(run, size=8)


def setup_document(doc, landscape=False):
    section = doc.sections[0]
    if landscape:
        section.orientation = WD_ORIENT.LANDSCAPE
        section.page_width, section.page_height = section.page_height, section.page_width
        section.left_margin = Cm(1.35)
        section.right_margin = Cm(1.35)
        section.top_margin = Cm(1.45)
        section.bottom_margin = Cm(1.35)
    else:
        section.left_margin = Cm(2.4)
        section.right_margin = Cm(2.1)
        section.top_margin = Cm(2.2)
        section.bottom_margin = Cm(2.0)
    section.header_distance = Cm(1.0)
    section.footer_distance = Cm(1.0)

    normal = doc.styles["Normal"]
    normal.font.name = "Microsoft YaHei"
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), "Microsoft YaHei")
    normal.font.size = Pt(10.5)
    normal.paragraph_format.line_spacing = 1.35
    normal.paragraph_format.space_after = Pt(4)

    for style_name, size, color in (("Heading 1", 16, (31, 78, 121)), ("Heading 2", 13, (47, 84, 150)), ("Heading 3", 11, (31, 78, 121))):
        style = doc.styles[style_name]
        style.font.name = "Microsoft YaHei"
        style._element.rPr.rFonts.set(qn("w:eastAsia"), "Microsoft YaHei")
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = RGBColor(*color)
        style.paragraph_format.space_before = Pt(12)
        style.paragraph_format.space_after = Pt(5)
        style.paragraph_format.keep_with_next = True

    header = section.header.paragraphs[0]
    header.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    header_run = header.add_run("TLS401 RFID + CCD 联动演示项目")
    set_font(header_run, size=8.5, color=(100, 100, 100))
    add_page_number(section.footer.paragraphs[0])


def add_title(doc, title, subtitle):
    for _ in range(5):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_after = Pt(18)
    run = p.add_run(title)
    set_font(run, size=24, bold=True, color=(31, 78, 121))
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run(subtitle)
    set_font(run, size=14, color=(89, 89, 89))
    doc.add_paragraph()
    table = doc.add_table(rows=4, cols=2)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = "Table Grid"
    for row, (key, value) in zip(table.rows, [
        ("项目名称", "TLS401 RFID + CCD 联动演示项目"),
        ("文档版本", "V1.0"),
        ("编制日期", TODAY),
        ("文档状态", "项目交付文档"),
    ]):
        shade(row.cells[0], "D9EAF7")
        cell_text(row.cells[0], key, size=10, bold=True, align=WD_ALIGN_PARAGRAPH.CENTER)
        cell_text(row.cells[1], value, size=10)
    doc.add_page_break()


def add_heading(doc, text, level=1):
    paragraph = doc.add_heading(text, level=level)
    return paragraph


def add_body(doc, text):
    p = doc.add_paragraph()
    p.paragraph_format.first_line_indent = Cm(0.74)
    p.paragraph_format.line_spacing = 1.35
    run = p.add_run(text)
    set_font(run, size=10.5)
    return p


def add_bullet(doc, text):
    p = doc.add_paragraph(style="List Bullet")
    p.paragraph_format.line_spacing = 1.25
    run = p.add_run(text)
    set_font(run, size=10.5)
    return p


def add_standard_table(doc, headers, rows, widths=None, font_size=9):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = "Table Grid"
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    for index, header in enumerate(headers):
        cell = table.rows[0].cells[index]
        shade(cell, "4472C4")
        cell_text(cell, header, size=font_size, bold=True, color=(255, 255, 255), align=WD_ALIGN_PARAGRAPH.CENTER)
        if widths:
            set_cell_width(cell, widths[index])
    set_repeat_table_header(table.rows[0])
    for values in rows:
        row = table.add_row()
        prevent_row_split(row)
        for index, value in enumerate(values):
            cell_text(row.cells[index], value, size=font_size)
            if widths:
                set_cell_width(row.cells[index], widths[index])
    doc.add_paragraph()
    return table


def build_design_doc():
    doc = Document()
    setup_document(doc)
    add_title(doc, "软件概要设计说明书", "TLS401 RFID + CCD 联动演示项目")

    add_heading(doc, "目录", 1)
    for item in [
        "1 引言", "2 总体设计", "3 功能模块设计", "4 数据设计", "5 接口与部署设计", "6 关键业务流程", "7 异常处理与安全性", "8 已知限制与后续扩展", "9 测试关联",
    ]:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Cm(0.74)
        run = p.add_run(item)
        set_font(run, size=10.5)
    doc.add_page_break()

    add_heading(doc, "1 引言", 1)
    add_heading(doc, "1.1 编写目的", 2)
    add_body(doc, "本文档说明 TLS401 RFID + CCD 联动演示项目的总体结构、模块职责、数据存储、主要流程和运行限制，为项目演示、测试执行及后续维护提供依据。本文以仓库当前代码实现为准，不把尚未实现的产品需求描述为已交付能力。")
    add_heading(doc, "1.2 项目范围", 2)
    add_bullet(doc, "在一个 Qt 桌面程序中集成 RFID 识别、CCD 相机预览、四舱信息展示、16 孔采集、图片归档和历史回放。")
    add_bullet(doc, "以 RFID UID 为技术关联键，将标签资料与 1 至 4 号舱室建立唯一绑定。")
    add_bullet(doc, "按 4 -> 3 -> 2 -> 1 顺序执行演示版批量识别和拍照流程；每个舱室完成一轮 16 孔采集后进入下一舱室。")
    add_bullet(doc, "使用 SQLite 保存标签、绑定、轮次和图片索引；图像文件保存至应用数据目录。")
    add_heading(doc, "1.3 术语说明", 2)
    add_standard_table(doc, ["术语", "说明"], [
        ("舱室", "系统中的 1 至 4 号逻辑操作单元。"),
        ("标签 UID", "由 RFID 读取的标签唯一技术标识，用于本地关联和历史恢复。"),
        ("采集轮次", "同一培养皿的一次 16 孔完整采集，轮次号按 UID 递增。"),
        ("孔位", "培养皿内 1 至 16 号固定位置。"),
        ("重拍", "对已有图像的孔位再次采集；旧图像失效，新图像作为有效版本。"),
        ("暂存图像", "轮次未完成时位于 staging 目录的 JPEG 图像。"),
    ], [3.2, 13.8])

    add_heading(doc, "2 总体设计", 1)
    add_heading(doc, "2.1 运行环境", 2)
    add_standard_table(doc, ["项目", "设计基线"], [
        ("操作系统", "Windows 10/11 64 位"),
        ("应用框架", "Qt 5.14.2 Widgets，C++17，CMake"),
        ("编译基线", "MSVC 2017 x86；RFID、CCD SDK 与主程序必须统一为 32 位"),
        ("数据库", "SQLite，通过 Qt Sql 访问，启用 foreign_keys、WAL 和 busy_timeout"),
        ("RFID", "厂商 RFID SDK，经 RealRfidService 封装"),
        ("CCD", "IDS Peak SDK，经 RealCameraService 封装"),
        ("界面样式", "Qt Widgets + QSS"),
    ], [4.0, 13.0])
    add_heading(doc, "2.2 分层架构", 2)
    add_body(doc, "系统采用 UI 层、应用层、设备层、存储层四层结构。UI 层只接收用户操作和展示状态；应用层协调识别和采集流程；设备层隔离 RFID/CCD 厂商 SDK；存储层负责 SQLite 与图像文件。")
    add_standard_table(doc, ["层级", "主要组件", "职责"], [
        ("UI 层", "MainWindow", "构建首页、培养皿详情和单孔详情页面；显示设备状态、孔位状态和历史图像。"),
        ("应用层", "ApplicationController", "初始化设备；调度批量识别、拍照队列、暂停恢复和异常终止。"),
        ("业务层", "ChamberSessionService、CaptureWorkflowService", "维护舱室会话、UID 绑定、轮次、孔位推进、重拍和提交校验。"),
        ("设备层", "IRfidService、ICameraService、RealRfidService、RealCameraService", "对上提供统一异步能力，对下封装厂商 SDK 调用。"),
        ("存储层", "Database、Repository、ImageFileStore", "管理 SQLite 表、业务映射、暂存/归档图片及文件清理。"),
    ], [2.2, 5.2, 9.6])
    add_heading(doc, "2.3 组件关系", 2)
    add_body(doc, "MainWindow 通过信号和控制器接口发起操作。ApplicationController 调用会话与采集服务，并订阅 RFID、CCD 的异步信号。业务服务依赖 Repository 和 ImageFileStore 完成持久化，设备服务不直接向 UI 暴露厂商 SDK 类型。")

    add_heading(doc, "3 功能模块设计", 1)
    module_rows = [
        ("M01", "应用启动与设备初始化", "创建 QApplication、加载 QSS、打开 SQLite、创建真实 RFID/CCD 服务并触发初始化。", "main.cpp、ApplicationController"),
        ("M02", "四舱会话与标签绑定", "维护 4 个舱室和当前选择；校验 UID 不可同时绑定多个舱室；重新识别同一 UID 时加载其历史轮次。", "ChamberSessionService"),
        ("M03", "RFID 识别", "接收 RFID 异步结果，解析有效标签资料，向会话服务提交绑定；批量识别顺序为 4、3、2、1。", "IRfidService、RealRfidService、RfidPayloadCodec"),
        ("M04", "CCD 预览与抓拍", "连接相机、接收预览帧、发起单帧抓拍，向控制器返回抓拍成功或失败信号。", "ICameraService、RealCameraService"),
        ("M05", "16 孔采集工作流", "创建活动轮次，从当前孔抓拍，保存成功后自动前进；16 孔完成后归档整个轮次。", "CaptureWorkflowService"),
        ("M06", "图片和数据库持久化", "图像先写入 staging，再在轮次完成时复制到 images；数据库记录轮次、图片路径和相机参数。", "ImageFileStore、Repository、Database"),
        ("M07", "历史浏览", "培养皿页展示轮次的 16 孔有效图像；单孔页按时间回放有效历史图像。", "MainWindow、Repository"),
    ]
    add_standard_table(doc, ["编号", "模块", "主要设计", "核心文件"], module_rows, [1.2, 3.2, 8.2, 4.4], font_size=8.5)
    add_heading(doc, "3.1 四舱与 RFID 绑定", 2)
    add_body(doc, "ChamberSessionService 在内存中创建固定的 4 个 ChamberModel。选择舱室后，bindProfile 先检查当前进程内是否已有其他舱室使用同一 UID，再加载该 UID 的已完成历史，最后通过 Repository 写入数据库绑定。数据库写入成功后才更新内存模型，避免持久化失败导致界面状态与实际状态不一致。")
    add_heading(doc, "3.2 16 孔采集与重拍", 2)
    add_body(doc, "CaptureWorkflowService 每个活动轮次固定维护 16 个孔位历史容器。正常抓拍只允许写入空孔；重拍只允许写入已有图像的孔。提交前将舱室号、UID、轮次号、孔号、相机参数和图像冻结为 PendingCapture；提交时再次校验当前上下文，防止异步结果在舱室或孔位改变后写入错误对象。")
    add_heading(doc, "3.3 页面设计", 2)
    add_standard_table(doc, ["页面", "主要内容", "主要操作"], [
        ("首页", "四个舱室卡片、16 孔缩略图、RFID/CCD/数据库状态。", "选择舱室、开始识别、开始拍照、进入已绑定舱室详情。"),
        ("培养皿详情", "患者/培养皿信息、4x4 孔位网格、轮次滑块和播放控件。", "查看不同轮次、选择孔位、进入单孔详情。"),
        ("单孔详情", "单孔大图、历史回放、浏览/校准两个视图。", "切换孔位、播放历史、查看 CCD 实时预览。"),
    ], [3.0, 8.0, 5.9])

    add_heading(doc, "4 数据设计", 1)
    add_heading(doc, "4.1 领域数据对象", 2)
    add_standard_table(doc, ["对象", "关键字段", "用途"], [
        ("TagProfile", "uid、dishNumber、inseminationTime、femaleName、medicalRecordNumber、maleName、identifiedAt", "保存 RFID 标签资料和本地补充信息。"),
        ("ChamberModel", "number、profile、rounds", "表示一个逻辑舱室及其当前绑定和历史轮次。"),
        ("CaptureRound", "number、finished、history、persistentId", "表示一次 16 孔采集及其持久化主键。"),
        ("WellCapture", "image、sourceSize、capturedAt、active、available、exposureUs、gainDb", "表示某孔一次图像采集和可用性。"),
    ], [3.0, 8.4, 5.5])
    add_heading(doc, "4.2 SQLite 数据表", 2)
    add_standard_table(doc, ["表名", "用途", "约束要点"], [
        ("tag_profile", "保存标签资料。", "uid 为主键；识别重复 UID 时更新标签字段和最近识别时间。"),
        ("chamber_assignment", "保存 1 至 4 号舱室与标签的当前绑定。", "chamber_no 为主键；tag_uid 唯一，保证一个 UID 只绑定一个舱室。"),
        ("capture_round", "保存采集轮次。", "同一 UID 的 round_no 唯一；同一 UID 同时最多一个 in_progress 轮次。"),
        ("capture_image", "保存每张归档图像的索引和采集参数。", "well_no 范围 1 至 16；有效图像在 round_id、well_no、focal_layer 维度唯一。"),
    ], [3.2, 5.1, 8.6])
    add_heading(doc, "4.3 文件存储", 2)
    add_body(doc, "应用数据根目录下使用 tls401.sqlite 保存数据库，使用 staging 和 images 保存图片。未完成轮次的图像为 staging/{uid}/{日期}/round_{轮次}/well_{孔位}/...jpg；轮次完成后归档为 images/{uid}/{日期}/round_{轮次}/well_{孔位}/...jpg。UID 会被过滤为路径安全字符，图像文件名包含时间戳和 UUID。")

    add_heading(doc, "5 接口与部署设计", 1)
    add_heading(doc, "5.1 设备接口", 2)
    add_standard_table(doc, ["接口", "输入/输出", "说明"], [
        ("IRfidService", "initialize、recognize、cancel；stateChanged、recognized", "提供 RFID 初始化、识别取消和标准化结果信号。"),
        ("ICameraService", "connectDevice、startPreview、capture；previewFrame、captured、captureFailed", "提供 CCD 连接、实时预览和单帧抓拍。"),
        ("Repository", "标签、绑定、轮次、图片的读写方法", "屏蔽 SQL 细节，向业务层提供领域对象持久化。"),
    ], [3.2, 6.8, 6.9])
    add_heading(doc, "5.2 部署约束", 2)
    add_bullet(doc, "应用、Qt、RFID SDK 和 IDS Peak SDK 的位数必须一致，当前基线为 x86。")
    add_bullet(doc, "运行环境需要有效的 RFID 驱动、CCD 相机驱动和 SQLite Qt 驱动插件。")
    add_bullet(doc, "数据库和图像写入当前 Windows 用户的应用数据目录；该目录需要具备写入权限和足够空间。")

    add_heading(doc, "6 关键业务流程", 1)
    add_heading(doc, "6.1 批量识别流程", 2)
    add_body(doc, "用户点击开始识别 -> 清除本轮四舱当前绑定 -> 控制器设置顺序 [4, 3, 2, 1] -> 依次选中舱室并请求 RFID 识别 -> 成功结果写入标签表和舱室绑定表 -> 所有舱室完成后，仅当至少识别到一个标签时允许开始拍照。单个舱室识别失败会显示错误并继续下一舱室。")
    add_heading(doc, "6.2 拍照与归档流程", 2)
    add_body(doc, "用户点击开始拍照 -> 控制器选择已绑定舱室 -> 创建活动轮次 -> 等待有效 CCD 预览帧 -> 请求抓拍 -> 校验图像尺寸及 PendingCapture 上下文 -> 写入暂存 JPEG -> 更新当前孔位 -> 16 孔全部有效后逐孔复制到正式目录、写入 SQLite、完成轮次 -> 切换下一舱室。任一步失败时终止队列并丢弃当前未完成轮次的暂存数据。")
    add_heading(doc, "6.3 历史查看流程", 2)
    add_body(doc, "重新识别某个已有 UID 时，系统从 SQLite 读取其历史轮次和有效图像；培养皿详情页按轮次显示 16 孔有效图像；单孔详情页按轮次时间顺序形成该孔的回放序列。发现归档文件缺失或无法读取时，系统将该图像标记为不可用，不把无效文件当作正常历史显示。")

    add_heading(doc, "7 异常处理与安全性", 1)
    add_standard_table(doc, ["异常场景", "处理策略"], [
        ("未选择舱室即单次识别", "提示先选择舱室，不发起 RFID 请求。"),
        ("RFID 无标签、解析异常或设备错误", "显示错误提示；批量识别继续下一个舱室，已有已确认历史不会因本次失败被删除。"),
        ("同一 UID 重复绑定", "拒绝将同一 UID 同时绑定到两个舱室，并返回错误信息。"),
        ("CCD 抓拍失败或图像无效", "停止当前拍照队列；不提交该孔，不将孔标记完成。"),
        ("文件或数据库写入失败", "删除本次已归档文件、删除未完成数据库轮次并保留错误信息。"),
        ("页面切换发生在拍照期间", "暂停序列；在途结果会被丢弃或恢复后继续，避免写入当前页面以外的上下文。"),
    ], [5.1, 11.8])

    add_heading(doc, "8 已知限制与后续扩展", 1)
    add_bullet(doc, "本项目为硬件联动 Demo，不包含舱门传感器、温湿度控制、运动平台、网络同步、用户权限和审计日志。")
    add_bullet(doc, "校准界面已具备实时预览和轴参数控件布局，但 X/Y/Z/L 运动控制、曝光增益设置保存尚未形成设备控制闭环，不能作为已实现功能验收。")
    add_bullet(doc, "当前采集主流程使用 JPEG 暂存并最终归档 JPEG；若后续业务要求无损图像，应统一改为 PNG 或其他批准的无损格式。")
    add_bullet(doc, "自动序列由软件调度；若真实设备不具备自动切换孔位的硬件能力，应在产品化阶段增加人工确认或运动平台控制，避免把同一视野错误归属为多个孔位。")

    add_heading(doc, "9 测试关联", 1)
    add_body(doc, "配套的《系统测试用例》使用 GN、RF、CP、PS、HS、UI 等编号覆盖本说明书中的启动、识别、采集、持久化、历史查看和界面展示能力。已有 Qt Test 自动化测试位于 tests/tst_workflow.cpp，用于验证 UID 唯一性、16 孔完成、重拍、SQLite/图像持久化、部分轮次清理和历史恢复等核心规则。")

    output = OUT_DIR / "TLS401_RFID_CCD_联动演示项目_软件概要设计说明书_V1.0.docx"
    doc.save(output)
    return output


def case(case_id, section, title, category, precondition, procedure, input_data, expected):
    return (case_id, section, title, category, precondition, procedure, input_data, expected, "实际结果：待执行\n结论：待执行")


def build_test_doc():
    doc = Document()
    setup_document(doc, landscape=True)
    add_title(doc, "系统测试用例", "TLS401 RFID + CCD 联动演示项目")

    add_heading(doc, "1 测试说明", 1)
    add_standard_table(doc, ["项目", "说明"], [
        ("测试目标", "验证当前 Demo 的 RFID、CCD、四舱联动、16 孔采集、持久化和历史查看闭环。"),
        ("执行环境", "Windows 10/11；Qt 5.14.2 MSVC2017 x86；已部署 RFID SDK、IDS Peak SDK、有效设备驱动。"),
        ("数据要求", "至少准备一个合法 RFID 标签；如验证重复绑定，准备两个可被识别的舱室操作场景。"),
        ("结果填写", "测试人员执行后在“备注”列填入实际结果、日期、执行人和通过/失败结论。"),
        ("测试类型", "手工：在正式界面和设备环境执行；集成：可结合模拟服务或调试入口；自动化：Qt Test。"),
    ], [4.0, 20.0], font_size=9)
    add_heading(doc, "2 测试用例", 1)

    headers = ["Test Case ID", "Manual Section", "Test Case Title", "Test Category", "Pre-condition", "Procedure", "Input data", "Expected Result", "Remark"]
    widths = [2.0, 2.6, 3.3, 2.3, 4.0, 6.4, 3.2, 5.6, 3.3]

    groups = [
        ("2.1 通用与初始化", [
            case("GN001", "应用启动", "首次启动与页面加载", "手工", "已安装应用及所需运行库。", "1. 启动程序。\n2. 观察首页和顶部状态区域。", "无", "主窗口正常显示；首页展示 1 至 4 号舱室卡片；无崩溃。"),
            case("GN002", "数据初始化", "SQLite 数据库初始化", "手工", "以新的 Windows 用户或清空应用数据目录运行。", "1. 启动程序。\n2. 检查数据库状态。\n3. 关闭后再次启动。", "无", "数据库状态为可用；应用数据目录创建 tls401.sqlite；再次启动不重复报建表错误。"),
            case("GN003", "设备状态", "设备初始化状态展示", "手工", "RFID 与 CCD 均已连接并驱动正常。", "1. 启动程序。\n2. 等待设备初始化完成。", "已连接 RFID、CCD", "界面分别显示 RFID、CCD 的最终状态；状态与实际设备可用性一致。"),
            case("GN004", "设备状态", "设备不可用提示", "手工", "断开 RFID 或 CCD 其中一个设备。", "1. 启动程序。\n2. 观察状态和提示。", "缺少一台设备", "对应设备显示错误或离线；程序保持可操作且不崩溃。"),
        ]),
        ("2.2 RFID 与舱室", [
            case("RF001", "RFID 识别", "单舱成功识别与绑定", "手工", "RFID 正常；准备合法标签。", "1. 选择 1 号舱室。\n2. 执行识别。", "合法 UID 与合法载荷", "1 号舱室显示培养皿序号、姓名、病历号等标签资料；数据库保存标签和绑定关系。"),
            case("RF002", "RFID 识别", "未选择舱室时识别", "集成", "RFID 正常；当前未选择任何舱室。", "1. 调用单次识别入口。", "合法标签", "系统拒绝发起识别，并提示先选择舱室。"),
            case("RF003", "RFID 异常", "无标签识别", "手工", "RFID 正常；感应区域无标签。", "1. 选择舱室。\n2. 执行识别。", "无标签", "显示无标签或相应识别错误；不新增或错误覆盖标签绑定。"),
            case("RF004", "RFID 异常", "标签载荷格式异常", "集成", "可使用模拟 RFID 服务或异常测试标签。", "1. 返回非法长度或非法字段载荷。\n2. 执行识别。", "非法 RFID payload", "系统报告解析异常；不写入 tag_profile 和 chamber_assignment。"),
            case("RF005", "舱室绑定", "同一 UID 的唯一绑定", "自动化/集成", "已将 UID-A 绑定到 1 号舱室。", "1. 选择 2 号舱室。\n2. 尝试绑定 UID-A。", "UID-A", "操作失败并提示 UID 已绑定；UID-A 保持只属于 1 号舱室。"),
            case("RF006", "批量识别", "四舱批量识别顺序", "集成", "RFID 服务可记录当前选中舱室；四次识别结果可配置。", "1. 点击开始识别。\n2. 记录每次发起识别前的舱室号。", "4 次 RFID 响应", "识别顺序为 4、3、2、1；完成后至少成功一个标签时可开始拍照。"),
            case("RF007", "批量识别", "批量识别中单舱失败", "集成", "四次响应中至少一次返回失败。", "1. 点击开始识别。\n2. 令其中一次返回失败。", "3 次成功，1 次失败", "失败舱室显示错误提示；流程继续检查其余舱室；成功舱室仍完成绑定。"),
            case("RF008", "历史恢复", "重新识别加载历史", "自动化/集成", "UID-A 已有至少一个已完成采集轮次。", "1. 清除当前内存绑定或重启程序。\n2. 将 UID-A 识别到任一舱室。", "UID-A", "该舱室恢复 UID-A 的已完成轮次和可用图像，历史可浏览。"),
        ]),
        ("2.3 CCD 与 16 孔采集", [
            case("CP001", "拍照授权", "未完成识别不得开始拍照", "手工", "刚启动程序且未完成本次 RFID 识别。", "1. 观察开始拍照按钮状态。\n2. 尝试触发拍照。", "无", "拍照按钮不可用或系统拒绝执行，并提示先完成 RFID 识别。"),
            case("CP002", "CCD 预览", "相机实时预览", "手工", "CCD 正常连接。", "1. 进入单孔详情。\n2. 切换到位置校准视图。", "正常 CCD 图像", "校准视图显示持续更新的 CCD 实时画面；不产生采集轮次或图片归档。"),
            case("CP003", "采集轮次", "创建活动轮次", "自动化/集成", "已选中并绑定合法标签的舱室。", "1. 调用创建轮次。", "合法 TagProfile", "创建编号递增的活动轮次；当前孔位为 1；同一舱室不能同时创建两个活动轮次。"),
            case("CP004", "孔位推进", "单孔抓拍后自动前进", "自动化", "存在活动轮次，当前孔为 1。", "1. 提交一张至少 32x32 的有效图像。", "32x32 或更大 QImage", "1 号孔保存为完成，当前孔自动变为 2 号。"),
            case("CP005", "整轮采集", "16 孔完成并结束轮次", "自动化", "存在活动轮次和可用持久化目录。", "1. 连续提交 16 张有效图像。", "16 张有效图像", "轮次自动完成；每个孔位有一张有效图像；活动轮次结束。"),
            case("CP006", "拍照队列", "多舱自动拍照顺序", "集成", "四舱均已绑定；CCD 模拟服务可连续返回有效帧。", "1. 点击开始拍照。\n2. 记录每轮对应舱室。", "4 x 16 有效帧", "系统依次完成 4、3、2、1 号舱室的 16 孔轮次；全部完成后结束队列。"),
            case("CP007", "拍照暂停", "进入单孔页暂停与恢复", "集成", "拍照队列正在运行，尚未完成当前轮次。", "1. 进入单孔详情。\n2. 等待在途帧结束。\n3. 返回首页或浏览模式。", "有效 CCD 帧", "暂停期间不向当前轮次写入新图像；恢复后从上次成功后的下一个孔继续。"),
            case("CP008", "图像校验", "无效 CCD 图像处理", "集成", "拍照队列正在运行。", "1. 返回空图像或尺寸小于 32x32 的图像。", "空图像或 16x16 图像", "系统报告 CCD 图像异常；不标记当前孔完成；当前拍照队列结束。"),
            case("CP009", "抓拍异常", "CCD 抓拍失败处理", "集成", "拍照队列正在运行。", "1. 触发 captureFailed 信号。", "错误原因字符串", "显示拍照失败原因；不写入当前孔；当前拍照队列结束。"),
            case("CP010", "重拍", "已有孔位重拍替换", "自动化", "活动轮次的 1 号孔已有一张有效图像。", "1. 选择 1 号孔。\n2. 使用 retake=true 提交第二张图像。", "两张颜色不同的有效图像", "旧图像标记为非有效；新图像为有效版本；历史保留替换关系。"),
            case("CP011", "重拍", "空孔禁止重拍", "自动化", "活动轮次的 3 号孔尚无图像。", "1. 选择 3 号孔。\n2. 使用 retake=true 提交图像。", "有效图像", "操作被拒绝并给出错误，不写入 3 号孔。"),
            case("CP012", "失败补偿", "轮次中断清理暂存数据", "自动化/集成", "活动轮次已完成部分孔位且存在暂存 JPEG。", "1. 调用 discardActiveRound 或触发队列失败。", "已暂存图片", "内存中删除未完成轮次；对应 staging 图片被删除；数据库不保留该未完成轮次。"),
        ]),
        ("2.4 持久化与历史", [
            case("PS001", "图片归档", "完成轮次保存数据库和图片", "自动化", "可写入的临时应用数据目录。", "1. 完成一轮 16 孔采集。\n2. 查询 SQLite 并检查 images 目录。", "16 张有效图像", "存在 1 条 completed capture_round、16 条 capture_image 和 16 个归档 JPEG；图片路径可读取。"),
            case("PS002", "采集参数", "曝光和增益持久化", "自动化", "存在活动轮次。", "1. 以指定曝光和增益完成一轮。\n2. 查询 capture_image。", "exposure=12345.0，gain=4.5", "capture_image 中保存对应 exposure_us=12345.0 和 gain_db=4.5。"),
            case("PS003", "持久化失败", "归档或数据库失败回滚", "集成", "可模拟目录不可写或数据库插入失败。", "1. 完成到第 16 孔时触发存储失败。", "文件写入失败或 SQL 失败", "系统返回错误；已归档的本轮文件被清理；本轮数据库记录被删除；轮次不显示为完成。"),
            case("PS004", "文件可用性", "历史图像文件缺失", "集成", "数据库已有一张归档图像。", "1. 手动删除该归档 JPEG。\n2. 重新识别对应 UID 并加载历史。", "缺失的 file_path", "该图像标记为不可用；界面不将其作为可正常播放的历史图像。"),
            case("PS005", "数据清理", "清除采集历史保留标签绑定", "自动化", "已有标签绑定和至少一个完成轮次。", "1. 调用清除采集历史和图片存储。\n2. 查询数据库。", "无", "capture_round 与 capture_image 被清空；tag_profile 与 chamber_assignment 保留。"),
            case("HS001", "培养皿历史", "培养皿页轮次切换", "手工/集成", "同一 UID 至少有两轮已完成采集。", "1. 进入培养皿详情。\n2. 切换轮次滑块或上一轮/下一轮。", "两轮历史", "4x4 网格随轮次切换更新；每孔显示该轮次有效图像或明确空状态。"),
            case("HS002", "单孔历史", "单孔按时间回放", "手工/集成", "某孔在多轮中都有有效图像。", "1. 进入该孔详情。\n2. 点击播放。", "同一孔的多轮图像", "图像按轮次时间顺序播放；播放到最后一帧后停止并停留在最后一帧。"),
            case("HS003", "单孔历史", "无历史时的空状态", "手工", "选择尚未采集的孔或没有历史的 UID。", "1. 进入单孔详情。", "无图像记录", "显示暂无轮次/空状态；播放控件不可用或不产生异常。"),
        ]),
        ("2.5 界面与已有自动化测试", [
            case("UI001", "首页", "四舱选择状态与缩略图", "手工", "至少一个舱室已绑定并有完成轮次。", "1. 分别点击舱室卡片。\n2. 点击该卡片的孔位缩略图。", "已完成轮次", "选中舱室有明确状态；缩略图显示最近有效图像；点击后进入对应培养皿详情。"),
            case("UI002", "状态提示", "识别与拍照状态反馈", "手工", "设备可用。", "1. 发起批量识别。\n2. 发起拍照队列。", "正常设备响应", "顶部和舱室卡片显示识别中、拍照中、完成或异常等与流程一致的状态。"),
            case("UI003", "校准视图", "校准控件范围说明", "手工", "CCD 已连接。", "1. 进入位置校准视图。\n2. 操作 X/Y/Z/L、曝光和步长控件。", "无", "控件可显示和修改本地数值、实时预览可显示；不应宣称已驱动运动平台或保存相机参数。"),
            case("AT001", "自动化测试", "Qt Test 工作流回归", "自动化", "已配置 debug 构建和 Qt Test 运行环境。", "1. 执行 ctest --preset debug --output-on-failure。", "仓库 tests/tst_workflow.cpp", "工作流测试全部通过，覆盖 UID 唯一性、16 孔完成、重拍、持久化、暂存清理、历史恢复等规则。"),
        ]),
    ]

    for group_title, rows in groups:
        add_heading(doc, group_title, 2)
        add_standard_table(doc, headers, rows, widths, font_size=7.2)

    add_heading(doc, "3 执行记录", 1)
    add_standard_table(doc, ["执行日期", "执行人", "构建版本", "通过数", "失败数", "阻塞数", "备注"], [
        ("", "", "", "", "", "", ""),
        ("", "", "", "", "", "", ""),
    ], [3.0, 3.0, 4.5, 2.5, 2.5, 2.5, 8.0], font_size=8.5)

    output = OUT_DIR / "TLS401_RFID_CCD_联动演示项目_系统测试用例_V1.0.docx"
    doc.save(output)
    return output


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    design = build_design_doc()
    tests = build_test_doc()
    print(design)
    print(tests)


if __name__ == "__main__":
    main()
