#pragma once
#include <gtkmm.h>
#include <memory>
#include <vector>
#include "BoardWidget.h"
#include "OpeningCreatorPanel.h"
#include "GraphWidget.h"
#include "TournamentManager.h"
#include "WebServer.h"
#include "options.h"

class MainWindow : public Gtk::ApplicationWindow
{
public:
    MainWindow();
    virtual ~MainWindow();

protected:
    // Signal handlers
    void on_menu_file_quit();
    void on_menu_tournament_new();
    void on_menu_opening_creator();
    void on_action_stop();
    void update_ui_state(bool is_running);
    bool on_timeout_update();
    void start_tournament(const Options& opts, const std::vector<EngineOptions>& eos);
    void update_results_table(const std::vector<PairResult>& results);
    void show_tournament_panel();
    void show_opening_panel();

    // Layout
    Gtk::Box m_VBox;
    Gtk::Paned m_Paned;           // Horizontal split: board | controls
    Gtk::Stack m_RightStack;      // Switch between tournament controls & opening creator
    
    // Board
    BoardWidget m_BoardWidget;

    // Controls panel (tournament mode)
    Gtk::Box m_ControlBox;
    Gtk::Label m_LabelStatus;
    Gtk::Label m_LabelLastResult;
    Gtk::ProgressBar m_ProgressBar;
    Gtk::Button m_BtnStop;
    
    // Player Bar
    Gtk::Box m_PlayerBar;
    Gtk::Frame m_FrameBlack, m_FrameWhite;
    Gtk::Box m_BoxBlack, m_BoxWhite; // Inside frames
    Gtk::Label m_LabelBlackName, m_LabelWhiteName;
    Gtk::Label m_LabelBlackTime, m_LabelWhiteTime;
    Gtk::Label m_LabelBlackSymbol, m_LabelWhiteSymbol;

    // Log view
    Gtk::Frame m_FrameLog{"Log"};
    Gtk::ScrolledWindow m_ScrollLog;
    Gtk::TextView m_TextViewLog;
    Glib::RefPtr<Gtk::TextBuffer> m_LogBuffer;

    // Graph
    GraphWidget m_GraphWidget;
    void on_engine_eval(const TournamentManager::EvalInfo& info);

    // Results table
    Gtk::Frame m_FrameResults{"Results"};
    Gtk::ScrolledWindow m_ScrollResults;
    Gtk::TreeView m_TreeViewResults;
    Glib::RefPtr<Gtk::ListStore> m_ResultsStore;

    class ResultColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ResultColumns() {
            add(col_name1); add(col_name2);
            add(col_wins); add(col_losses); add(col_draws);
            add(col_score); add(col_total);
        }
        Gtk::TreeModelColumn<Glib::ustring> col_name1, col_name2, col_score;
        Gtk::TreeModelColumn<int> col_wins, col_losses, col_draws, col_total;
    };
    ResultColumns m_ResultColumns;

    // Opening creator panel
    OpeningCreatorPanel m_OpeningPanel;

    // Action Group for menus
    Glib::RefPtr<Gio::SimpleActionGroup> m_refActionGroup;
    Glib::RefPtr<Gio::SimpleAction> m_refActionNewTournament;
    Glib::RefPtr<Gio::SimpleAction> m_refActionStop;

    // Core Logic
    std::unique_ptr<class TournamentManager> m_Manager;
    std::unique_ptr<WebServer> m_WebServer;
    sigc::connection m_TimerConnection;
};
