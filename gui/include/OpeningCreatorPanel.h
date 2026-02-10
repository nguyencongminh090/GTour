#pragma once

#include "BoardWidget.h"
#include <gtkmm.h>
#include <string>
#include <vector>

// Panel for interactively creating opening positions
class OpeningCreatorPanel : public Gtk::Box
{
public:
    OpeningCreatorPanel(BoardWidget& board);

    // Called by MainWindow when entering/leaving this mode
    void activate();
    void deactivate();

private:
    BoardWidget& m_board;

    // Current position editing
    int m_boardSize = 15;
    BoardState m_editState;                  // current board being edited
    std::vector<std::pair<int,int>> m_moves; // move sequence for current position

    // Widgets — current position
    Gtk::Label m_LblCurrentPos{"Current Position"};
    Gtk::Entry m_EntryCurrentPos;
    Gtk::Box   m_BoxCurrentBtns{Gtk::Orientation::HORIZONTAL, 5};
    Gtk::Button m_BtnUndo{"Undo"};
    Gtk::Button m_BtnClearBoard{"Clear Board"};
    Gtk::Button m_BtnAddToList{"Add to List"};

    // Widgets — openings list
    Gtk::Separator m_Sep;
    Gtk::Label m_LblOpenings{"Openings List"};
    Gtk::ScrolledWindow m_ScrollList;
    Gtk::ListBox m_ListBox;
    Gtk::Box   m_BoxListBtns{Gtk::Orientation::HORIZONTAL, 5};
    Gtk::Button m_BtnRemoveSelected{"Remove Selected"};
    Gtk::Button m_BtnClearList{"Clear List"};
    Gtk::Button m_BtnSaveFile{"Save to File..."};

    // Data
    std::vector<std::string> m_openings;

    // Handlers
    void on_board_click(int x, int y);
    void on_undo();
    void on_clear_board();
    void on_add_to_list();
    void on_remove_selected();
    void on_clear_list();
    void on_save_file();

    void refresh_current_text();
    void refresh_board();
    void refresh_list_box();
    std::string moves_to_pos_string() const;
};
