#include "GraphWidget.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

GraphWidget::GraphWidget()
{
    set_draw_func(sigc::mem_fun(*this, &GraphWidget::on_draw));

    // Interactive support
    m_MotionController = Gtk::EventControllerMotion::create();
    m_MotionController->signal_motion().connect(sigc::mem_fun(*this, &GraphWidget::on_motion));
    m_MotionController->signal_leave().connect(sigc::mem_fun(*this, &GraphWidget::on_leave));
    add_controller(m_MotionController);
    
    set_size_request(-1, 150); // Minimum height
}

GraphWidget::~GraphWidget()
{
}

void GraphWidget::push_data(int engineIdx, int moveIdx, double winrate)
{
    // Check if we need to clear widely out of order data (e.g. new game)
    // Simple heuristic: if moveIdx is 0 or 1, likely new game.
    // Ideally clear() is called externally, but we can be safe.
    // For now assuming caller manages clear().
    
    DataPoint p = {moveIdx, winrate};
    
    // Filter duplicates or updates for same move?
    // Engines might report multipv or depth updates. We usually want the latest for the move.
    
    auto& vec = m_Data[engineIdx];
    bool found = false;
    for (auto& dp : vec) {
        if (dp.moveIdx == moveIdx) {
            dp.winrate = winrate;
            found = true;
            break;
        }
    }
    if (!found) {
        vec.push_back(p);
        // Keep sorted
        std::sort(vec.begin(), vec.end(), [](const DataPoint& a, const DataPoint& b){
            return a.moveIdx < b.moveIdx;
        });
    }
    
    queue_draw();
}

void GraphWidget::clear()
{
    m_Data.clear();
    m_ShowTooltip = false;
    queue_draw();
}

void GraphWidget::on_motion(double x, double y)
{
    m_HoverX = x;
    m_HoverY = y;
    m_Hovering = true;
    
    // Find closest point
    double minDist = 20.0; // hit radius
    bool found = false;
    
    int width = get_width();
    int height = get_height();
    
    // Determine bounds
    int maxMove = 0;
    for (const auto& kv : m_Data) {
        if (!kv.second.empty()) maxMove = std::max(maxMove, kv.second.back().moveIdx);
    }
    if (maxMove < 10) maxMove = 10;
    
    // Margins
    const double padLeft = 30, padRight = 10, padTop = 10, padBottom = 20;
    double graphW = width - padLeft - padRight;
    double graphH = height - padTop - padBottom;
    
    for (const auto& kv : m_Data) {
        int engIdx = kv.first;
        const auto& vec = kv.second;
        
        for (const auto& p : vec) {
            double px = padLeft + (double)p.moveIdx / maxMove * graphW;
            double py = padTop + (1.0 - p.winrate) * graphH;
            
            double dist = std::abs(px - x); // Primarily check X distance
            if (dist < minDist) {
                // Secondary Y check looser
                if (std::abs(py - y) < 50) {
                    m_Tooltip = {p.moveIdx, p.winrate, engIdx, px, py};
                    minDist = dist;
                    found = true;
                }
            }
        }
    }
    
    if (found != m_ShowTooltip) {
        m_ShowTooltip = found;
        queue_draw();
    } else if (found) {
        // Redraw if tooltip content implies change (though here we just redraw always when moving if found)
        queue_draw();
    }
}

void GraphWidget::on_leave()
{
    m_Hovering = false;
    if (m_ShowTooltip) {
        m_ShowTooltip = false;
        queue_draw();
    }
}

void GraphWidget::on_draw(const Glib::RefPtr<Cairo::Context>& cr, int width, int height)
{
    // Background style
    cr->set_source_rgba(0.15, 0.15, 0.17, 1.0); // Dark sleek background
    cr->paint();
    
    // Margins
    const double padLeft = 30, padRight = 10, padTop = 10, padBottom = 20;
    double graphW = width - padLeft - padRight;
    double graphH = height - padTop - padBottom;
    
    // Grid
    cr->set_source_rgba(0.3, 0.3, 0.35, 1.0);
    cr->set_line_width(1.0);
    
    // Horizontal lines (0, 0.5, 1.0)
    for (int i = 0; i <= 2; i++) {
        double y = padTop + (i * 0.5) * graphH; // 0=1.0(top), 1=0.5(mid), 2=0.0(bot) ?? No:
        // winrate 1.0 is TOP (padTop + 0*H)
        // winrate 0.0 is BOT (padTop + 1*H)
        // Loop i=0(top, 1.0), i=1(0.5), i=2(0.0)
        
        cr->move_to(padLeft, y);
        cr->line_to(width - padRight, y);
        cr->stroke();
    }
    
    // Range
    int maxMove = 0;
    for (const auto& kv : m_Data) {
        if (!kv.second.empty()) maxMove = std::max(maxMove, kv.second.back().moveIdx);
    }
    if (maxMove < 20) maxMove = 20; // Min scale
    
    // Draw Data
    cr->set_line_width(2.0);
    
    // Colors for Engine 0 and 1
    // Engine 0: Cyan/Blue
    // Engine 1: Orange/Red
    const double colors[2][3] = {
        {0.0, 0.8, 1.0}, // Cyan
        {1.0, 0.5, 0.0}  // Orange
    };
    
    for (const auto& kv : m_Data) {
        int engIdx = kv.first;
        if (kv.second.empty()) continue;
        
        // Pick color (mod 2)
        int colorIdx = engIdx % 2;
        cr->set_source_rgba(colors[colorIdx][0], colors[colorIdx][1], colors[colorIdx][2], 1.0);
        
        const auto& vec = kv.second;
        bool first = true;
        
        for (const auto& p : vec) {
            double x = padLeft + (double)p.moveIdx / maxMove * graphW;
            double y = padTop + (1.0 - p.winrate) * graphH;
            
            if (first) {
                cr->move_to(x, y);
                first = false;
            } else {
                cr->line_to(x, y);
            }
        }
        cr->stroke();
        
        // Draw points slightly partially
        /*
        for (const auto& p : vec) {
            double x = padLeft + (double)p.moveIdx / maxMove * graphW;
            double y = padTop + (1.0 - p.winrate) * graphH;
            cr->arc(x, y, 2.0, 0, 2*M_PI);
            cr->fill();
        }
        */
    }
    
    // Axes Labels (simple)
    cr->set_source_rgba(0.7, 0.7, 0.7, 1.0);
    cr->select_font_face("Monospace", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(10);
    
    // Y-axis labels
    cr->move_to(2, padTop + 0 * graphH + 10); cr->show_text("1.0");
    cr->move_to(2, padTop + 0.5 * graphH + 4); cr->show_text("0.5");
    cr->move_to(2, padTop + 1.0 * graphH); cr->show_text("0.0");
    
    // Tooltip
    if (m_ShowTooltip) {
        // Draw crosshair line
        cr->set_source_rgba(1.0, 1.0, 1.0, 0.3);
        cr->set_line_width(1.0);
        cr->move_to(m_Tooltip.x, padTop);
        cr->line_to(m_Tooltip.x, height - padBottom);
        cr->stroke();
        
        // Draw Box
        std::stringstream ss;
        ss << "Move: " << m_Tooltip.moveIdx << " Win: " << std::fixed << std::setprecision(2) << m_Tooltip.winrate;
        std::string text = ss.str();
        
        Cairo::TextExtents te;
        cr->get_text_extents(text, te);
        
        double boxW = te.width + 10;
        double boxH = te.height + 10;
        double boxX = m_Tooltip.x + 10;
        double boxY = m_Tooltip.y - 10 - boxH;
        
        if (boxX + boxW > width) boxX = m_Tooltip.x - 10 - boxW;
        if (boxY < 0) boxY = m_Tooltip.y + 10;
        
        cr->set_source_rgba(0.1, 0.1, 0.1, 0.9);
        cr->rectangle(boxX, boxY, boxW, boxH);
        cr->fill();
        
        cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
        cr->move_to(boxX + 5, boxY + boxH - 5);
        cr->show_text(text);
        
        // Highlight point
        cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
        cr->arc(m_Tooltip.x, m_Tooltip.y, 4.0, 0, 2*M_PI);
        cr->fill();
    }
}
