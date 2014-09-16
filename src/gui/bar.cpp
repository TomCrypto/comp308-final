#include "gui/bar.h"

namespace gui
{
    basic_bar::basic_bar(const std::string& name)
    {
        if (!(m_bar = TwNewBar((m_name = name).c_str())))
            throw std::runtime_error("failed to create tweakbar");
    }

    basic_bar::~basic_bar()
    {
        TwDeleteBar(m_bar);
    }

    void basic_bar::set_title(const std::string& title)
    {
        TwDefine((m_name + " label='" + title + "'").c_str());
    }

    void basic_bar::refresh()
    {
        TwRefreshBar(m_bar);
    }

    main_bar::main_bar(const std::string& name) : basic_bar(name)
    {
        this->rotation = 0;

        TwAddVarRW(m_bar,
            "Rotation", TW_TYPE_FLOAT, &this->rotation,
            "min=0 max=360 step=1.0 help='Rotate the skeleton'");

        TwDefine((m_name + " contained=true").c_str());
    }
}