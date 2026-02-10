#include "BoardWidget.h"
#include <cmath>
#include <string>

// ──── Color Palette (warm wood tones) ────
static constexpr double BOARD_R = 0.87, BOARD_G = 0.72, BOARD_B = 0.47;  // warm wood
static constexpr double GRID_R  = 0.25, GRID_G  = 0.20, GRID_B  = 0.15;  // dark brown
static constexpr double BG_R    = 0.15, BG_G    = 0.15, BG_B    = 0.17;  // dark background

BoardWidget::BoardWidget()
{
    set_draw_func(sigc::mem_fun(*this, &BoardWidget::on_draw));
    set_content_width(500);
    set_content_height(500);
    set_hexpand(true);
    set_vexpand(true);

    // Click gesture for interactive mode
    m_clickGesture = Gtk::GestureClick::create();
    m_clickGesture->signal_released().connect(
        sigc::mem_fun(*this, &BoardWidget::on_click));
    add_controller(m_clickGesture);
}

void BoardWidget::set_interactive(bool on)
{
    m_interactive = on;
}

void BoardWidget::on_click(int /*n_press*/, double px, double py)
{
    if (!m_interactive) return;

    const int N = m_state.boardSize;
    if (N <= 0) return;

    // Replicate the same coordinate math as on_draw()
    const double margin = 30.0;
    const double labelSpace = 1.8;
    const int width  = get_width();
    const int height = get_height();
    const double availW = width  - 2 * margin;
    const double availH = height - 2 * margin;
    const double cellSize = std::min(availW, availH) / (N - 1 + 2 * labelSpace);
    const double totalW = cellSize * (N - 1 + 2 * labelSpace);
    const double offsetX = (width  - totalW) / 2.0 + cellSize * labelSpace;
    const double offsetY = (height - totalW) / 2.0 + cellSize * labelSpace;

    // Snap to nearest grid intersection
    int bx = static_cast<int>(std::round((px - offsetX) / cellSize));
    int by = static_cast<int>(std::round((py - offsetY) / cellSize));

    if (bx >= 0 && bx < N && by >= 0 && by < N) {
        m_clickSignal.emit(bx, by);
    }
}

void BoardWidget::update_state(const BoardState& state)
{
    m_state = state;
    queue_draw();
}

void BoardWidget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
    const int N = m_state.boardSize;
    if (N <= 0) return;

    // Background
    cr->set_source_rgb(BG_R, BG_G, BG_B);
    cr->paint();

    // Calculate cell size to fit the board with margins
    const double margin = 30.0;
    const double labelSpace = 1.8;  // cells reserved for coordinate labels on each side
    const double availW = width  - 2 * margin;
    const double availH = height - 2 * margin;
    const double cellSize = std::min(availW, availH) / (N - 1 + 2 * labelSpace);
    
    // Center the board
    const double totalW = cellSize * (N - 1 + 2 * labelSpace);
    const double offsetX = (width  - totalW) / 2.0 + cellSize * labelSpace;
    const double offsetY = (height - totalW) / 2.0 + cellSize * labelSpace;

    // Draw the wooden board surface
    {
        double pad = cellSize * 0.6;
        double bx = offsetX - pad;
        double by = offsetY - pad;
        double bw = cellSize * (N - 1) + pad * 2;
        double bh = bw;
        
        // Rounded rectangle
        double radius = cellSize * 0.3;
        cr->begin_new_sub_path();
        cr->arc(bx + bw - radius, by + radius,          radius, -M_PI/2, 0);
        cr->arc(bx + bw - radius, by + bh - radius,     radius, 0,       M_PI/2);
        cr->arc(bx + radius,      by + bh - radius,     radius, M_PI/2,  M_PI);
        cr->arc(bx + radius,      by + radius,          radius, M_PI,    3*M_PI/2);
        cr->close_path();
        
        // Wood gradient
        auto gradient = Cairo::LinearGradient::create(bx, by, bx + bw, by + bh);
        gradient->add_color_stop_rgb(0.0, BOARD_R * 1.05, BOARD_G * 1.05, BOARD_B * 1.05);
        gradient->add_color_stop_rgb(0.5, BOARD_R,        BOARD_G,        BOARD_B);
        gradient->add_color_stop_rgb(1.0, BOARD_R * 0.90, BOARD_G * 0.90, BOARD_B * 0.90);
        cr->set_source(gradient);
        cr->fill_preserve();
        
        // Subtle border
        cr->set_source_rgba(0.0, 0.0, 0.0, 0.3);
        cr->set_line_width(1.5);
        cr->stroke();
    }

    draw_board_grid(cr, cellSize, offsetX, offsetY);
    draw_star_points(cr, cellSize, offsetX, offsetY);
    draw_coordinates(cr, cellSize, offsetX, offsetY);
    draw_stones(cr, cellSize, offsetX, offsetY);
    draw_last_move_marker(cr, cellSize, offsetX, offsetY);
}

void BoardWidget::draw_board_grid(const Cairo::RefPtr<Cairo::Context>& cr,
                                   double cellSize, double offsetX, double offsetY)
{
    const int N = m_state.boardSize;
    cr->set_source_rgb(GRID_R, GRID_G, GRID_B);
    cr->set_line_width(1.0);

    for (int i = 0; i < N; i++) {
        // Horizontal
        cr->move_to(offsetX, offsetY + i * cellSize);
        cr->line_to(offsetX + (N-1) * cellSize, offsetY + i * cellSize);
        // Vertical
        cr->move_to(offsetX + i * cellSize, offsetY);
        cr->line_to(offsetX + i * cellSize, offsetY + (N-1) * cellSize);
    }
    cr->stroke();

    // Thicker border
    cr->set_line_width(2.0);
    cr->rectangle(offsetX, offsetY, (N-1)*cellSize, (N-1)*cellSize);
    cr->stroke();
}

void BoardWidget::draw_star_points(const Cairo::RefPtr<Cairo::Context>& cr,
                                    double cellSize, double offsetX, double offsetY)
{
    const int N = m_state.boardSize;
    cr->set_source_rgb(GRID_R, GRID_G, GRID_B);

    // Standard star points for 15x15 board
    std::vector<std::pair<int,int>> stars;
    if (N == 15) {
        stars = {{3,3},{3,7},{3,11},{7,3},{7,7},{7,11},{11,3},{11,7},{11,11}};
    } else if (N == 19) {
        stars = {{3,3},{3,9},{3,15},{9,3},{9,9},{9,15},{15,3},{15,9},{15,15}};
    } else if (N >= 9) {
        int mid = N / 2;
        stars = {{mid, mid}};
    }

    double r = cellSize * 0.12;
    for (auto [x, y] : stars) {
        cr->arc(offsetX + x * cellSize, offsetY + y * cellSize, r, 0, 2 * M_PI);
        cr->fill();
    }
}

void BoardWidget::draw_stones(const Cairo::RefPtr<Cairo::Context>& cr,
                               double cellSize, double offsetX, double offsetY)
{
    const int N = m_state.boardSize;
    const double stoneR = cellSize * 0.44;

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            auto cell = m_state.at(x, y);
            if (cell == BoardState::EMPTY) continue;

            double cx = offsetX + x * cellSize;
            double cy = offsetY + y * cellSize;

            if (cell == BoardState::BLACK) {
                // Black stone with radial gradient for 3D effect
                auto grad = Cairo::RadialGradient::create(
                    cx - stoneR * 0.3, cy - stoneR * 0.3, stoneR * 0.1,
                    cx, cy, stoneR
                );
                grad->add_color_stop_rgb(0.0, 0.45, 0.45, 0.50);  // highlight
                grad->add_color_stop_rgb(0.5, 0.15, 0.15, 0.18);
                grad->add_color_stop_rgb(1.0, 0.05, 0.05, 0.08);
                cr->set_source(grad);
            } else {
                // White stone with radial gradient
                auto grad = Cairo::RadialGradient::create(
                    cx - stoneR * 0.3, cy - stoneR * 0.3, stoneR * 0.1,
                    cx, cy, stoneR
                );
                grad->add_color_stop_rgb(0.0, 1.0,  1.0,  1.0);
                grad->add_color_stop_rgb(0.7, 0.92, 0.92, 0.90);
                grad->add_color_stop_rgb(1.0, 0.78, 0.78, 0.75);
                cr->set_source(grad);
            }

            cr->arc(cx, cy, stoneR, 0, 2 * M_PI);
            cr->fill();

            // Subtle shadow under each stone
            cr->set_source_rgba(0.0, 0.0, 0.0, 0.15);
            cr->arc(cx + 1.5, cy + 1.5, stoneR, 0, 2 * M_PI);
            cr->fill();
        }
    }
}

void BoardWidget::draw_last_move_marker(const Cairo::RefPtr<Cairo::Context>& cr,
                                         double cellSize, double offsetX, double offsetY)
{
    if (m_state.lastMoveX < 0 || m_state.lastMoveY < 0) return;

    double cx = offsetX + m_state.lastMoveX * cellSize;
    double cy = offsetY + m_state.lastMoveY * cellSize;
    double r  = cellSize * 0.15;

    auto cell = m_state.at(m_state.lastMoveX, m_state.lastMoveY);
    
    // Draw a contrasting marker (red dot or diamond)
    if (cell == BoardState::BLACK) {
        cr->set_source_rgb(1.0, 0.3, 0.3);  // red on black
    } else {
        cr->set_source_rgb(0.9, 0.2, 0.2);  // darker red on white
    }
    
    // Diamond shape
    cr->move_to(cx, cy - r);
    cr->line_to(cx + r, cy);
    cr->line_to(cx, cy + r);
    cr->line_to(cx - r, cy);
    cr->close_path();
    cr->fill();
}

void BoardWidget::draw_move_numbers(const Cairo::RefPtr<Cairo::Context>& cr,
                                     double cellSize, double offsetX, double offsetY)
{
    // Optional: draw move numbers on stones
    double fontSize = cellSize * 0.35;
    cr->set_font_size(fontSize);

    for (size_t i = 0; i < m_state.moveOrder.size(); i++) {
        auto [x, y] = m_state.moveOrder[i];
        double cx = offsetX + x * cellSize;
        double cy = offsetY + y * cellSize;

        auto cell = m_state.at(x, y);
        if (cell == BoardState::BLACK) {
            cr->set_source_rgb(1.0, 1.0, 1.0);
        } else {
            cr->set_source_rgb(0.0, 0.0, 0.0);
        }

        std::string num = std::to_string(i + 1);
        Cairo::TextExtents ext;
        cr->get_text_extents(num, ext);
        cr->move_to(cx - ext.width / 2, cy + ext.height / 2);
        cr->show_text(num);
    }
}

void BoardWidget::draw_coordinates(const Cairo::RefPtr<Cairo::Context>& cr,
                                    double cellSize, double offsetX, double offsetY)
{
    const int N = m_state.boardSize;
    double fontSize = cellSize * 0.42;
    cr->set_font_size(fontSize);
    cr->set_source_rgba(0.75, 0.75, 0.75, 0.9);

    cr->select_font_face("sans-serif", Cairo::ToyFontFace::Slant::NORMAL,
                          Cairo::ToyFontFace::Weight::BOLD);

    for (int i = 0; i < N; i++) {
        // Column labels (A-O)
        char letter = 'A' + i;
        std::string lbl(1, letter);
        
        Cairo::TextExtents ext;
        cr->get_text_extents(lbl, ext);

        // Top — placed above the board surface
        cr->move_to(offsetX + i * cellSize - ext.width / 2,
                     offsetY - cellSize * 1.0);
        cr->show_text(lbl);

        // Row labels (1-15, bottom to top)
        std::string rowLbl = std::to_string(N - i);
        cr->get_text_extents(rowLbl, ext);

        // Left side — placed left of the board surface
        cr->move_to(offsetX - cellSize * 1.1 - ext.width / 2,
                     offsetY + i * cellSize + ext.height / 2);
        cr->show_text(rowLbl);
    }
}
