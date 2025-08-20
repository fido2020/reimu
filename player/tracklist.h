#pragma once

#include "reimu/gui/widget.h"
#include <reimu/gui/widget.h>

class TrackListProvider {

};

class TrackList : public reimu::gui::FlowBox {
    TrackList(TrackListProvider &provider);

private:
    TrackListProvider &m_provider;
};
