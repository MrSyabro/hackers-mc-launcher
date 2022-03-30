//
// Created by alex2 on 15.04.2021.
//

#include "LauncherSettingsWindow.h"
#include "MainWindow.h"
#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/View/ATabView.h>
#include <AUI/View/AText.h>
#include <AUI/View/APathChooserView.h>
#include <AUI/Json/AJson.h>
#include <Model/Settings.h>
#include <AUI/View/ARadioButton.h>
#include <AUI/View/ANumberPicker.h>
#include <AUI/View/ACheckBox.h>
#include <AUI/Platform/AMessageBox.h>
#include <AUI/Platform/ADesktop.h>
#include <AUI/ASS/ASS.h>

using namespace ass;

LauncherSettingsWindow::LauncherSettingsWindow() :
        AWindow("Settings", 400_dp, 400_dp, Autumn::get<MainWindow>().get(), WindowStyle::MODAL | WindowStyle::NO_RESIZE) {
    auto binding = _new<ADataBinding<Settings>>(Settings::inst());

    _<AView> resolutionView;
    _<ACheckBox> fullscreenCheckbox;

    setContents(
        Vertical {
            _new<ATabView>() let {
                // GAME TAB ============================================================================================
                it->addTab(
                    Vertical{
                        _form({
                            {"Game folder:"_as, _new<APathChooserView>() && binding(&Settings::gameDir)},
                            {
                                "Display:"_as,
                                Horizontal {
                                    resolutionView = Horizontal{
                                        _new<ANumberPicker>() && binding(&Settings::width),
                                        _new<ALabel>("x"),
                                        _new<ANumberPicker>() && binding(&Settings::height),
                                    },
                                    fullscreenCheckbox = _new<ACheckBox>("Fullscreen") && binding(&Settings::isFullscreen),
                                }
                            },
                        }),
                    }, "Game"
                );

                // ABOUT TAB ===========================================================================================
                it->addTab(
                    Vertical{
                        _new<ALabel>("Hacker's Minecraft Launcher") let {
                            it->setCustomAss({
                                FontSize { 19_pt },
                                Margin { 8_dp, 0, 4_dp },
                                TextAlign::CENTER,
                            });
                        },
                        _new<ALabel>("Version " HACKERS_MC_VERSION) let {
                            it->setCustomAss({
                                FontSize { 8_pt },
                                //Margin { 0, 0, 4_dp },
                                TextAlign::CENTER,
                                TextColor { 0x444444_rgb },
                            });
                        },
                        _new<ALabel>("Distributed under GNU General Public License v3") let {
                            it->setCustomAss({
                                FontSize { 8_pt },
                                Margin { 0, 0, 4_dp },
                                TextAlign::CENTER,
                                TextColor { 0x444444_rgb },
                            });
                        },
                        AText::fromString("Open source free Minecraft launcher that's designed to be independent of "
                                          "any third-party commercial organizations like Minecraft servers, hostings, "
                                          "launcher-based projects, even official Minecraft. Do not allow to feed "
                                          "yourself with terrible bullshit filled with annoying ads!"),
                        AText::fromString("The launcher is distributed under GNU General Public License (v3) and "
                                          "powered by open source software: zlib, minizip, Freetype, Boost, AUI."),
                        AText::fromString("Contributors: Alex2772"),
                        Horizontal {
                            _new<ASpacer>(),
                            _new<AButton>("View GNU General Public License v3").connect(&AButton::clicked, this, [] {
                                ADesktop::openUrl("https://www.gnu.org/licenses/gpl-3.0.html");
                            }),
                            _new<AButton>("Check for updates..."),
                        }
                    }, "About"
                );
            },
            _new<ASpacer>(),
            Horizontal {
                mResetButton = _new<AButton>("Reset to defaults").connect(&AButton::clicked, this, [this, binding] {
                    Settings::reset();
                    binding->setModel(Settings::inst());
                    mResetButton->setDisabled();
                }),
                _new<ASpacer>(),
                _new<AButton>("OK").connect(&AButton::clicked, this, [this, binding] {
                    Settings::inst() = binding->getModel();
                    auto s = Settings::inst();
                    Settings::save();
                    close();
                }) let { it->setDefault(); },
                _new<AButton>("Cancel").connect(&AView::clicked, this, [this, binding] {
                    if (binding->getEditableModel() != Settings::inst()) {
                        auto result = AMessageBox::show(this,
                                                        "Unsaved settings",
                                                        "You have an unsaved changes. Do you wish to continue?",
                                                        AMessageBox::Icon::WARNING,
                                                        AMessageBox::Button::YES_NO);
                        if (result == AMessageBox::ResultButton::YES) {
                            close();
                        }
                    } else {
                        close();
                    }
                }),
            }
        }
    );
    connect(binding->modelChanged, [&, binding] {
        mResetButton->enable();
    });
    connect(fullscreenCheckbox->checked, [this, resolutionView](bool g) { resolutionView->setVisibility(g ? Visibility::VISIBLE : Visibility::GONE); updateLayout(); });
}
