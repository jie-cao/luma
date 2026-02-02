#ifdef LUMA_ENABLE_QT
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QSlider>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>

#include "engine/engine_facade.h"
#include "engine/foundation/log.h"
#include "engine/renderer/render_graph/render_graph.h"
#include "engine/renderer/rhi/dx12_backend.h"
#include "engine/scene/scene.h"
#include "engine/actions/action.h"

#include <memory>

class ViewportWidget : public QWidget {
    Q_OBJECT
public:
    explicit ViewportWidget(luma::EngineFacade* engine, QWidget* parent = nullptr)
        : QWidget(parent), engine_(engine) {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen, true);
        setUpdatesEnabled(false);
    }

    void initialize_backend() {
        if (backend_) return;
        HWND hwnd = reinterpret_cast<HWND>(winId());
        RECT rect{};
        GetClientRect(hwnd, &rect);
        const uint32_t width = rect.right - rect.left;
        const uint32_t height = rect.bottom - rect.top;

        luma::rhi::NativeWindow wnd{hwnd, width, height};
        backend_ = luma::rhi::create_dx12_backend(wnd);
        render_graph_ = std::make_unique<luma::render_graph::RenderGraph>(backend_);
        // Create a default RT resource description to illustrate RG resource usage.
        luma::render_graph::ResourceDesc desc{};
        desc.width = width;
        desc.height = height;
        color_rt_ = render_graph_->create_resource(desc);
    }

    void render(float t) {
        if (!backend_) return;
        const float r = 0.1f + 0.4f * std::sin(t);
        const float g = 0.2f + 0.3f * std::cos(t * 0.5f);
        const float b = 0.4f + 0.2f * std::sin(t * 0.7f);
        render_graph_->clear(r, g, b);
        render_graph_->execute();
        render_graph_->present();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (engine_) {
            luma::Action click{luma::ActionType::SetState, "Viewport", "Clicked", std::nullopt, 100};
            engine_->dispatch_action(click);
        }
        QWidget::mousePressEvent(event);
    }

    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        initialize_backend();
    }

private:
    std::shared_ptr<luma::rhi::Backend> backend_;
    std::unique_ptr<luma::render_graph::RenderGraph> render_graph_;
    luma::EngineFacade* engine_{nullptr};
    luma::render_graph::ResourceHandle color_rt_;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    luma::Scene scene;
    scene.add_node(luma::Node{
        .name = "MeshNode",
        .renderable = "asset_mesh_hero",
        .camera = std::nullopt,
        .transform = {}
    });
    scene.add_node(luma::Node{
        .name = "CameraNode",
        .renderable = {},
        .camera = luma::AssetID{"asset_camera_main"},
        .transform = {}
    });

    luma::EngineFacade engine;
    engine.load_scene(scene);
    engine.dispatch_action({luma::ActionType::ApplyLook, "look_qt_viewport", {}, std::nullopt, 1});
    engine.dispatch_action({luma::ActionType::SwitchCamera, "asset_camera_main", {}, std::optional<int>{1}, 2});

    QMainWindow window;
    window.setWindowTitle("Luma Creator (stub)");

    auto* central = new QWidget(&window);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* viewport = new ViewportWidget(&engine, central);
    layout->addWidget(viewport);
    window.setCentralWidget(central);

    auto makeDock = [&](const QString& title, const QString& text) {
        auto* dock = new QDockWidget(title, &window);
        dock->setWidget(new QLabel(text, dock));
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
    };

    // Assets panel
    auto* assetsDock = new QDockWidget("Assets", &window);
    auto* assetsList = new QListWidget(assetsDock);
    auto manifest = luma::asset_pipeline::build_demo_manifest();
    for (const auto& a : manifest.assets) {
        assetsList->addItem(QString::fromStdString(a.id));
    }
    QObject::connect(assetsList, &QListWidget::itemDoubleClicked, [&](QListWidgetItem* item) {
        const auto id = item->text().toStdString();
        if (id.rfind("look", 0) == 0) {
            engine.dispatch_action({luma::ActionType::ApplyLook, id, {}, std::nullopt, 202});
        } else if (id.find("_mat_") != std::string::npos) {
            engine.dispatch_action({luma::ActionType::SetMaterialVariant, id, "", std::optional<int>{0}, 203});
        } else {
            engine.dispatch_action({luma::ActionType::SetState, "Asset", id, std::nullopt, 204});
        }
    });
    assetsDock->setWidget(assetsList);
    window.addDockWidget(Qt::LeftDockWidgetArea, assetsDock);

    // Hierarchy panel
    auto* hierarchyDock = new QDockWidget("Hierarchy", &window);
    auto* tree = new QTreeWidget(hierarchyDock);
    tree->setHeaderHidden(true);
    for (const auto& [nodeId, node] : engine.scene().getAllNodes()) {
        auto* item = new QTreeWidgetItem(QStringList(QString::fromStdString(node.name)));
        item->setData(0, Qt::UserRole, QString::fromStdString(node.camera.value_or("")));
        tree->addTopLevelItem(item);
    }
    QObject::connect(tree, &QTreeWidget::itemDoubleClicked, [&](QTreeWidgetItem* item, int) {
        const auto camId = item->data(0, Qt::UserRole).toString().toStdString();
        if (!camId.empty()) {
            engine.dispatch_action({luma::ActionType::SwitchCamera, camId, {}, std::optional<int>{1}, 201});
        }
    });
    hierarchyDock->setWidget(tree);
    window.addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Inspector panel
    auto* inspectorDock = new QDockWidget("Inspector", &window);
    auto* inspectorWidget = new QWidget(inspectorDock);
    auto* inspectorLayout = new QVBoxLayout(inspectorWidget);
    auto* inspector = new QLabel("Inspector (live)", inspectorWidget);
    auto* paramLabel = new QLabel("Param Name:", inspectorWidget);
    auto* paramName = new QLineEdit(inspectorWidget);
    auto* paramSlider = new QSlider(Qt::Horizontal, inspectorWidget);
    paramSlider->setRange(0, 100);
    auto* variantLabel = new QLabel("Material Variant:", inspectorWidget);
    auto* variantIndex = new QSpinBox(inspectorWidget);
    variantIndex->setRange(0, 10);
    auto* paramTable = new QTableWidget(inspectorWidget);
    paramTable->setColumnCount(2);
    paramTable->setHorizontalHeaderLabels({"Name", "Value"});
    paramTable->horizontalHeader()->setStretchLastSection(true);
    auto* addParamBtn = new QPushButton("Add Param", inspectorWidget);
    inspectorLayout->addWidget(inspector);
    inspectorLayout->addWidget(paramLabel);
    inspectorLayout->addWidget(paramName);
    inspectorLayout->addWidget(paramSlider);
    inspectorLayout->addWidget(variantLabel);
    inspectorLayout->addWidget(variantIndex);
    inspectorLayout->addWidget(paramTable);
    inspectorLayout->addWidget(addParamBtn);
    inspectorWidget->setLayout(inspectorLayout);
    inspectorDock->setWidget(inspectorWidget);
    window.addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Timeline panel with slider to drive SetParameter/PlayAnimation
    auto* timelineDock = new QDockWidget("Timeline", &window);
    auto* timelineWidget = new QWidget(timelineDock);
    auto* timelineLayout = new QHBoxLayout(timelineWidget);
    timelineLayout->setContentsMargins(4, 4, 4, 4);
    auto* timelineSlider = new QSlider(Qt::Horizontal, timelineWidget);
    timelineSlider->setRange(0, 100);
    auto* playAnimBtn = new QPushButton("PlayAnim", timelineWidget);
    timelineLayout->addWidget(timelineSlider);
    timelineLayout->addWidget(playAnimBtn);
    timelineDock->setWidget(timelineWidget);
    window.addDockWidget(Qt::BottomDockWidgetArea, timelineDock);

    QObject::connect(timelineSlider, &QSlider::valueChanged, [&](int v) {
        const float t = v / 100.0f;
        engine.set_time(t);
        engine.dispatch_action({luma::ActionType::SetParameter, "TimelineCurve", std::to_string(t), std::nullopt, 204});
    });
    QObject::connect(playAnimBtn, &QPushButton::clicked, [&]() {
        engine.dispatch_action({luma::ActionType::PlayAnimation, "MeshNode", "clip_timeline", std::nullopt, 205});
    });

    auto* menu = window.menuBar();
    auto* fileMenu = menu->addMenu("File");
    auto* viewMenu = menu->addMenu("View");

    auto* exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, [&]() { QApplication::quit(); });

    auto* resetCam = viewMenu->addAction("Reset Camera");
    QObject::connect(resetCam, &QAction::triggered, [&]() {
        engine.dispatch_action({luma::ActionType::SwitchCamera, "asset_camera_main", {}, std::optional<int>{1}, 200});
    });

    window.resize(1280, 720);
    window.show();

    QString selectedAsset;
    QString selectedNode;
    QString selectedMaterial;
    bool populatingTable = false;

    QObject::connect(paramSlider, &QSlider::valueChanged, [&](int v) {
        if (paramName->text().isEmpty()) return;
        const float t = v / 100.0f;
        engine.dispatch_action({luma::ActionType::SetParameter,
                                paramName->text().toStdString(),
                                std::to_string(t),
                                std::nullopt,
                                301});
    });

    QObject::connect(variantIndex, qOverload<int>(&QSpinBox::valueChanged), [&](int idx) {
        if (selectedAsset.isEmpty()) return;
        engine.dispatch_action({luma::ActionType::SetMaterialVariant,
                                selectedAsset.toStdString(),
                                "",
                                idx,
                                302});
    });

    QObject::connect(tree, &QTreeWidget::itemClicked, [&](QTreeWidgetItem* item, int) {
        selectedNode = item->text(0);
    });

    QObject::connect(assetsList, &QListWidget::itemClicked, [&](QListWidgetItem* item) {
        selectedAsset = item->text();
        if (selectedAsset.contains("_mat_")) {
            selectedMaterial = selectedAsset;
            const auto* mat = engine.find_material(selectedMaterial.toStdString());
            variantIndex->setValue(mat ? mat->variant : 0);
            paramName->setText(selectedMaterial + "/param");

            populatingTable = true;
            paramTable->setRowCount(0);
            if (mat) {
                int row = 0;
                for (const auto& kv : mat->parameters) {
                    paramTable->insertRow(row);
                    paramTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(kv.first)));
                    paramTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(kv.second)));
                    ++row;
                }
            }
            populatingTable = false;
        } else {
            selectedMaterial.clear();
            paramTable->setRowCount(0);
            paramName->setText(selectedAsset);
        }
    });

    QObject::connect(paramTable, &QTableWidget::cellChanged, [&](int row, int col) {
        if (populatingTable) return;
        if (selectedMaterial.isEmpty()) return;
        auto* nameItem = paramTable->item(row, 0);
        auto* valueItem = paramTable->item(row, 1);
        if (!nameItem || !valueItem) return;
        const auto name = nameItem->text().toStdString();
        const auto val = valueItem->text().toStdString();
        engine.dispatch_action({luma::ActionType::SetParameter,
                                selectedMaterial.toStdString() + "/" + name,
                                val,
                                std::nullopt,
                                303});
    });

    QObject::connect(addParamBtn, &QPushButton::clicked, [&]() {
        if (selectedMaterial.isEmpty()) return;
        populatingTable = true;
        int row = paramTable->rowCount();
        const QString defaultName = QString("param_%1").arg(row);
        paramTable->insertRow(row);
        paramTable->setItem(row, 0, new QTableWidgetItem(defaultName));
        paramTable->setItem(row, 1, new QTableWidgetItem("0"));
        populatingTable = false;
        engine.dispatch_action({luma::ActionType::SetParameter,
                                selectedMaterial.toStdString() + "/" + defaultName.toStdString(),
                                "0",
                                std::nullopt,
                                304});
    });

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        engine.advance_time(0.016f);
        // Bind material params of the currently selected material (if any) to the render graph.
        if (!selectedMaterial.isEmpty()) {
            render_graph_->set_material_params(engine.material_params_copy(selectedMaterial.toStdString()));
        } else {
            render_graph_->set_material_params({});
        }
        viewport->render(engine.timeline().time());
        inspector->setText(QString::fromStdString(
            "Camera: " + (engine.scene().active_camera().has_value() ? *engine.scene().active_camera() : "<none>")
            + "\nLook: " + engine.look().id
            + "\nSelected Asset: " + selectedAsset.toStdString()
            + "\nSelected Node: " + selectedNode.toStdString()
            + "\nSelected Material Variant: "
            + (selectedMaterial.isEmpty()
                   ? "<none>"
                   : std::to_string(engine.find_material(selectedMaterial.toStdString())
                                        ? engine.find_material(selectedMaterial.toStdString())->variant
                                        : 0))));
    });
    timer.start(16);

    return app.exec();
}

#include "main.moc"
#else
int main() { return 0; }
#endif

