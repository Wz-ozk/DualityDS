/*
    Copyright 2016-2025 melonDS team, DualityDS contributors

    This file is part of DualityDS, based on melonDS.

    DualityDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#pragma once

// Application-wide stylesheet giving DualityDS the soft white/silver console
// faceplate look (light gray surfaces, blue accents) used by the game library.
// Applied to qApp; per-widget stylesheets (carousel buttons, bottom bar) still
// win over these rules. The OpenGL emulator screen is untouched (bare QWidget is
// never targeted here).
inline const char* const DUALITY_THEME_QSS = R"QSS(
/* ---- Default text color (fixes white-on-light labels in config dialogs) ---- */
QWidget { color: #2B3038; }
QLabel { background: transparent; }
QToolTip { color: #2B3038; background: #FFFFFF; border: 1px solid #C6D2DC; }
QHeaderView::section {
    background: #E4EBF1; color: #3A4750;
    border: 1px solid #C6D2DC; padding: 4px;
}

/* ---- Menu bar (File / System / View / Config …) ---- */
QMenuBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #FBFCFD, stop:1 #DCE3EA);
    color: #2B3038;
    border-bottom: 1px solid rgba(120,180,235,0.45);
}
QMenuBar::item { background: transparent; padding: 6px 12px; }
QMenuBar::item:selected { background: #CFE6F8; border-radius: 6px; }
QMenuBar::item:pressed { background: #5BB4EE; color: #FFFFFF; border-radius: 6px; }

/* ---- Drop-down menus ---- */
QMenu {
    background: #FFFFFF; color: #2B3038;
    border: 1px solid #C6D2DC; border-radius: 8px; padding: 4px;
}
QMenu::item { padding: 6px 24px 6px 20px; border-radius: 6px; }
QMenu::item:selected { background: #5BB4EE; color: #FFFFFF; }
QMenu::item:disabled { color: #A6B2BC; }
QMenu::separator { height: 1px; background: #E1E7EC; margin: 4px 8px; }

/* ---- Toolbars / status bar ---- */
QToolBar { background: #EDF1F5; border: none; spacing: 4px; }
QStatusBar { background: #EDF1F5; color: #3A4750; }

/* ---- Dialogs ---- */
QDialog, QMessageBox { background: #F3F6F9; }

/* ---- Buttons (carousel buttons override this with their own QSS) ---- */
QPushButton {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #FFFFFF, stop:1 #E4EBF1);
    color: #2A6FA0; border: 1px solid #BFCDD8; border-radius: 7px;
    padding: 6px 14px;
}
QPushButton:hover { border: 1px solid #5BB4EE; background: #EAF4FB; }
QPushButton:pressed { background: #D6E9F7; }
QPushButton:disabled { color: #A6B2BC; background: #EEF1F4; border: 1px solid #D6DCE2; }

/* ---- Inputs / lists ---- */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit,
QListWidget, QListView, QTreeWidget, QTreeView, QTableWidget, QTableView {
    background: #FFFFFF; color: #2B3038;
    border: 1px solid #C6D2DC; border-radius: 6px;
    selection-background-color: #5BB4EE; selection-color: #FFFFFF;
}
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { padding: 4px 6px; }
QComboBox::drop-down { border: none; width: 18px; }

/* ---- Tabs ---- */
QTabWidget::pane { border: 1px solid #C6D2DC; border-radius: 6px; top: -1px; }
QTabBar::tab {
    background: #E4EBF1; color: #3A4750; padding: 6px 14px;
    border: 1px solid #C6D2DC; border-bottom: none;
    border-top-left-radius: 6px; border-top-right-radius: 6px;
}
QTabBar::tab:selected { background: #FFFFFF; color: #2A6FA0; }
QTabBar::tab:hover { background: #EAF4FB; }

/* ---- Group boxes / toggles ---- */
QGroupBox {
    border: 1px solid #C6D2DC; border-radius: 8px;
    margin-top: 10px; padding-top: 6px; color: #2B3038;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #2A6FA0; }
QCheckBox, QRadioButton { color: #2B3038; }

/* ---- Scrollbars ---- */
QScrollBar:vertical { background: #EDF1F5; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #BFCDD8; border-radius: 6px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #5BB4EE; }
QScrollBar:horizontal { background: #EDF1F5; height: 12px; margin: 0; }
QScrollBar::handle:horizontal { background: #BFCDD8; border-radius: 6px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: #5BB4EE; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; background: none; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }
)QSS";
