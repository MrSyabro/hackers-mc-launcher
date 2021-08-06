//
// Created by alex2772 on 2/17/21.
//

#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/View/ACheckBox.h>
#include <AUI/View/AButton.h>
#include <Repository/UsersRepository.h>
#include <AUI/Platform/AMessageBox.h>
#include <AUI/Traits/strings.h>
#include <AUI/Util/ARandom.h>
#include "UserWindow.h"

UserWindow::UserWindow(User* user):
    AWindow(user == nullptr ? "New user" : "Edit user", 200, 100, dynamic_cast<AWindow*>(AWindow::current()), WindowStyle::DIALOG)
{

    if (user) {
        mBinding->setModel(*user);
    } else {
        mBinding->setModel(User{});
    }

    setContents(
        Vertical {
            _form({
                {"Username"_as, mUsername = _new<ATextField>() let { it->focus(); it && mBinding(&User::username); }},
                {{},            _new<ACheckBox>("Online account on minecraft.net") && mBinding(&User::isOnlineAccount)},
                {"Password"_as, _new<ATextField>() && mBinding(&User::token) && mBinding(&User::isOnlineAccount, &ATextField::setEnabled) },
                }),
            Horizontal {
                user ? (_new<AButton>("Delete user").connect(&AView::clicked, this, [&, user] {
                    if (AMessageBox::show(this,
                                      "Delete user",
                                      "Do you really want to delete user \"{}\"? "
                                      "The operation is unrecoverable!"_as.format(user->username),
                                      AMessageBox::Icon::INFO,
                                      AMessageBox::Button::YES_NO) == AMessageBox::ResultButton::YES) {
                        close();
                        emit deleteUser();
                    }
                })) : nullptr,
                _new<ASpacer>(),
                (user ?
                    _new<AButton>("OK").connect(&AView::clicked, this, [&, user] {
                        if (mBinding->getModel().username.empty()) {
                            AMessageBox::show(this,
                                              "Username is empty",
                                              "Please enter some nonempty username.",
                                              AMessageBox::Icon::INFO,
                                              AMessageBox::Button::OK);
                        }
                        *user = mBinding->getModel();
                        emit finished();
                        close();
                    })
                        :
                    _new<AButton>("Create").connect(&AView::clicked, this, [&] {
                        auto user = mBinding->getModel();
                        user.uuid = Autumn::get<ARandom>()->nextUuid();
                        UsersRepository::inst().addUser(user);
                        close();
                    })) let { it->setDefault(); },
                _new<AButton>("Cancel").connect(&AView::clicked, me::close)
            }
        }
    );
    pack();
}
