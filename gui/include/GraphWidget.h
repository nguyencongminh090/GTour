#pragma once
#include <gtkmm.h>
#include <vector>
#include <map>

class GraphWidget : public Gtk::DrawingArea
{
public:
    GraphWidget();
    virtual ~GraphWidget();

    struct DataPoint {
        int moveIdx;
        double winrate; // 0.0 to 1.0
    };

    void push_data(int engineIdx, int moveIdx, double winrate);
    void clear();
    void set_interactive(bool interactive);

protected:
    void on_draw(const Glib::RefPtr<Cairo::Context>& cr, int width, int height);

    // Data storage: map from engineIdx to vector of points
    std::map<int, std::vector<DataPoint>> m_Data;
    
    // Interactive
    Glib::RefPtr<Gtk::EventControllerMotion> m_MotionController;
    void on_motion(double x, double y);
    void on_leave();

    double m_HoverX = -1;
    double m_HoverY = -1;
    bool   m_Hovering = false;
    
    struct TooltipInfo {
        int moveIdx;
        double winrate;
        int engineIdx;
        double x, y;
    } m_Tooltip;
    bool m_ShowTooltip = false;
};
