//
// Created by alex2 on 15.04.2021.
//

#include "LauncherSettingsWindow.h"
#include "MainWindow.h"
#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/View/ATabView.h>
#include <AUI/View/APathChooserView.h>
#include <AUI/Json/AJson.h>
#include <Model/Settings.h>
#include <AUI/View/ARadioButton.h>
#include <AUI/View/ANumberPicker.h>
#include <AUI/View/ACheckBox.h>
#include <AUI/Platform/AMessageBox.h>


LauncherSettingsWindow::LauncherSettingsWindow() :
        AWindow("Settings", 400_dp, 300_dp, Autumn::get<MainWindow>().get(), WS_DIALOG) {
    auto binding = _new<ADataBinding<Settings>>(Settings::inst());
    setContents(
        Vertical {
            _new<ATabView>() let {
                it->addTab(
                    Vertical{
                        _form({
                            {"Game folder:"_as, _new<APathChooserView>() && binding(&Settings::game_folder)},
                            {
                                "Display:"_as,
                                Horizontal {
                                    _new<ANumberPicker>() && binding(&Settings::width),
                                    _new<ALabel>("x"),
                                    _new<ANumberPicker>()&& binding(&Settings::height),
                                    _new<ACheckBox>("Fullscreen") && binding(&Settings::is_fullscreen),
                                }
                            },
                        }),
                    }, "Game"
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
}