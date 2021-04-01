#ifdef USE_OPEN3D
#include "WindowPCscan.h"

namespace open3d
{
    namespace visualization
    {
        WindowPCscan::WindowPCscan(const std::string &title, int width, int height)
            : WindowO3D(title, width, height), impl_(new WindowPCscan::Impl())
        {
            Init();
        }

        void WindowPCscan::setCbResetPC(OnBtnClickedCb pCb, void *pPCV)
        {
            if (!pPCV)
                return;

            m_cbResetPC.m_pCb = pCb;
            m_cbResetPC.m_pPCV = pPCV;
        }

        void WindowPCscan::setCbResetPicker(OnBtnClickedCb pCb, void *pPCV)
        {
            if (!pPCV)
                return;

            m_cbResetPicker.m_pCb = pCb;
            m_cbResetPicker.m_pPCV = pPCV;
        }

        void WindowPCscan::setCbSavePC(OnBtnClickedCb pCb, void *pPCV)
        {
            if (!pPCV)
                return;

            m_cbSavePC.m_pCb = pCb;
            m_cbSavePC.m_pPCV = pPCV;
        }

        void WindowPCscan::Init()
        {
            m_strDevice = "CPU:0";
            m_cbResetPC.init();
            m_cbResetPicker.init();
            m_cbSavePC.init();

            auto &app = gui::Application::GetInstance();
            auto &theme = GetTheme();
            gui::Application::GetInstance().SetMenubar(NULL);

            // Create scene
            impl_->scene_wgt_ = std::make_shared<gui::SceneWidget>();
            impl_->scene_wgt_->SetScene(std::make_shared<rendering::Open3DScene>(GetRenderer()));
            impl_->scene_wgt_->SetOnSunDirectionChanged(
                [this](const Eigen::Vector3f &new_dir) {
                    auto lighting = impl_->settings_.model_.GetLighting(); // copy
                    lighting.sun_dir = new_dir.normalized();
                    impl_->settings_.model_.SetCustomLighting(lighting);
                });
            impl_->scene_wgt_->EnableSceneCaching(true);

            // Create light
            auto &settings = impl_->settings_;
            std::string resource_path = app.GetResourcePath();
            auto ibl_path = resource_path + "/default";
            auto *render_scene = impl_->scene_wgt_->GetScene()->GetScene();
            render_scene->SetIndirectLight(ibl_path);
            impl_->scene_wgt_->ShowSkybox(settings.model_.GetShowSkybox());

            // Create materials
            impl_->InitializeMaterials(GetRenderer(), resource_path);

            // Apply model settings (which should be defaults) to the rendering entities
            impl_->UpdateFromModel(GetRenderer(), false);

            // Setup UI
            const auto em = theme.font_size;
            const int lm = int(std::ceil(0.5 * em));
            const int grid_spacing = int(std::ceil(0.25 * em));

            AddChild(impl_->scene_wgt_);

            // Add settings widget
            const int separation_height = int(std::ceil(0.75 * em));
            // (we don't want as much left margin because the twisty arrow is the
            // only thing there, and visually it looks larger than the right.)
            const gui::Margins base_margins(int(std::round(0.5 * lm)), lm, lm, lm);
            settings.wgt_base = std::make_shared<gui::Vert>(0, base_margins);

            gui::Margins indent(em, 0, 0, 0);

            //Main controls
            auto pc_ctrls =
                std::make_shared<gui::CollapsableVert>("Main Controls", 0, indent);

            auto btnResetPC = std::make_shared<SmallButton>("Reset Point Cloud");
            btnResetPC->SetOnClicked([this]() {
                if (!m_cbResetPC.m_pCb)
                    return;
                m_cbResetPC.m_pCb(m_cbResetPC.m_pPCV);
            });

            auto btnResetPick = std::make_shared<SmallButton>("Reset Picker");
            btnResetPick->SetOnClicked([this]() {
                if (!m_cbResetPicker.m_pCb)
                    return;
                m_cbResetPicker.m_pCb(m_cbResetPicker.m_pPCV);
            });

            auto btnSavePC = std::make_shared<SmallButton>("Save Point Cloud");
            btnSavePC->SetOnClicked([this]() {
                if (!m_cbSavePC.m_pCb)
                    return;
                m_cbSavePC.m_pCb(m_cbSavePC.m_pPCV);
            });

            auto pc_resetPC = std::make_shared<gui::Horiz>(grid_spacing);
            pc_resetPC->AddStretch();
            pc_resetPC->AddChild(gui::Horiz::MakeCentered(btnResetPC));
            pc_resetPC->AddStretch();

            auto pc_resetPick = std::make_shared<gui::Horiz>(grid_spacing);
            pc_resetPick->AddStretch();
            pc_resetPick->AddChild(gui::Horiz::MakeCentered(btnResetPick));
            pc_resetPick->AddStretch();

            auto pc_savePC = std::make_shared<gui::Horiz>(grid_spacing);
            pc_savePC->AddStretch();
            pc_savePC->AddChild(gui::Horiz::MakeCentered(btnSavePC));
            pc_savePC->AddStretch();

            pc_ctrls->AddChild(pc_resetPC);
            pc_ctrls->AddFixed(separation_height);
            pc_ctrls->AddChild(pc_resetPick);
            pc_ctrls->AddFixed(separation_height);
            pc_ctrls->AddChild(pc_savePC);
            pc_ctrls->AddFixed(separation_height);

            settings.wgt_base->AddChild(pc_ctrls);

            // Mouse controls
            auto view_ctrls =
                std::make_shared<gui::CollapsableVert>("Mouse", 0, indent);

            // ... view manipulator buttons
            settings.wgt_mouse_arcball = std::make_shared<SmallToggleButton>("Arcball");
            impl_->settings_.wgt_mouse_arcball->SetOn(true);
            settings.wgt_mouse_arcball->SetOnClicked([this]() {
                impl_->SetMouseControls(*this,
                                        gui::SceneWidget::Controls::ROTATE_CAMERA);
            });
            settings.wgt_mouse_fly = std::make_shared<SmallToggleButton>("Fly");
            settings.wgt_mouse_fly->SetOnClicked([this]() {
                impl_->SetMouseControls(*this, gui::SceneWidget::Controls::FLY);
            });
            settings.wgt_mouse_model = std::make_shared<SmallToggleButton>("Model");
            settings.wgt_mouse_model->SetOnClicked([this]() {
                impl_->SetMouseControls(*this,
                                        gui::SceneWidget::Controls::ROTATE_MODEL);
            });
            settings.wgt_mouse_sun = std::make_shared<SmallToggleButton>("Sun");
            settings.wgt_mouse_sun->SetOnClicked([this]() {
                impl_->SetMouseControls(*this, gui::SceneWidget::Controls::ROTATE_SUN);
            });
            settings.wgt_mouse_ibl = std::make_shared<SmallToggleButton>("Environment");
            settings.wgt_mouse_ibl->SetOnClicked([this]() {
                impl_->SetMouseControls(*this, gui::SceneWidget::Controls::ROTATE_IBL);
            });

            auto reset_camera = std::make_shared<SmallButton>("Reset camera");
            reset_camera->SetOnClicked([this]() {
                impl_->scene_wgt_->GoToCameraPreset(
                    gui::SceneWidget::CameraPreset::PLUS_Z);
            });

            auto camera_controls1 = std::make_shared<gui::Horiz>(grid_spacing);
            camera_controls1->AddStretch();
            camera_controls1->AddChild(settings.wgt_mouse_arcball);
            camera_controls1->AddChild(settings.wgt_mouse_fly);
            camera_controls1->AddChild(settings.wgt_mouse_model);
            camera_controls1->AddStretch();
            auto camera_controls2 = std::make_shared<gui::Horiz>(grid_spacing);
            camera_controls2->AddStretch();
            camera_controls2->AddChild(settings.wgt_mouse_sun);
            camera_controls2->AddChild(settings.wgt_mouse_ibl);
            camera_controls2->AddStretch();

            view_ctrls->AddChild(camera_controls1);
            view_ctrls->AddFixed(int(std::ceil(0.25 * em)));
            view_ctrls->AddChild(camera_controls2);
            view_ctrls->AddFixed(separation_height);
            view_ctrls->AddChild(gui::Horiz::MakeCentered(reset_camera));

            settings.wgt_base->AddChild(view_ctrls);

            // ... lighting and materials
            settings.view_ = std::make_shared<GuiSettingsView>(
                settings.model_, theme, resource_path, [this](const char *name) {
                    std::string resource_path =
                        gui::Application::GetInstance().GetResourcePath();
                    impl_->SetIBL(GetRenderer(),
                                  resource_path + "/" + name + "_ibl.ktx");
                });
            settings.model_.SetOnChanged([this](bool material_type_changed) {
                impl_->settings_.view_->Update();
                impl_->UpdateFromModel(GetRenderer(), material_type_changed);
            });
            settings.wgt_base->AddChild(settings.view_);

            AddChild(settings.wgt_base);
        }

        WindowPCscan::~WindowPCscan() {}

        void WindowPCscan::SetTitle(const std::string &title)
        {
            Super::SetTitle(title.c_str());
        }

        void WindowPCscan::SetGeometry(std::shared_ptr<const geometry::Geometry> geometry)
        {
            auto scene3d = impl_->scene_wgt_->GetScene();
            scene3d->ClearGeometry();
            impl_->SetMaterialsToDefault();

            rendering::Material loaded_material;
            std::shared_ptr<const geometry::Geometry> g = geometry;

            if (g->GetGeometryType() ==
                geometry::Geometry::GeometryType::PointCloud)
            {
                auto pcd = std::static_pointer_cast<const geometry::PointCloud>(g);

                loaded_material.shader = "defaultUnlit";

                t::geometry::PointCloud tpcd = open3d::t::geometry::PointCloud::
                    FromLegacyPointCloud(
                        *pcd.get(),
                        core::Dtype::Float32,
                        core::Device(m_strDevice));
                scene3d->AddGeometry(MODEL_NAME,
                                     &tpcd,
                                     loaded_material,
                                     true);

                impl_->settings_.model_.SetDisplayingPointClouds(true);
                if (!impl_->settings_.model_.GetUserHasChangedLightingProfile())
                {
                    auto &profile =
                        GuiSettingsModel::GetDefaultPointCloudLightingProfile();
                    impl_->settings_.model_.SetLightingProfile(profile);
                }
            }

            auto type = impl_->settings_.model_.GetMaterialType();
            if (type == GuiSettingsModel::MaterialType::LIT ||
                type == GuiSettingsModel::MaterialType::UNLIT)
            {
                if (loaded_material.shader == "defaultUnlit")
                {
                    impl_->settings_.model_.SetMaterialType(
                        GuiSettingsModel::MaterialType::UNLIT);
                }
                else
                {
                    impl_->settings_.model_.SetMaterialType(
                        GuiSettingsModel::MaterialType::LIT);
                }
            }

            // Setup UI for loaded model/point cloud
            impl_->settings_.model_.UnsetCustomDefaultColor();
            impl_->settings_.view_->ShowFileMaterialEntry(false);
            impl_->settings_.view_->Update(); // make sure prefab material is correct

            auto &bounds = scene3d->GetBoundingBox();
            impl_->scene_wgt_->SetupCamera(60.0, bounds,
                                           bounds.GetCenter().cast<float>());

            // Make sure scene is redrawn
            impl_->scene_wgt_->ForceRedraw();
        }

        void WindowPCscan::Layout(const gui::Theme &theme)
        {
            auto r = GetContentRect();
            const auto em = theme.font_size;
            impl_->scene_wgt_->SetFrame(r);

            // Draw help keys HUD in upper left
            // const auto pref = impl_->help_keys_->CalcPreferredSize(theme);
            // impl_->help_keys_->SetFrame(gui::Rect(0, r.y, pref.width, pref.height));
            // impl_->help_keys_->Layout(theme);

            // Draw camera HUD in lower left
            // const auto prefcam = impl_->help_camera_->CalcPreferredSize(theme);
            // impl_->help_camera_->SetFrame(gui::Rect(0, r.height + r.y - prefcam.height,
            //                                         prefcam.width, prefcam.height));
            // impl_->help_camera_->Layout(theme);

            // Settings in upper right
            const auto LIGHT_SETTINGS_WIDTH = 18 * em;
            auto light_settings_size =
                impl_->settings_.wgt_base->CalcPreferredSize(theme);
            gui::Rect lightSettingsRect(r.width - LIGHT_SETTINGS_WIDTH, r.y,
                                        LIGHT_SETTINGS_WIDTH,
                                        std::min(r.height, light_settings_size.height));
            impl_->settings_.wgt_base->SetFrame(lightSettingsRect);

            Super::Layout(theme);
        }

        void WindowPCscan::ExportCurrentImage(const std::string &path)
        {
            impl_->scene_wgt_->EnableSceneCaching(false);
            impl_->scene_wgt_->GetScene()->GetScene()->RenderToImage(
                [this, path](std::shared_ptr<geometry::Image> image) mutable {
                    if (!io::WriteImage(path, *image))
                    {
                        this->ShowMessageBox(
                            "Error", (std::string("Could not write image to ") +
                                      path + ".")
                                         .c_str());
                    }
                    impl_->scene_wgt_->EnableSceneCaching(true);
                });
        }

        void WindowPCscan::OnMenuItemSelected(gui::Menu::ItemId item_id)
        {
            auto menu_id = MenuId(item_id);
            switch (menu_id)
            {
            case FILE_OPEN:
            {
                auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Mode::OPEN, "Open Geometry", GetTheme());
                dlg->AddFilter(".ply .stl .fbx .obj .off .gltf .glb",
                               "Triangle mesh files (.ply, .stl, .fbx, .obj, .off, "
                               ".gltf, .glb)");
                dlg->AddFilter(".xyz .xyzn .xyzrgb .ply .pcd .pts",
                               "Point cloud files (.xyz, .xyzn, .xyzrgb, .ply, "
                               ".pcd, .pts)");
                dlg->AddFilter(".ply", "Polygon files (.ply)");
                dlg->AddFilter(".stl", "Stereolithography files (.stl)");
                dlg->AddFilter(".fbx", "Autodesk Filmbox files (.fbx)");
                dlg->AddFilter(".obj", "Wavefront OBJ files (.obj)");
                dlg->AddFilter(".off", "Object file format (.off)");
                dlg->AddFilter(".gltf", "OpenGL transfer files (.gltf)");
                dlg->AddFilter(".glb", "OpenGL binary transfer files (.glb)");
                dlg->AddFilter(".xyz", "ASCII point cloud files (.xyz)");
                dlg->AddFilter(".xyzn", "ASCII point cloud with normals (.xyzn)");
                dlg->AddFilter(".xyzrgb",
                               "ASCII point cloud files with colors (.xyzrgb)");
                dlg->AddFilter(".pcd", "Point Cloud Data files (.pcd)");
                dlg->AddFilter(".pts", "3D Points files (.pts)");
                dlg->AddFilter("", "All files");
                dlg->SetOnCancel([this]() { this->CloseDialog(); });
                dlg->SetOnDone([this](const char *path) {
                    this->CloseDialog();
                    OnDragDropped(path);
                });
                ShowDialog(dlg);
                break;
            }
            case FILE_EXPORT_RGB:
            {
                auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Mode::SAVE, "Save File", GetTheme());
                dlg->AddFilter(".png", "PNG images (.png)");
                dlg->AddFilter("", "All files");
                dlg->SetOnCancel([this]() { this->CloseDialog(); });
                dlg->SetOnDone([this](const char *path) {
                    this->CloseDialog();
                    this->ExportCurrentImage(path);
                });
                ShowDialog(dlg);
                break;
            }
            case FILE_QUIT:
                gui::Application::GetInstance().Quit();
                break;
            case SETTINGS_LIGHT_AND_MATERIALS:
            {
                auto visibility = !impl_->settings_.wgt_base->IsVisible();
                impl_->settings_.wgt_base->SetVisible(visibility);
                auto menubar = gui::Application::GetInstance().GetMenubar();
                menubar->SetChecked(SETTINGS_LIGHT_AND_MATERIALS, visibility);

                // We need relayout because materials settings pos depends on light
                // settings visibility
                Layout(GetTheme());

                break;
            }
            }
        }

        void WindowPCscan::UpdateGeometry(std::shared_ptr<const geometry::PointCloud> sPC)
        {
            gui::Application::GetInstance().PostToMainThread(
                this, [this, sPC]() 
                {
                    t::geometry::PointCloud tpcd = open3d::t::geometry::PointCloud::
                        FromLegacyPointCloud(
                            *sPC.get(),
                            core::Dtype::Float32,
                            core::Device(m_strDevice));

                    impl_->scene_wgt_->GetScene()->GetScene()->UpdateGeometry(
                        MODEL_NAME,
                        tpcd,
                        open3d::visualization::rendering::Scene::kUpdatePointsFlag |
                            open3d::visualization::rendering::Scene::kUpdateColorsFlag);
                    impl_->scene_wgt_->ForceRedraw();
                });
        }

        void WindowPCscan::setDevice(const string &device)
        {
            m_strDevice = device;
        }

    }
}

#endif
