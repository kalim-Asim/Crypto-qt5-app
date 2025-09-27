#pragma once  // ensures the header is only included once during compilation

#include <QMainWindow>   // base class for the main application window
#include <QProgressBar>  // widget to show progress (e.g., file processing %)
#include <QLabel>        // widget to display text labels (status/info)
#include <QPushButton>   // clickable buttons (Upload, Download, etc.)
#include <QTextEdit>     // multi-line text editor (for logs/output)
#include <QComboBox>     // drop-down selection box (choose operation)
#include <QLineEdit>     // single-line text field (enter or show keys)

class MainWindow : public QMainWindow {
    Q_OBJECT // macro enables Qt’s signals & slots system (automatic event handling like button clicks)

public:
    MainWindow(QWidget* parent = nullptr);

private slots: // Event Handlers
    void onUpload();
    void onProcess();
    void onDownload();
    void onGenerateKey();

private:
    void loadConfig();
    void setStatus(const QString& s);
    bool readFileToByteArray(const QString& path, QByteArray& out);
    bool writeByteArrayToFile(const QString& path, const QByteArray& data);

    QPushButton* uploadBtn;
    QPushButton* processBtn;
    QPushButton* downloadBtn;
    QPushButton* genKeyBtn;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QTextEdit* outputText;
    QComboBox* opCombo;
    QLineEdit* keyHexEdit;   // show symmetric key in hex
    QLineEdit* hmacKeyEdit;  // hmac key in hex (optional)


    QString inputFilePath;
    QByteArray processedData;

    // crypto params
    int aesKeyBytes = 32;
    int aesIvBytes = 16;
    int hmacKeyBytes = 32;

    // state tracking for download behavior & previews
    bool lastOutputIsText = false;
    QString lastTextOutput; // UTF-8 text to save if lastOutputIsText == true

    // keys generated & last action
    QString lastGeneratedSymKeyHex;
    QString lastGeneratedHmacKeyHex;
    enum class LastAction { 
        None, 
        GeneratedKey, 
        ProcessedData, 
        ShaOrHmacText 
    } lastAction = LastAction::None;
};
