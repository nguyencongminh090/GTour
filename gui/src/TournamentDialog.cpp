#include "TournamentDialog.h"
#include <iostream>
#include <fstream>

TournamentDialog::TournamentDialog(Gtk::Window& parent)
: Gtk::Dialog("New Tournament", parent, true)
{
    set_default_size(650, 550);

    auto content_area = get_content_area();
    content_area->set_margin(8);
    content_area->append(m_ContentBox);
    m_ContentBox.set_spacing(8);

    // ---- Notebook (tabs) ----
    m_ContentBox.append(m_Notebook);
    m_Notebook.set_vexpand(true);

    m_Notebook.append_page(*build_engines_tab(),      "Engines");
    m_Notebook.append_page(*build_game_tab(),         "Game");
    m_Notebook.append_page(*build_openings_tab(),     "Openings");
    m_Notebook.append_page(*build_adjudication_tab(), "Adjudication");
    m_Notebook.append_page(*build_output_tab(),       "Output");
    m_Notebook.append_page(*build_advanced_tab(),     "Advanced");

    // ---- Action Buttons ----
    m_BoxButtons.set_halign(Gtk::Align::END);
    m_BoxButtons.set_spacing(10);
    m_BoxButtons.set_margin_top(8);
    m_BoxButtons.append(m_BtnLoadSettings);
    m_BoxButtons.append(m_BtnSaveSettings);
    m_BoxButtons.append(m_BtnCancel);
    m_BoxButtons.append(m_BtnStart);
    m_ContentBox.append(m_BoxButtons);

    m_BtnLoadSettings.signal_clicked().connect([this]() { on_load_settings(); });
    m_BtnSaveSettings.signal_clicked().connect([this]() { on_save_settings(); });
    m_BtnCancel.signal_clicked().connect([this]() { response(Gtk::ResponseType::CANCEL); });
    m_BtnStart.signal_clicked().connect([this]() { response(Gtk::ResponseType::OK); });
}

TournamentDialog::~TournamentDialog() {}

// ============================================================
//  Engines Tab
// ============================================================

Gtk::Box* TournamentDialog::build_engines_tab()
{
    m_BoxEngines.set_margin(10);
    m_BoxEngines.set_spacing(8);

    // Engine rows container
    m_BoxEngineList.set_spacing(3);
    m_BoxEngines.append(m_BoxEngineList);

    // Add 2 default engine rows
    on_add_engine();
    on_add_engine();

    // Add Engine button
    m_BoxEngineButtons.set_halign(Gtk::Align::START);
    m_BoxEngineButtons.set_spacing(5);
    m_BoxEngineButtons.append(m_BtnAddEngine);
    m_BtnAddEngine.signal_clicked().connect(sigc::mem_fun(*this, &TournamentDialog::on_add_engine));
    m_BoxEngines.append(m_BoxEngineButtons);

    // Shared time control frame
    m_FrameEachTC.set_child(m_GridEachTC);
    m_GridEachTC.set_margin(8);
    m_GridEachTC.set_row_spacing(5);
    m_GridEachTC.set_column_spacing(10);

    m_SpinTurnTimeout.set_adjustment(Gtk::Adjustment::create(5000, 0, 600000, 100, 1000, 0));
    m_SpinMatchTimeout.set_adjustment(Gtk::Adjustment::create(0, 0, 3600000, 1000, 10000, 0));
    m_SpinIncrement.set_adjustment(Gtk::Adjustment::create(0, 0, 60000, 100, 1000, 0));
    m_SpinMaxMemory.set_adjustment(Gtk::Adjustment::create(350, 0, 65536, 50, 100, 0));
    m_SpinThreads.set_adjustment(Gtk::Adjustment::create(1, 0, 128, 1, 4, 0));
    m_SpinTolerance.set_adjustment(Gtk::Adjustment::create(3, 0, 60, 1, 5, 0));

    int row = 0;
    m_GridEachTC.attach(m_LblTurnTimeout,   0, row);
    m_GridEachTC.attach(m_SpinTurnTimeout,   1, row++);
    m_GridEachTC.attach(m_LblMatchTimeout,  0, row);
    m_GridEachTC.attach(m_SpinMatchTimeout,  1, row++);
    m_GridEachTC.attach(m_LblIncrement,     0, row);
    m_GridEachTC.attach(m_SpinIncrement,     1, row++);
    m_GridEachTC.attach(m_LblMaxMemory,     0, row);
    m_GridEachTC.attach(m_SpinMaxMemory,     1, row++);
    m_GridEachTC.attach(m_LblThreads,       0, row);
    m_GridEachTC.attach(m_SpinThreads,       1, row++);
    m_GridEachTC.attach(m_LblTolerance,     0, row);
    m_GridEachTC.attach(m_SpinTolerance,     1, row++);

    m_BoxEngines.append(m_FrameEachTC);

    return &m_BoxEngines;
}

void TournamentDialog::on_add_engine()
{
    auto row = std::make_unique<EngineRow>();
    int idx = static_cast<int>(m_EngineRows.size()) + 1;

    row->box.set_spacing(5);

    row->entryCmd.set_hexpand(true);
    row->entryCmd.set_placeholder_text("Path to Engine " + std::to_string(idx));
    row->entryName.set_placeholder_text("Name");
    row->entryName.set_max_width_chars(12);

    auto* entryPtr = &row->entryCmd;
    row->btnBrowse.signal_clicked().connect([this, entryPtr]() { on_browse_engine(entryPtr); });

    auto* rowPtr = row.get();
    row->btnRemove.signal_clicked().connect([this, rowPtr]() { on_remove_engine(rowPtr); });

    row->box.append(row->entryCmd);
    row->box.append(row->entryName);
    row->box.append(row->btnBrowse);
    row->box.append(row->btnRemove);

    // Simply append to the engine list container — no need to juggle siblings
    m_BoxEngineList.append(row->box);

    m_EngineRows.push_back(std::move(row));
    update_remove_buttons();
}

void TournamentDialog::on_remove_engine(EngineRow* target)
{
    for (auto it = m_EngineRows.begin(); it != m_EngineRows.end(); ++it) {
        if (it->get() == target) {
            m_BoxEngineList.remove(target->box);
            m_EngineRows.erase(it);
            break;
        }
    }
    update_remove_buttons();
}

void TournamentDialog::update_remove_buttons()
{
    bool canRemove = m_EngineRows.size() > 2;
    for (auto& row : m_EngineRows) {
        row->btnRemove.set_sensitive(canRemove);
    }
}

void TournamentDialog::on_browse_engine(Gtk::Entry* entry)
{
    on_browse_file(entry, "Select Engine Executable");
}

void TournamentDialog::on_browse_file(Gtk::Entry* entry, const std::string& title)
{
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title(title);
    dialog->open(*this, [this, entry, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            auto file = dialog->open_finish(result);
            if (file) entry->set_text(file->get_path());
        } catch (const Glib::Error& err) {
            std::cerr << "File dialog error: " << err.what() << std::endl;
        }
    });
}

// ============================================================
//  Game Tab
// ============================================================

Gtk::Box* TournamentDialog::build_game_tab()
{
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(10);
    box->append(m_GridGame);

    m_GridGame.set_row_spacing(8);
    m_GridGame.set_column_spacing(12);

    m_SpinBoardSize.set_adjustment(Gtk::Adjustment::create(15, 5, 22, 1, 5, 0));
    m_SpinGames.set_adjustment(Gtk::Adjustment::create(10, 1, 100000, 1, 10, 0));
    m_SpinRounds.set_adjustment(Gtk::Adjustment::create(1, 1, 1000, 1, 10, 0));
    m_SpinConcurrency.set_adjustment(Gtk::Adjustment::create(1, 1, 64, 1, 4, 0));

    // Game rules
    m_ComboGameRule.append("0", "Freestyle (≥5)");
    m_ComboGameRule.append("1", "Standard (=5)");
    m_ComboGameRule.append("4", "Renju");
    m_ComboGameRule.set_active_id("0");

    int row = 0;
    m_GridGame.attach(m_LblBoardSize,   0, row);
    m_GridGame.attach(m_SpinBoardSize,  1, row++);
    m_GridGame.attach(m_LblGameRule,    0, row);
    m_GridGame.attach(m_ComboGameRule,  1, row++);
    m_GridGame.attach(m_LblGames,       0, row);
    m_GridGame.attach(m_SpinGames,      1, row++);
    m_GridGame.attach(m_LblRounds,      0, row);
    m_GridGame.attach(m_SpinRounds,     1, row++);
    m_GridGame.attach(m_LblConcurrency, 0, row);
    m_GridGame.attach(m_SpinConcurrency,1, row++);

    return box;
}

// ============================================================
//  Openings Tab
// ============================================================

Gtk::Box* TournamentDialog::build_openings_tab()
{
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(10);
    box->set_spacing(8);
    box->append(m_GridOpenings);

    m_GridOpenings.set_row_spacing(8);
    m_GridOpenings.set_column_spacing(10);

    m_EntryOpening.set_hexpand(true);
    m_ComboOpeningType.append("offset", "Offset");
    m_ComboOpeningType.append("pos", "Position");
    m_ComboOpeningType.set_active_id("offset");

    int row = 0;
    m_GridOpenings.attach(m_LblOpeningFile,  0, row);
    m_GridOpenings.attach(m_EntryOpening,    1, row);
    m_GridOpenings.attach(m_BtnBrowseOpening,2, row++);
    m_GridOpenings.attach(m_LblOpeningType,  0, row);
    m_GridOpenings.attach(m_ComboOpeningType, 1, row++);

    m_BtnBrowseOpening.signal_clicked().connect(
        [this]() { on_browse_file(&m_EntryOpening, "Select Opening File"); });

    box->append(m_ChkRandomOrder);
    box->append(m_ChkRepeat);
    box->append(m_ChkTransform);

    return box;
}

// ============================================================
//  Adjudication Tab
// ============================================================

Gtk::Box* TournamentDialog::build_adjudication_tab()
{
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(10);
    box->append(m_GridAdjudication);

    m_GridAdjudication.set_row_spacing(8);
    m_GridAdjudication.set_column_spacing(12);

    m_SpinResignCount.set_adjustment(Gtk::Adjustment::create(0, 0, 100, 1, 5, 0));
    m_SpinResignScore.set_adjustment(Gtk::Adjustment::create(0, 0, 100000, 100, 1000, 0));
    m_SpinDrawCount.set_adjustment(Gtk::Adjustment::create(0, 0, 100, 1, 5, 0));
    m_SpinDrawScore.set_adjustment(Gtk::Adjustment::create(0, 0, 100000, 100, 1000, 0));
    m_SpinDrawAfter.set_adjustment(Gtk::Adjustment::create(0, 0, 1000, 10, 50, 0));

    int row = 0;
    m_GridAdjudication.attach(m_LblResignCount, 0, row);
    m_GridAdjudication.attach(m_SpinResignCount, 1, row++);
    m_GridAdjudication.attach(m_LblResignScore, 0, row);
    m_GridAdjudication.attach(m_SpinResignScore, 1, row++);

    auto* sep = Gtk::make_managed<Gtk::Separator>();
    m_GridAdjudication.attach(*sep, 0, row++, 2);

    m_GridAdjudication.attach(m_LblDrawCount, 0, row);
    m_GridAdjudication.attach(m_SpinDrawCount, 1, row++);
    m_GridAdjudication.attach(m_LblDrawScore, 0, row);
    m_GridAdjudication.attach(m_SpinDrawScore, 1, row++);
    m_GridAdjudication.attach(m_LblDrawAfter, 0, row);
    m_GridAdjudication.attach(m_SpinDrawAfter, 1, row++);

    return box;
}

// ============================================================
//  Output Tab
// ============================================================

Gtk::Box* TournamentDialog::build_output_tab()
{
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(10);
    box->set_spacing(8);
    box->append(m_GridOutput);

    m_GridOutput.set_row_spacing(8);
    m_GridOutput.set_column_spacing(10);

    m_EntryPgn.set_hexpand(true);
    m_EntrySgf.set_hexpand(true);

    int row = 0;
    m_GridOutput.attach(m_LblPgn,       0, row);
    m_GridOutput.attach(m_EntryPgn,     1, row);
    m_GridOutput.attach(m_BtnBrowsePgn, 2, row++);
    m_GridOutput.attach(m_LblSgf,       0, row);
    m_GridOutput.attach(m_EntrySgf,     1, row);
    m_GridOutput.attach(m_BtnBrowseSgf, 2, row++);

    m_BtnBrowsePgn.signal_clicked().connect(
        [this]() { on_browse_file(&m_EntryPgn, "Select PGN File"); });
    m_BtnBrowseSgf.signal_clicked().connect(
        [this]() { on_browse_file(&m_EntrySgf, "Select SGF File"); });

    box->append(m_ChkLog);
    box->append(m_ChkDebug);

    return box;
}

// ============================================================
//  Advanced Tab
// ============================================================

Gtk::Box* TournamentDialog::build_advanced_tab()
{
    m_BoxAdvanced.set_margin(10);
    m_BoxAdvanced.set_spacing(8);

    m_ChkUseTURN.set_active(true);  // default ON

    m_BoxAdvanced.append(m_ChkGauntlet);
    m_BoxAdvanced.append(m_ChkLoseOnly);
    m_BoxAdvanced.append(m_ChkUseTURN);
    m_BoxAdvanced.append(m_ChkFatalError);

    return &m_BoxAdvanced;
}

// ============================================================
//  Getters — build Options / EngineOptions from UI state
// ============================================================

Options TournamentDialog::get_options() const
{
    Options o;

    o.boardSize    = m_SpinBoardSize.get_value_as_int();
    o.gameRule     = static_cast<GameRule>(std::stoi(m_ComboGameRule.get_active_id()));
    o.games        = m_SpinGames.get_value_as_int();
    o.rounds       = m_SpinRounds.get_value_as_int();
    o.concurrency  = m_SpinConcurrency.get_value_as_int();

    o.openings     = m_EntryOpening.get_text();
    o.openingType  = (m_ComboOpeningType.get_active_id() == "pos") ? OPENING_POS : OPENING_OFFSET;
    o.random       = m_ChkRandomOrder.get_active();
    o.repeat       = m_ChkRepeat.get_active();
    o.transform    = m_ChkTransform.get_active();

    o.resignCount      = m_SpinResignCount.get_value_as_int();
    o.resignScore      = m_SpinResignScore.get_value_as_int();
    o.drawCount        = m_SpinDrawCount.get_value_as_int();
    o.drawScore        = m_SpinDrawScore.get_value_as_int();
    o.forceDrawAfter   = m_SpinDrawAfter.get_value_as_int();

    o.pgn          = m_EntryPgn.get_text();
    o.sgf          = m_EntrySgf.get_text();
    o.log          = m_ChkLog.get_active();
    o.debug        = m_ChkDebug.get_active();

    o.gauntlet     = m_ChkGauntlet.get_active();
    o.saveLoseOnly = m_ChkLoseOnly.get_active();
    o.useTURN      = m_ChkUseTURN.get_active();
    o.fatalError   = m_ChkFatalError.get_active();

    return o;
}

std::vector<EngineOptions> TournamentDialog::get_engine_options() const
{
    std::vector<EngineOptions> eos;

    int64_t turnMs   = static_cast<int64_t>(m_SpinTurnTimeout.get_value());
    int64_t matchMs  = static_cast<int64_t>(m_SpinMatchTimeout.get_value());
    int64_t incMs    = static_cast<int64_t>(m_SpinIncrement.get_value());
    int64_t memBytes = static_cast<int64_t>(m_SpinMaxMemory.get_value()) * 1048576LL;
    int     threads  = m_SpinThreads.get_value_as_int();
    int64_t tolMs    = static_cast<int64_t>(m_SpinTolerance.get_value() * 1000);

    for (size_t i = 0; i < m_EngineRows.size(); i++) {
        EngineOptions e;
        e.cmd  = m_EngineRows[i]->entryCmd.get_text();
        e.name = m_EngineRows[i]->entryName.get_text();
        if (e.name.empty()) {
            e.name = "Engine" + std::to_string(i + 1);
        }

        e.timeoutTurn  = turnMs;
        e.timeoutMatch = matchMs;
        e.increment    = incMs;
        e.maxMemory    = memBytes;
        e.numThreads   = threads;
        e.tolerance    = tolMs;

        eos.push_back(e);
    }

    return eos;
}

// ============================================================
//  Save/Load Settings
// ============================================================

void TournamentDialog::on_save_settings()
{
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Save Tournament Settings");
    dialog->set_initial_name("tournament_config.json");

    dialog->save(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            auto file = dialog->save_finish(result);
            if (file) {
                std::string path = file->get_path();
                std::ofstream ofs(path);
                if (ofs.is_open()) {
                    Options o = get_options();
                    std::vector<EngineOptions> eos = get_engine_options();
                    
                    // Simple manual JSON construction
                    ofs << "{\n";
                    ofs << "  \"options\": ";
                    o.to_json(ofs);
                    ofs << ",\n";
                    ofs << "  \"engines\": [\n";
                    for (size_t i = 0; i < eos.size(); ++i) {
                        eos[i].to_json(ofs);
                        if (i < eos.size() - 1) ofs << ",";
                        ofs << "\n";
                    }
                    ofs << "  ]\n";
                    ofs << "}\n";
                    ofs.close();
                }
            }
        } catch (const Glib::Error& err) {
            std::cerr << "Save error: " << err.what() << std::endl;
        }
    });
}

void TournamentDialog::on_load_settings()
{
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Load Tournament Settings");
    
    // Filter for JSON
    auto filter = Gtk::FileFilter::create();
    filter->set_name("JSON Files");
    filter->add_pattern("*.json");
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    filters->append(filter);
    dialog->set_filters(filters);

    dialog->open(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            auto file = dialog->open_finish(result);
            if (file) {
                std::string path = file->get_path();
                std::ifstream ifs(path);
                if (ifs.is_open()) {
                    // Simple manual JSON parsing (naive)
                    // We expect {"options": {...}, "engines": [...]}
                    
                    Options o;
                    std::vector<EngineOptions> eos;
                    
                    char c;
                    while (ifs.get(c)) {
                        if (c == '{') break; 
                    }
                    
                    // look for keys
                    while (ifs.good()) {
                        while (isspace(ifs.peek())) ifs.get();
                        
                        if (ifs.peek() == '"') {
                            std::string key;
                            ifs.get(); // "
                            while (ifs.good()) {
                                char k = ifs.get();
                                if (k == '"') break;
                                key += k;
                            }
                            
                            while (isspace(ifs.peek())) ifs.get();
                            if (ifs.peek() == ':') ifs.get();
                            
                            if (key == "options") {
                                o.from_json(ifs);
                            } else if (key == "engines") {
                                while (isspace(ifs.peek())) ifs.get();
                                if (ifs.peek() == '[') {
                                    ifs.get();
                                    while (ifs.good()) {
                                        while (isspace(ifs.peek())) ifs.get();
                                        if (ifs.peek() == ']') { ifs.get(); break; }
                                        if (ifs.peek() == ',') { ifs.get(); continue; }
                                        
                                        EngineOptions eo;
                                        eo.from_json(ifs);
                                        eos.push_back(eo);
                                    }
                                }
                            } else {
                                Options::skip_json_value(ifs);
                            }
                        }
                        
                        while (isspace(ifs.peek())) ifs.get();
                        if (ifs.peek() == ',') ifs.get();
                        if (ifs.peek() == '}') break; // End of root object
                    }
                    
                    apply_options_to_ui(o);
                    apply_engine_options_to_ui(eos);
                }
            }
        } catch (const Glib::Error& err) {
            std::cerr << "Load error: " << err.what() << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "Exception: " << ex.what() << std::endl;
        }
    });
}

void TournamentDialog::apply_options_to_ui(const Options& o)
{
    m_SpinBoardSize.set_value(o.boardSize);
    m_ComboGameRule.set_active_id(std::to_string((int)o.gameRule));
    m_SpinGames.set_value(o.games);
    m_SpinRounds.set_value(o.rounds);
    m_SpinConcurrency.set_value(o.concurrency);

    m_EntryOpening.set_text(o.openings);
    m_ComboOpeningType.set_active_id(o.openingType == OPENING_POS ? "pos" : "offset");
    m_ChkRandomOrder.set_active(o.random);
    m_ChkRepeat.set_active(o.repeat);
    m_ChkTransform.set_active(o.transform);

    m_SpinResignCount.set_value(o.resignCount);
    m_SpinResignScore.set_value(o.resignScore);
    m_SpinDrawCount.set_value(o.drawCount);
    m_SpinDrawScore.set_value(o.drawScore);
    m_SpinDrawAfter.set_value(o.forceDrawAfter);

    m_EntryPgn.set_text(o.pgn);
    m_EntrySgf.set_text(o.sgf);
    m_ChkLog.set_active(o.log);
    m_ChkDebug.set_active(o.debug);

    m_ChkGauntlet.set_active(o.gauntlet);
    m_ChkLoseOnly.set_active(o.saveLoseOnly);
    m_ChkUseTURN.set_active(o.useTURN);
    m_ChkFatalError.set_active(o.fatalError);
}

void TournamentDialog::apply_engine_options_to_ui(const std::vector<EngineOptions>& eos)
{
    // Clear existing engines
    while (auto* child = m_BoxEngineList.get_first_child()) {
        m_BoxEngineList.remove(*child);
    }
    m_EngineRows.clear();
    
    // Add engines from list
    for (const auto& eo : eos) {
        on_add_engine(); // adds a minimal row
        // Populate the last added row
        auto& row = m_EngineRows.back();
        row->entryCmd.set_text(eo.cmd);
        row->entryName.set_text(eo.name);
    }
    
    // Update common settings from the first engine (if any)
    if (!eos.empty()) {
        const auto& first = eos[0];
        m_SpinTurnTimeout.set_value(first.timeoutTurn);
        m_SpinMatchTimeout.set_value(first.timeoutMatch);
        m_SpinIncrement.set_value(first.increment);
        m_SpinMaxMemory.set_value(first.maxMemory / 1048576);
        m_SpinThreads.set_value(first.numThreads);
        m_SpinTolerance.set_value(first.tolerance / 1000.0);
    }
    
    // Ensure we have at least 2 rows visual
    while (m_EngineRows.size() < 2) {
        on_add_engine();
    }
    update_remove_buttons();
}
