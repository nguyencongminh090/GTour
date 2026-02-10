#pragma once
#include <gtkmm.h>
#include <vector>
#include <string>
#include <memory>

#include "options.h"

// One row in the dynamic engine list
struct EngineRow {
    Gtk::Box    box{Gtk::Orientation::HORIZONTAL};
    Gtk::Entry  entryCmd;
    Gtk::Entry  entryName;
    Gtk::Button btnBrowse{"Browse..."};
    Gtk::Button btnRemove{"✕"};
};

class TournamentDialog : public Gtk::Dialog
{
public:
    TournamentDialog(Gtk::Window& parent);
    virtual ~TournamentDialog();

    // Returns fully populated core structs (no intermediate TournamentSettings)
    Options                    get_options() const;
    std::vector<EngineOptions> get_engine_options() const;

protected:
    // Signal handlers
    void on_add_engine();
    void on_remove_engine(EngineRow* row);
    void on_browse_engine(Gtk::Entry* entry);
    void on_browse_file(Gtk::Entry* entry, const std::string& title);

    // Build each tab page
    Gtk::Box* build_engines_tab();
    Gtk::Box* build_game_tab();
    Gtk::Box* build_openings_tab();
    Gtk::Box* build_adjudication_tab();
    Gtk::Box* build_output_tab();
    Gtk::Box* build_advanced_tab();

    void update_remove_buttons();

    // Settings load/save
    void on_load_settings();
    void on_save_settings();

    // Helpers to populate UI from Options/EngineOptions
    void apply_options_to_ui(const Options& o);
    void apply_engine_options_to_ui(const std::vector<EngineOptions>& eos);

    // ---- Top-level layout ----
    Gtk::Box      m_ContentBox{Gtk::Orientation::VERTICAL};
    Gtk::Notebook m_Notebook;

    // Action buttons (inside content box)
    Gtk::Box    m_BoxButtons{Gtk::Orientation::HORIZONTAL};
    Gtk::Button m_BtnLoadSettings{"Load Settings..."};
    Gtk::Button m_BtnSaveSettings{"Save Settings..."};
    Gtk::Button m_BtnCancel{"Cancel"};
    Gtk::Button m_BtnStart{"Start"};

    // ======== Engines tab ========
    Gtk::Box    m_BoxEngines{Gtk::Orientation::VERTICAL};
    Gtk::Box    m_BoxEngineList{Gtk::Orientation::VERTICAL};  // container for engine rows only
    Gtk::Box    m_BoxEngineButtons{Gtk::Orientation::HORIZONTAL};
    Gtk::Button m_BtnAddEngine{"+ Add Engine"};
    std::vector<std::unique_ptr<EngineRow>> m_EngineRows;

    // Shared engine settings ("each")
    Gtk::Frame      m_FrameEachTC{"Default Time Control (all engines)"};
    Gtk::Grid       m_GridEachTC;
    Gtk::Label      m_LblTurnTimeout{"Turn timeout (ms):"};
    Gtk::SpinButton m_SpinTurnTimeout;
    Gtk::Label      m_LblMatchTimeout{"Match timeout (ms):"};
    Gtk::SpinButton m_SpinMatchTimeout;
    Gtk::Label      m_LblIncrement{"Increment (ms):"};
    Gtk::SpinButton m_SpinIncrement;
    Gtk::Label      m_LblMaxMemory{"Max memory (MB):"};
    Gtk::SpinButton m_SpinMaxMemory;
    Gtk::Label      m_LblThreads{"Threads:"};
    Gtk::SpinButton m_SpinThreads;
    Gtk::Label      m_LblTolerance{"Tolerance (s):"};
    Gtk::SpinButton m_SpinTolerance;

    // ======== Game tab ========
    Gtk::Grid       m_GridGame;
    Gtk::Label      m_LblBoardSize{"Board size:"};
    Gtk::SpinButton m_SpinBoardSize;
    Gtk::Label      m_LblGameRule{"Game rule:"};
    Gtk::ComboBoxText m_ComboGameRule;
    Gtk::Label      m_LblGames{"Games per pair:"};
    Gtk::SpinButton m_SpinGames;
    Gtk::Label      m_LblRounds{"Rounds:"};
    Gtk::SpinButton m_SpinRounds;
    Gtk::Label      m_LblConcurrency{"Concurrency:"};
    Gtk::SpinButton m_SpinConcurrency;

    // ======== Openings tab ========
    Gtk::Grid       m_GridOpenings;
    Gtk::Label      m_LblOpeningFile{"Opening file:"};
    Gtk::Entry      m_EntryOpening;
    Gtk::Button     m_BtnBrowseOpening{"..."};
    Gtk::Label      m_LblOpeningType{"Type:"};
    Gtk::ComboBoxText m_ComboOpeningType;
    Gtk::CheckButton m_ChkRandomOrder{"Random order"};
    Gtk::CheckButton m_ChkRepeat{"Repeat openings"};
    Gtk::CheckButton m_ChkTransform{"Apply transformations"};

    // ======== Adjudication tab ========
    Gtk::Grid       m_GridAdjudication;
    Gtk::Label      m_LblResignCount{"Resign move count:"};
    Gtk::SpinButton m_SpinResignCount;
    Gtk::Label      m_LblResignScore{"Resign score:"};
    Gtk::SpinButton m_SpinResignScore;
    Gtk::Label      m_LblDrawCount{"Draw move count:"};
    Gtk::SpinButton m_SpinDrawCount;
    Gtk::Label      m_LblDrawScore{"Draw score:"};
    Gtk::SpinButton m_SpinDrawScore;
    Gtk::Label      m_LblDrawAfter{"Force draw after:"};
    Gtk::SpinButton m_SpinDrawAfter;

    // ======== Output tab ========
    Gtk::Grid       m_GridOutput;
    Gtk::Label      m_LblPgn{"PGN file:"};
    Gtk::Entry      m_EntryPgn;
    Gtk::Button     m_BtnBrowsePgn{"..."};
    Gtk::Label      m_LblSgf{"SGF file:"};
    Gtk::Entry      m_EntrySgf;
    Gtk::Button     m_BtnBrowseSgf{"..."};
    Gtk::CheckButton m_ChkLog{"Enable logging"};
    Gtk::CheckButton m_ChkDebug{"Debug mode"};

    // ======== Advanced tab ========
    Gtk::Box        m_BoxAdvanced{Gtk::Orientation::VERTICAL};
    Gtk::CheckButton m_ChkGauntlet{"Gauntlet mode"};
    Gtk::CheckButton m_ChkLoseOnly{"Save losses only"};
    Gtk::CheckButton m_ChkUseTURN{"Use TURN protocol (vs BOARD)"};
    Gtk::CheckButton m_ChkFatalError{"Treat engine errors as fatal"};
};
