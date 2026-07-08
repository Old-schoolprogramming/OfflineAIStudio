#include "skillimportpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include <QFileDialog>
#include <QFrame>

SkillImportPage::SkillImportPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyTheme();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &SkillImportPage::onThemeChanged);
}

SkillImportPage::~SkillImportPage()
{
}

void SkillImportPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName("skillContent");
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(32, 32, 32, 32);
    contentLayout->setSpacing(0);

    setupUploadZone();
    setupSkillGrid();

    contentLayout->addWidget(m_uploadZone);

    // Path input row
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->setSpacing(12);
    pathLayout->setContentsMargins(0, 16, 0, 0);

    QLabel* pathLabel = new QLabel("或输入技能路径", m_contentWidget);
    pathLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    pathLayout->addWidget(pathLabel);

    m_pathEdit = new QLineEdit(m_contentWidget);
    m_pathEdit->setObjectName("pathInput");
    m_pathEdit->setPlaceholderText("例如: /path/to/skill-config.yaml");
    pathLayout->addWidget(m_pathEdit, 1);

    m_importBtn = new QPushButton("导入", m_contentWidget);
    m_importBtn->setObjectName("primaryButton");
    m_importBtn->setCursor(Qt::PointingHandCursor);
    connect(m_importBtn, &QPushButton::clicked, this, &SkillImportPage::onImportClicked);
    pathLayout->addWidget(m_importBtn);

    contentLayout->addLayout(pathLayout);

    // Divider
    QHBoxLayout* dividerLayout = new QHBoxLayout();
    dividerLayout->setContentsMargins(0, 32, 0, 24);
    dividerLayout->setSpacing(16);

    QFrame* leftLine = new QFrame(m_contentWidget);
    leftLine->setFrameShape(QFrame::HLine);
    leftLine->setObjectName("dividerLine");
    dividerLayout->addWidget(leftLine, 1);

    QLabel* dividerLabel = new QLabel("已安装的技能", m_contentWidget);
    dividerLabel->setStyleSheet("font-size: 13px; font-weight: 500; white-space: nowrap;");
    dividerLayout->addWidget(dividerLabel);

    QFrame* rightLine = new QFrame(m_contentWidget);
    rightLine->setFrameShape(QFrame::HLine);
    rightLine->setObjectName("dividerLine");
    dividerLayout->addWidget(rightLine, 1);

    contentLayout->addLayout(dividerLayout);
    contentLayout->addLayout(m_skillGrid);

    // Bottom info bar
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 24, 0, 0);
    bottomLayout->setSpacing(8);

    QLabel* countLabel = new QLabel("共 4 个技能 · 3 个已启用", m_contentWidget);
    countLabel->setStyleSheet("font-size: 12px;");
    bottomLayout->addWidget(countLabel);
    bottomLayout->addStretch();

    QPushButton* linkBtn = new QPushButton("从仓库获取更多技能 →", m_contentWidget);
    linkBtn->setObjectName("linkButton");
    linkBtn->setCursor(Qt::PointingHandCursor);
    bottomLayout->addWidget(linkBtn);

    contentLayout->addLayout(bottomLayout);
    contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);
}

void SkillImportPage::setupUploadZone()
{
    m_uploadZone = new QWidget(m_contentWidget);
    m_uploadZone->setObjectName("uploadZone");
    m_uploadZone->setCursor(Qt::PointingHandCursor);
    m_uploadZone->setFixedHeight(180);

    QVBoxLayout* zoneLayout = new QVBoxLayout(m_uploadZone);
    zoneLayout->setAlignment(Qt::AlignCenter);
    zoneLayout->setSpacing(12);

    QLabel* iconLabel = new QLabel(m_uploadZone);
    iconLabel->setPixmap(IconHelper::cloudUpload(48, QColor("#94A3B8")));
    iconLabel->setAlignment(Qt::AlignCenter);
    zoneLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("拖拽技能包到此处，或点击上传", m_uploadZone);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 600;");
    titleLabel->setAlignment(Qt::AlignCenter);
    zoneLayout->addWidget(titleLabel);

    QLabel* hintLabel = new QLabel("支持 .zip, .json, .yaml 格式的技能配置文件", m_uploadZone);
    hintLabel->setStyleSheet("font-size: 13px;");
    hintLabel->setAlignment(Qt::AlignCenter);
    zoneLayout->addWidget(hintLabel);

    QPushButton* selectBtn = new QPushButton("选择文件", m_uploadZone);
    selectBtn->setObjectName("outlineButton");
    selectBtn->setCursor(Qt::PointingHandCursor);
    connect(selectBtn, &QPushButton::clicked, this, &SkillImportPage::onUploadClicked);
    zoneLayout->addWidget(selectBtn, 0, Qt::AlignCenter);

    m_uploadZone->setToolTip("点击上传技能文件");
}

void SkillImportPage::setupSkillGrid()
{
    m_skillGrid = new QGridLayout();
    m_skillGrid->setSpacing(16);

    m_skills = {
        {"Python 代码助手", "v2.1.0", "智能 Python 代码生成、优化与调试辅助，支持主流框架", "代码生成", "#3B82F6, #F59E0B", true},
        {"数据分析技能", "v1.3.2", "自动数据清洗、可视化与统计分析，支持 Pandas 和 SQL", "数据分析", "#16A34A, #22D3EE", true},
        {"技术文档撰写", "v1.0.0", "根据代码仓库自动生成 API 文档、使用指南与变更日志", "文档撰写", "#F59E0B, #F97316", false},
        {"自动化工作流", "v3.2.1", "创建自定义自动化流水线，支持定时任务与事件触发", "自动化", "#8B5CF6, #6366F1", true}
    };

    for (int i = 0; i < m_skills.size(); ++i) {
        const auto& skill = m_skills[i];
        createSkillCard(skill.name, skill.version, skill.description,
                        skill.type, skill.gradient, skill.enabled);
    }
}

void SkillImportPage::createSkillCard(const QString& name, const QString& version,
                                      const QString& description, const QString& type,
                                      const QString& gradient, bool enabled)
{
    int row = m_skillGrid->count() / 2;
    int col = m_skillGrid->count() % 2;

    QWidget* card = new QWidget(m_contentWidget);
    card->setObjectName("skillCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    // Header
    QHBoxLayout* header = new QHBoxLayout();
    header->setSpacing(12);

    QWidget* iconWidget = new QWidget(card);
    iconWidget->setFixedSize(32, 32);
    iconWidget->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 8px;").arg(gradient.split(",")[0], gradient.split(",")[1]));
    header->addWidget(iconWidget);

    QLabel* nameLabel = new QLabel(name, card);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: 600;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    header->addWidget(nameLabel, 1);

    QLabel* versionLabel = new QLabel(version, card);
    versionLabel->setStyleSheet("font-size: 11px; font-weight: 500; padding: 1px 8px; border-radius: 999px;");
    versionLabel->setObjectName("versionBadge");
    header->addWidget(versionLabel);

    cardLayout->addLayout(header);

    // Description
    QLabel* descLabel = new QLabel(description, card);
    descLabel->setStyleSheet("font-size: 13px; line-height: 1.5;");
    descLabel->setWordWrap(true);
    cardLayout->addWidget(descLabel);

    // Footer
    QHBoxLayout* footer = new QHBoxLayout();
    footer->setSpacing(8);

    QLabel* typeBadge = new QLabel(type, card);
    typeBadge->setStyleSheet("font-size: 11px; font-weight: 500; padding: 2px 10px; border-radius: 999px;");
    typeBadge->setObjectName("typeBadge");
    footer->addWidget(typeBadge);
    footer->addStretch();

    QPushButton* configBtn = new QPushButton("配置", card);
    configBtn->setObjectName("ghostButton");
    configBtn->setFixedHeight(28);
    configBtn->setCursor(Qt::PointingHandCursor);
    connect(configBtn, &QPushButton::clicked, this, [this, name]() {
        emit skillConfigClicked(name);
    });
    footer->addWidget(configBtn);

    QPushButton* toggleBtn = new QPushButton(card);
    toggleBtn->setObjectName(enabled ? "toggleOn" : "toggleOff");
    toggleBtn->setFixedSize(44, 24);
    toggleBtn->setCursor(Qt::PointingHandCursor);
    toggleBtn->setCheckable(true);
    toggleBtn->setChecked(enabled);
    toggleBtn->setStyleSheet(enabled ?
        "QPushButton { background-color: #3B82F6; border-radius: 12px; }" :
        "QPushButton { background-color: #475569; border-radius: 12px; }");
    connect(toggleBtn, &QPushButton::toggled, this, [this, name, toggleBtn](bool checked) {
        toggleBtn->setObjectName(checked ? "toggleOn" : "toggleOff");
        toggleBtn->setStyleSheet(checked ?
            "QPushButton { background-color: #3B82F6; border-radius: 12px; }" :
            "QPushButton { background-color: #475569; border-radius: 12px; }");
        emit skillToggled(name, checked);
    });
    footer->addWidget(toggleBtn);

    cardLayout->addLayout(footer);

    m_skillGrid->addWidget(card, row, col);
}

void SkillImportPage::applyTheme()
{
    ThemeManager* tm = ThemeManager::instance();
    QString bg = tm->background().name();
    QString bg2 = tm->bgSecondary().name();
    QString bg3 = tm->bgTertiary().name();
    QString bdr = tm->border().name();
    QString pri = tm->primary().name();
    QString txtPri = tm->textPrimary().name();
    QString txtSec = tm->textSecondary().name();
    QString txtTer = tm->textTertiary().name();

    m_contentWidget->setStyleSheet(QString(
        "QWidget#skillContent { background-color: %1; }"
        "QWidget#uploadZone { background-color: %2; border: 2px dashed %4; border-radius: 16px; }"
        "QWidget#uploadZone:hover { border-color: %6; background-color: %3; }"
        "QLineEdit#pathInput { background-color: %5; color: %7; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        "QLineEdit#pathInput:focus { border-color: %6; }"
        "QPushButton#primaryButton { background-color: %6; color: white; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        "QPushButton#primaryButton:hover { background-color: %8; }"
        "QPushButton#outlineButton { background-color: transparent; color: %6; border: 1px solid %6; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        "QPushButton#outlineButton:hover { background-color: %6; color: white; }"
        "QPushButton#ghostButton { background-color: transparent; color: %9; border: 1px solid %4; border-radius: 6px; padding: 4px 12px; font-size: 12px; font-weight: 500; }"
        "QPushButton#ghostButton:hover { background-color: %3; border-color: %6; color: %7; }"
        "QPushButton#linkButton { background-color: transparent; color: %6; border: none; font-size: 13px; font-weight: 500; }"
        "QPushButton#linkButton:hover { text-decoration: underline; }"
        "QPushButton#toggleOn { background-color: %6; border-radius: 12px; }"
        "QPushButton#toggleOff { background-color: %4; border-radius: 12px; }"
        "QWidget#skillCard { background-color: %5; border: 1px solid %4; border-radius: 12px; }"
        "QWidget#skillCard:hover { border-color: %6; }"
        "QLabel#versionBadge { background-color: %3; color: %9; }"
        "QLabel#typeBadge { background-color: %3; color: %9; }"
        "QFrame#dividerLine { color: %4; }"
        "QLabel { color: %7; }"
    ).arg(bg, bg2, bg3, bdr, bg, pri, txtPri, txtSec, txtTer));
}

void SkillImportPage::onUploadClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择技能文件",
                                                QString(),
                                                "Skill Files (*.zip *.json *.yaml *.yml);;All Files (*)");
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
        emit skillImported(path);
    }
}

void SkillImportPage::onImportClicked()
{
    QString path = m_pathEdit->text().trimmed();
    if (!path.isEmpty()) {
        emit skillImported(path);
    }
}

void SkillImportPage::onThemeChanged()
{
    applyTheme();
}
