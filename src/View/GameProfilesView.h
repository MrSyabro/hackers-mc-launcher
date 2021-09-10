#pragma once


#include <AUI/View/AViewContainer.h>
#include <Model/GameProfile.h>

class GameProfilesView: public AViewContainer {
private:
    _<IListModel<GameProfile>> mModel;
    size_t mProfileIndex = 0;

    void onSelectedProfile(size_t profileIndex);

public:

    GameProfilesView(const _<IListModel<GameProfile>>& model);

    [[nodiscard]]
    size_t getSelectedProfileIndex() const {
        return mProfileIndex;
    }
signals:
    emits<> selectionChanged;
};


