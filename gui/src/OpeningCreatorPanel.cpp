#include "OpeningCreatorPanel.h"
#include <fstream>

OpeningCreatorPanel::OpeningCreatorPanel(BoardWidget& board)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 8)
    , m_board(board)
{
    set_margin(12);

    // ─── Current Position Section ───
    m_LblCurrentPos.set_css_classes({"title-4"});
    m_LblCurrentPos.set_halign(Gtk::Align::START);
    append(m_LblCurrentPos);

    m_EntryCurrentPos.set_editable(false);
    m_EntryCurrentPos.set_placeholder_text("(click board to place stones)");
    m_EntryCurrentPos.set_hexpand(true);
    append(m_EntryCurrentPos);

    m_BtnUndo.set_tooltip_text("Remove last stone");
    m_BtnClearBoard.set_tooltip_text("Remove all stones");
    m_BtnAddToList.set_tooltip_text("Add current position to openings list");
    m_BtnAddToList.set_css_classes({"suggested-action"});

    m_BoxCurrentBtns.set_halign(Gtk::Align::START);
    m_BoxCurrentBtns.append(m_BtnUndo);
    m_BoxCurrentBtns.append(m_BtnClearBoard);
    m_BoxCurrentBtns.append(m_BtnAddToList);
    append(m_BoxCurrentBtns);

    // ─── Separator ───
    append(m_Sep);

    // ─── Openings List Section ───
    m_LblOpenings.set_css_classes({"title-4"});
    m_LblOpenings.set_halign(Gtk::Align::START);
    append(m_LblOpenings);

    m_ScrollList.set_child(m_ListBox);
    m_ScrollList.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_ScrollList.set_vexpand(true);
    m_ScrollList.set_min_content_height(120);
    m_ListBox.set_selection_mode(Gtk::SelectionMode::SINGLE);
    append(m_ScrollList);

    m_BoxListBtns.set_halign(Gtk::Align::START);
    m_BoxListBtns.append(m_BtnRemoveSelected);
    m_BoxListBtns.append(m_BtnClearList);
    m_BoxListBtns.append(m_BtnSaveFile);
    m_BtnSaveFile.set_css_classes({"suggested-action"});
    append(m_BoxListBtns);

    // ─── Connect signals ───
    m_BtnUndo.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_undo));
    m_BtnClearBoard.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_clear_board));
    m_BtnAddToList.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_add_to_list));
    m_BtnRemoveSelected.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_remove_selected));
    m_BtnClearList.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_clear_list));
    m_BtnSaveFile.signal_clicked().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_save_file));

    m_board.signal_board_click().connect(sigc::mem_fun(*this, &OpeningCreatorPanel::on_board_click));
}

void OpeningCreatorPanel::activate()
{
    m_boardSize = m_board.get_board_size();
    m_editState.resize(m_boardSize);
    m_moves.clear();
    m_board.set_interactive(true);
    refresh_board();
    refresh_current_text();
    set_visible(true);
}

void OpeningCreatorPanel::deactivate()
{
    m_board.set_interactive(false);
    set_visible(false);
}

// ─── Board click handler ───
void OpeningCreatorPanel::on_board_click(int x, int y)
{
    // Ignore if cell already occupied
    if (m_editState.at(x, y) != BoardState::EMPTY) return;

    // Alternate Black/White
    auto color = (m_moves.size() % 2 == 0) ? BoardState::BLACK : BoardState::WHITE;
    m_editState.set(x, y, color);
    m_moves.push_back({x, y});
    m_editState.lastMoveX = x;
    m_editState.lastMoveY = y;
    m_editState.moveOrder.push_back({x, y});

    refresh_board();
    refresh_current_text();
}

void OpeningCreatorPanel::on_undo()
{
    if (m_moves.empty()) return;

    auto [x, y] = m_moves.back();
    m_editState.set(x, y, BoardState::EMPTY);
    m_moves.pop_back();
    m_editState.moveOrder.pop_back();

    if (!m_moves.empty()) {
        auto [lx, ly] = m_moves.back();
        m_editState.lastMoveX = lx;
        m_editState.lastMoveY = ly;
    } else {
        m_editState.lastMoveX = -1;
        m_editState.lastMoveY = -1;
    }

    refresh_board();
    refresh_current_text();
}

void OpeningCreatorPanel::on_clear_board()
{
    m_editState.resize(m_boardSize);
    m_moves.clear();
    refresh_board();
    refresh_current_text();
}

void OpeningCreatorPanel::on_add_to_list()
{
    if (m_moves.empty()) return;

    std::string posStr = moves_to_pos_string();
    m_openings.push_back(posStr);
    refresh_list_box();

    // Clear board for next opening
    on_clear_board();
}

void OpeningCreatorPanel::on_remove_selected()
{
    auto* row = m_ListBox.get_selected_row();
    if (!row) return;

    int idx = row->get_index();
    if (idx >= 0 && idx < static_cast<int>(m_openings.size())) {
        m_openings.erase(m_openings.begin() + idx);
        refresh_list_box();
    }
}

void OpeningCreatorPanel::on_clear_list()
{
    m_openings.clear();
    refresh_list_box();
}

void OpeningCreatorPanel::on_save_file()
{
    if (m_openings.empty()) return;

    auto* toplevel = dynamic_cast<Gtk::Window*>(get_root());

    auto* dialog = new Gtk::FileChooserDialog("Save Openings File",
                                               Gtk::FileChooser::Action::SAVE);
    if (toplevel)
        dialog->set_transient_for(*toplevel);

    dialog->set_modal(true);
    dialog->add_button("Cancel", Gtk::ResponseType::CANCEL);
    dialog->add_button("Save", Gtk::ResponseType::ACCEPT);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Text files");
    filter->add_pattern("*.txt");
    dialog->add_filter(filter);

    dialog->set_current_name("openings.txt");

    // Capture openings by value for the async callback
    auto openings_copy = m_openings;

    dialog->signal_response().connect([dialog, openings_copy](int response_id) {
        if (response_id == Gtk::ResponseType::ACCEPT) {
            auto path = dialog->get_file()->get_path();
            std::ofstream out(path);
            if (out.is_open()) {
                for (const auto& line : openings_copy) {
                    out << line << "\n";
                }
                out.close();
            }
        }
        delete dialog;
    });

    dialog->show();
}

// ─── Helpers ───

std::string OpeningCreatorPanel::moves_to_pos_string() const
{
    // OPENING_POS format: each move is [a-z][1-N]
    // x maps to 'a'+x, y maps to boardSize-y (since y=0 is top but row labels start from bottom)
    std::string s;
    for (auto [x, y] : m_moves) {
        s += static_cast<char>('a' + x);
        s += std::to_string(m_boardSize - y);
    }
    return s;
}

void OpeningCreatorPanel::refresh_current_text()
{
    if (m_moves.empty()) {
        m_EntryCurrentPos.set_text("");
    } else {
        m_EntryCurrentPos.set_text(moves_to_pos_string());
    }
}

void OpeningCreatorPanel::refresh_board()
{
    m_board.update_state(m_editState);
}

void OpeningCreatorPanel::refresh_list_box()
{
    // Clear all rows
    while (auto* child = m_ListBox.get_first_child()) {
        m_ListBox.remove(*child);
    }

    // Add rows
    for (size_t i = 0; i < m_openings.size(); i++) {
        auto* label = Gtk::make_managed<Gtk::Label>(
            std::to_string(i + 1) + ": " + m_openings[i]);
        label->set_halign(Gtk::Align::START);
        label->set_selectable(true);
        m_ListBox.append(*label);
    }
}
