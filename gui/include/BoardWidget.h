#pragma once

#include "BoardState.h"
#include <gtkmm.h>

// Custom Gtk::DrawingArea that renders a Gomoku board
class BoardWidget : public Gtk::DrawingArea
{
public:
    BoardWidget();

    void update_state(const BoardState& state);

    // Interactive mode — enables clicking to place stones
    void set_interactive(bool on);
    bool get_interactive() const { return m_interactive; }

    // Signal emitted when user clicks a valid board intersection in interactive mode
    using ClickSignal = sigc::signal<void(int, int)>;
    ClickSignal signal_board_click() { return m_clickSignal; }

    // Coordinate helpers (need to match on_draw logic)
    int get_board_size() const { return m_state.boardSize; }

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

private:
    BoardState m_state;
    bool m_interactive = false;
    ClickSignal m_clickSignal;
    Glib::RefPtr<Gtk::GestureClick> m_clickGesture;

    void on_click(int n_press, double x, double y);

    // Drawing helpers
    void draw_board_grid(const Cairo::RefPtr<Cairo::Context>& cr, 
                         double cellSize, double offsetX, double offsetY);
    void draw_star_points(const Cairo::RefPtr<Cairo::Context>& cr, 
                          double cellSize, double offsetX, double offsetY);
    void draw_stones(const Cairo::RefPtr<Cairo::Context>& cr, 
                     double cellSize, double offsetX, double offsetY);
    void draw_last_move_marker(const Cairo::RefPtr<Cairo::Context>& cr, 
                               double cellSize, double offsetX, double offsetY);
    void draw_move_numbers(const Cairo::RefPtr<Cairo::Context>& cr,
                           double cellSize, double offsetX, double offsetY);
    void draw_coordinates(const Cairo::RefPtr<Cairo::Context>& cr,
                          double cellSize, double offsetX, double offsetY);
};
