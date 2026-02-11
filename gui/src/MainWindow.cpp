#include "MainWindow.h"
#include "TournamentDialog.h"
#include "TournamentManager.h"
#include <iomanip>
#include <sstream>
#include <thread>

MainWindow::MainWindow()
: m_VBox(Gtk::Orientation::VERTICAL)
, m_Paned(Gtk::Orientation::HORIZONTAL)
, m_ControlBox(Gtk::Orientation::VERTICAL, 8)
, m_LabelStatus("Welcome to Gomoku Tournament Manager")
, m_OpeningPanel(m_BoardWidget)
{
    set_title("Gomoku Tournament Manager");
    set_default_size(1200, 750);

    set_child(m_VBox);

    // Create Actions
    m_refActionGroup = Gio::SimpleActionGroup::create();
    m_refActionGroup->add_action("quit", sigc::mem_fun(*this, &MainWindow::on_menu_file_quit));
    insert_action_group("app", m_refActionGroup);

    // Menu construction
    auto menuModel = Gio::Menu::create();
    auto fileMenu = Gio::Menu::create();
    auto tourMenu = Gio::Menu::create();
    auto openingMenu = Gio::Menu::create();

    fileMenu->append("Quit", "app.quit");
    tourMenu->append("New Tournament...", "app.new_tournament");
    tourMenu->append("Stop", "app.stop");
    openingMenu->append("Create Opening...", "app.opening_creator");

    menuModel->append_submenu("File", fileMenu);
    menuModel->append_submenu("Tournament", tourMenu);
    menuModel->append_submenu("Opening", openingMenu);

    auto menuBar = Gtk::make_managed<Gtk::PopoverMenuBar>(menuModel);
    m_VBox.append(*menuBar);

    // ─── Left: Board Widget ───
    m_BoardWidget.set_size_request(500, 500);
    m_Paned.set_start_child(m_BoardWidget);
    m_Paned.set_resize_start_child(true);
    m_Paned.set_shrink_start_child(false);

    // ─── Right: Stack with two panels ───

    // --- Tournament controls panel ---
    m_ControlBox.set_margin(12);
    m_ControlBox.set_valign(Gtk::Align::FILL);
    m_ControlBox.set_size_request(400, -1);

    m_LabelStatus.set_css_classes({"title-3"});
    m_LabelStatus.set_wrap(true);
    m_LabelStatus.set_halign(Gtk::Align::START);
    m_ControlBox.append(m_LabelStatus);

    auto sep = Gtk::make_managed<Gtk::Separator>();
    m_ControlBox.append(*sep);

    // Graph
    m_GraphWidget.set_margin_bottom(10);
    m_ControlBox.append(m_GraphWidget);

    m_ProgressBar.set_show_text(true);
    m_ProgressBar.set_visible(false);
    m_ControlBox.append(m_ProgressBar);

    m_LabelLastResult.set_visible(false);
    m_LabelLastResult.set_wrap(true);
    m_LabelLastResult.set_halign(Gtk::Align::START);
    m_LabelLastResult.set_css_classes({"dim-label"});
    m_ControlBox.append(m_LabelLastResult);

    m_BtnStop.set_label("Stop Tournament");
    m_BtnStop.set_visible(false);
    m_BtnStop.set_halign(Gtk::Align::CENTER);
    m_BtnStop.set_margin_top(6);
    m_BtnStop.set_css_classes({"destructive-action"});
    m_BtnStop.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_action_stop));
    m_ControlBox.append(m_BtnStop);

    // Results table
    m_ResultsStore = Gtk::ListStore::create(m_ResultColumns);
    m_TreeViewResults.set_model(m_ResultsStore);
    m_TreeViewResults.append_column("Engine 1", m_ResultColumns.col_name1);
    m_TreeViewResults.append_column("Engine 2", m_ResultColumns.col_name2);
    m_TreeViewResults.append_column("W",        m_ResultColumns.col_wins);
    m_TreeViewResults.append_column("L",        m_ResultColumns.col_losses);
    m_TreeViewResults.append_column("D",        m_ResultColumns.col_draws);
    m_TreeViewResults.append_column("Score",    m_ResultColumns.col_score);
    m_TreeViewResults.append_column("Total",    m_ResultColumns.col_total);
    m_TreeViewResults.set_headers_visible(true);

    m_ScrollResults.set_child(m_TreeViewResults);
    m_ScrollResults.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_ScrollResults.set_min_content_height(100);
    m_FrameResults.set_child(m_ScrollResults);
    m_FrameResults.set_visible(false);
    m_ControlBox.append(m_FrameResults);

    // Log view
    m_LogBuffer = Gtk::TextBuffer::create();
    m_TextViewLog.set_buffer(m_LogBuffer);
    m_TextViewLog.set_editable(false);
    m_TextViewLog.set_cursor_visible(false);
    m_TextViewLog.set_wrap_mode(Gtk::WrapMode::WORD_CHAR);
    m_TextViewLog.set_monospace(true);

    m_ScrollLog.set_child(m_TextViewLog);
    m_ScrollLog.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_ScrollLog.set_vexpand(true);
    m_ScrollLog.set_min_content_height(120);
    m_FrameLog.set_child(m_ScrollLog);
    
    // Player Bar
    m_PlayerBar.set_spacing(10);
    m_PlayerBar.set_margin_top(5);
    m_PlayerBar.set_margin_bottom(5);
    m_PlayerBar.set_margin_start(5);
    m_PlayerBar.set_margin_end(5);

    // Black Side
    m_BoxBlack.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_BoxBlack.set_spacing(10);
    m_BoxBlack.set_margin_start(10);
    m_BoxBlack.set_margin_end(10);
    m_BoxBlack.set_margin_top(5);
    m_BoxBlack.set_margin_bottom(5);
    
    // Black Side (Time Name Symbol)
    m_LabelBlackTime.set_text("--:--");
    m_LabelBlackTime.set_halign(Gtk::Align::START);
    m_LabelBlackTime.set_width_chars(6);

    m_LabelBlackName.set_text("Black");
    m_LabelBlackName.set_halign(Gtk::Align::END); // Name aligns to right (near symbol) or center?
    // Actually, if Time is Left, Name should be Center/Right?
    // Let's make Name fill space.
    m_LabelBlackName.set_halign(Gtk::Align::FILL);
    m_LabelBlackName.set_hexpand(true);
    
    m_LabelBlackSymbol.set_markup("<span size='large'>●</span>");
    
    m_BoxBlack.append(m_LabelBlackTime);
    m_BoxBlack.append(m_LabelBlackName);
    m_BoxBlack.append(m_LabelBlackSymbol);
    m_FrameBlack.set_child(m_BoxBlack);
    m_FrameBlack.set_hexpand(true);
    
    // White Side (Symmetric: Time Name Symbol)
    m_BoxWhite.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_BoxWhite.set_spacing(10);
    m_BoxWhite.set_margin_start(10);
    m_BoxWhite.set_margin_end(10);
    m_BoxWhite.set_margin_top(5);
    m_BoxWhite.set_margin_bottom(5);

    // White Side (Symbol Name Time)
    m_LabelWhiteSymbol.set_markup("<span size='large'>○</span>");

    m_LabelWhiteName.set_text("White");
    m_LabelWhiteName.set_halign(Gtk::Align::FILL);
    m_LabelWhiteName.set_hexpand(true);
    
    m_LabelWhiteTime.set_text("--:--");
    m_LabelWhiteTime.set_halign(Gtk::Align::END);
    m_LabelWhiteTime.set_width_chars(6);

    m_BoxWhite.append(m_LabelWhiteSymbol);
    m_BoxWhite.append(m_LabelWhiteName);
    m_BoxWhite.append(m_LabelWhiteTime);
    m_FrameWhite.set_child(m_BoxWhite);
    m_FrameWhite.set_hexpand(true);

    m_PlayerBar.append(m_FrameBlack);
    m_PlayerBar.append(m_FrameWhite);

    m_VBox.append(m_PlayerBar);

    m_FrameLog.set_visible(false);
    m_FrameLog.set_vexpand(true);
    m_ControlBox.append(m_FrameLog);

    // --- Stack setup ---
    m_RightStack.add(m_ControlBox, "tournament", "Tournament");
    m_RightStack.add(m_OpeningPanel, "opening", "Opening Creator");
    m_RightStack.set_visible_child("tournament");

    m_Paned.set_end_child(m_RightStack);
    m_Paned.set_resize_end_child(false);
    m_Paned.set_shrink_end_child(false);

    m_Paned.set_position(700);
    m_VBox.append(m_Paned);
    m_Paned.set_vexpand(true);

    // Register remaining actions
    m_refActionNewTournament = m_refActionGroup->add_action("new_tournament", sigc::mem_fun(*this, &MainWindow::on_menu_tournament_new));
    m_refActionStop = m_refActionGroup->add_action("stop", sigc::mem_fun(*this, &MainWindow::on_action_stop));
    m_refActionStop->set_enabled(false);
    m_refActionGroup->add_action("opening_creator", sigc::mem_fun(*this, &MainWindow::on_menu_opening_creator));

    // Show an empty board immediately
    BoardState emptyBoard;
    m_BoardWidget.update_state(emptyBoard);
}

void MainWindow::on_action_stop()
{
    if (m_Manager) {
        // Update UI immediately (don't wait for stop to finish)
        m_LabelStatus.set_text("Stopping Tournament...");
        update_ui_state(false);
        m_TimerConnection.disconnect();

        // Run both WebServer and TournamentManager stop in a background thread
        // to avoid blocking the GTK main loop.
        auto* mgr = m_Manager.get();
        auto* ws  = m_WebServer.release();  // transfer ownership to thread
        std::thread([mgr, ws]() {
            // Stop web server first (fast — signals SSE loop to exit)
            if (ws) {
                ws->stop();
                delete ws;
            }
            mgr->stop();
        }).detach();

        m_LabelStatus.set_text("Tournament Stopped by User");
    }
}

void MainWindow::update_ui_state(bool is_running)
{
    m_refActionNewTournament->set_enabled(!is_running);
    m_refActionStop->set_enabled(is_running);
    m_BtnStop.set_visible(is_running);
    m_ProgressBar.set_visible(is_running);
    m_FrameLog.set_visible(is_running || m_LogBuffer->size() > 0);
    m_FrameResults.set_visible(is_running || m_ResultsStore->children().size() > 0);
    if (!is_running) {
        m_LabelLastResult.set_visible(false);
    }
}

MainWindow::~MainWindow()
{
    m_TimerConnection.disconnect();
}

void MainWindow::on_menu_file_quit()
{
    hide();
}

void MainWindow::on_menu_tournament_new()
{
    show_tournament_panel();

    auto dialog = new TournamentDialog(*this);
    
    dialog->signal_response().connect([this, dialog](int response_id) {
        if (response_id == Gtk::ResponseType::OK) {
            auto opts = dialog->get_options();
            auto eos  = dialog->get_engine_options();
            start_tournament(opts, eos);
        }
        delete dialog;
    });

    dialog->show();
}

void MainWindow::on_menu_opening_creator()
{
    show_opening_panel();
}

void MainWindow::show_tournament_panel()
{
    m_OpeningPanel.deactivate();
    m_RightStack.set_visible_child("tournament");
}

void MainWindow::show_opening_panel()
{
    m_RightStack.set_visible_child("opening");
    m_OpeningPanel.activate();
}

void MainWindow::start_tournament(const Options& opts, const std::vector<EngineOptions>& eos)
{
    m_LogBuffer->set_text("");
    m_ResultsStore->clear();

    m_Manager = std::make_unique<TournamentManager>();
    m_Manager->onEngineEval = sigc::mem_fun(*this, &MainWindow::on_engine_eval);
    m_Manager->init(opts, eos);
    m_Manager->start();
    
    // Start the web server for live viewing
    m_WebServer = std::make_unique<WebServer>(*m_Manager, 8080);
    m_WebServer->start();
    
    m_GraphWidget.clear();

    m_LabelStatus.set_text("Tournament Running — Live at http://localhost:8080");
    m_ProgressBar.set_fraction(0.0);
    m_LabelLastResult.set_visible(false);
    update_ui_state(true);
    
    m_TimerConnection = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &MainWindow::on_timeout_update), 100
    );
}

bool MainWindow::on_timeout_update()
{
    if (m_Manager) {
        auto progress = m_Manager->getProgress();
        
        if (progress.gamesTotal > 0) {
            double frac = static_cast<double>(progress.gamesCompleted) / progress.gamesTotal;
            m_ProgressBar.set_fraction(frac);
            m_ProgressBar.set_text(
                std::to_string(progress.gamesCompleted) + " / " + std::to_string(progress.gamesTotal)
            );
        }

        if (!progress.workerStatuses.empty()) {
            // Show status of the first active worker
            const auto& ws = progress.workerStatuses[0];
            
            int64_t timeMs;
            auto formatTime = [](int64_t ms) {
                if (ms < 0) ms = 0;
                int totSec = ms / 1000;
                int min = totSec / 60;
                int sec = totSec % 60;
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << min << ":" << std::setw(2) << sec;
                return ss.str();
            };
            
            std::string bTimeStr = formatTime(ws.blackTime);
            std::string wTimeStr = formatTime(ws.whiteTime);

            if (ws.isBlackActive) {
                m_LabelBlackName.set_markup("<b>" + Glib::Markup::escape_text(ws.blackName) + "</b>");
                m_LabelBlackTime.set_markup("<b>" + bTimeStr + "</b>");
                
                m_LabelWhiteName.set_text(ws.whiteName);
                m_LabelWhiteTime.set_text(wTimeStr);
            } else {
                m_LabelWhiteName.set_markup("<b>" + Glib::Markup::escape_text(ws.whiteName) + "</b>");
                m_LabelWhiteTime.set_markup("<b>" + wTimeStr + "</b>");
                
                m_LabelBlackName.set_text(ws.blackName);
                m_LabelBlackTime.set_text(bTimeStr);
            }
        }

        if (!progress.lastResult.empty()) {
            m_LabelLastResult.set_text(progress.lastResult);
            m_LabelLastResult.set_visible(true);
        }

        // Batch log insertion to prevent UI freeze with high-volume engine output
        if (!progress.logLines.empty()) {
            std::string batch;
            // Pre-calculate size to avoid reallocations
            size_t totalSize = 0;
            for (const auto& line : progress.logLines) totalSize += line.size() + 1;
            batch.reserve(totalSize);

            for (const auto& line : progress.logLines) {
                batch += line;
                batch += "\n";
            }
            
            auto end_iter = m_LogBuffer->end();
            m_LogBuffer->insert(end_iter, batch);

            auto adj = m_ScrollLog.get_vadjustment();
            adj->set_value(adj->get_upper());
        }

        update_results_table(progress.pairResults);
        m_BoardWidget.update_state(progress.board);

        bool still_running = m_Manager->update();
        if (!still_running) {
            m_LabelStatus.set_text("Tournament Finished!");
            m_ProgressBar.set_fraction(1.0);
            m_Manager->stop();
            update_ui_state(false);
            m_ProgressBar.set_visible(true);
            m_LabelLastResult.set_visible(true);
            return false;
        }
    }
    return true;
}

void MainWindow::update_results_table(const std::vector<PairResult>& results)
{
    m_ResultsStore->clear();
    for (const auto& pr : results) {
        auto row = *(m_ResultsStore->append());
        row[m_ResultColumns.col_name1]  = pr.name1;
        row[m_ResultColumns.col_name2]  = pr.name2;
        row[m_ResultColumns.col_wins]   = pr.wins;
        row[m_ResultColumns.col_losses] = pr.losses;
        row[m_ResultColumns.col_draws]  = pr.draws;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.3f", pr.score);
        row[m_ResultColumns.col_score]  = Glib::ustring(buf);
        row[m_ResultColumns.col_total]  = pr.total;
    }
}

void MainWindow::on_engine_eval(const TournamentManager::EvalInfo& info)
{
    // Dispatch to GUI thread
    Glib::signal_idle().connect_once([this, info]() {
        m_GraphWidget.push_data(info.engineIdx, info.moveIdx, info.winrate);
    });
}
